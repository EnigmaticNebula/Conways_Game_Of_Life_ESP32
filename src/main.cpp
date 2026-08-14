#include <Arduino.h>
#include <iostream>
#include <simulation.hpp>
#include <led_array_driver.hpp>
#include <navigation_driver.hpp>

// put function declarations here:
int get_iteration_speed();
static void matrix_refresh(void* pvParameters);
static void simulation_loop(void* pvParameters);
void clear_buffers();
void reset_buffer(bool (*buffer)[16]);
void IRAM_ATTR play_pause_isr();
void IRAM_ATTR clear_isr();
void IRAM_ATTR iterate_isr();
void IRAM_ATTR nav_up();
void IRAM_ATTR nav_right();
void IRAM_ATTR nav_down();
void IRAM_ATTR nav_left();
void IRAM_ATTR nav_act();

bool sim_buffer[16][16];
bool display_buffer[16][16];

bool (*sim_buffer_ptr)[16] = sim_buffer;
bool (*display_buffer_ptr)[16] = display_buffer;

unsigned int last_iteration = 0;
unsigned int last_speed_check = 0;
unsigned int iteration_delay = 0;

volatile bool paused = false;
volatile bool cleared = false;
volatile bool iterate = false;

volatile unsigned int last_play_pause_press = 0;
volatile unsigned int last_cleared_press = 0;
volatile unsigned int last_iterate_press = 0;
volatile unsigned int last_nav_up_press = 0;
volatile unsigned int last_nav_right_press = 0;
volatile unsigned int last_nav_down_press = 0;
volatile unsigned int last_nav_left_press = 0;
volatile unsigned int last_nav_action_press = 0;

const unsigned int DEBOUNCE_DELAY = 100;

const int ITERATION_SPEED_PIN = 34;
const int ITERATE_BUTTON_PIN = 4;
const int CLEAR_BUTTON_PIN = 33;
const int PLAY_PAUSE_BUTTON_PIN = 32;
const int NAV_UP_PIN = 23;
const int NAV_LEFT_PIN = 16;
const int NAV_RIGHT_PIN = 21;
const int NAV_DOWN_PIN = 18;
const int NAV_ACTION_PIN = 22;
const int ROTARY_ENCODER_A_PIN = 17;
const int ROTARY_ENCODER_B_PIN = 19;
const int HIGH_SIDE_SERIAL_PIN = 15;
const int HIGH_SIDE_RCLK_PIN = 13;
const int HIGH_SIDE_SRCLK_PIN = 14;
const int LOW_SIDE_SERIAL_PIN = 25;
const int LOW_SIDE_CLK_PIN = 26;
const int LOW_SIDE_LATCH_PIN = 27;

Simulation simulation;
LedDriver led_driver{display_buffer_ptr};
NavigationDriver nav_driver{display_buffer_ptr, sim_buffer_ptr}; 

SemaphoreHandle_t buffer_mutex;

void setup() {
  // ------------- PIN DEFINITIONS -------------
  pinMode(ITERATION_SPEED_PIN, INPUT); // Iteration speed
  pinMode(ITERATE_BUTTON_PIN, INPUT_PULLUP); // Iterate
  pinMode(CLEAR_BUTTON_PIN, INPUT_PULLUP); // Clear
  pinMode(PLAY_PAUSE_BUTTON_PIN, INPUT_PULLUP); // Play/Pause

  pinMode(NAV_UP_PIN, INPUT_PULLUP); // Nav up
  pinMode(NAV_LEFT_PIN, INPUT_PULLUP); // Nav left
  pinMode(NAV_RIGHT_PIN, INPUT_PULLUP); // Nav right
  pinMode(NAV_ACTION_PIN, INPUT_PULLUP); // Nav action
  pinMode(NAV_DOWN_PIN, INPUT_PULLUP); // Nav down
  pinMode(ROTARY_ENCODER_A_PIN, INPUT_PULLUP); // Rotary encoder channel A
  pinMode(ROTARY_ENCODER_B_PIN, INPUT_PULLUP); // Rotary encoder channel B

  pinMode(HIGH_SIDE_SERIAL_PIN, OUTPUT); // Shift register serial
  pinMode(HIGH_SIDE_RCLK_PIN, OUTPUT); // RCLK
  pinMode(HIGH_SIDE_SRCLK_PIN, OUTPUT); // SRCLK

  buffer_mutex = xSemaphoreCreateMutex();

  Serial.begin(9600);

  // ------------- LED DRIVER TASK -------------
  xTaskCreatePinnedToCore(
    matrix_refresh,
    "LedDriver",
    10000,
    NULL,
    3, // Do not exceed priority of ~20 as core functions (Wi-Fi, BT, scheduling) operate at these priorities 
    NULL,
    0
  );
  xTaskCreatePinnedToCore(
    simulation_loop,
    "SimulationLoop",
    10000,
    NULL,
    1,
    NULL,
    1
  );
}

void loop() {
}

// put function definitions here:

int get_iteration_speed() {
    int raw_potentiometer_output = analogRead(ITERATION_SPEED_PIN);
    int iteration_delay = map(raw_potentiometer_output, 0, 4095, 0, 1000);
    return iteration_delay;
}

static void matrix_refresh(void* pvParameters) {
  for (;;) {
    // Ensure that buffers are not currently being used by the simulation loop
    if (xSemaphoreTake(buffer_mutex, (TickType_t) 10) == pdTRUE) {
      nav_driver.refresh_cursor_position();
      led_driver.refresh();
      xSemaphoreGive(buffer_mutex);
    }
    // Prevent task watchdog timer panics
    vTaskDelay(pdMS_TO_TICKS(1)); 
  }
}

static void simulation_loop(void* pvParameters) {
  attachInterrupt(digitalPinToInterrupt(ITERATE_BUTTON_PIN), iterate_isr, RISING);
  attachInterrupt(digitalPinToInterrupt(CLEAR_BUTTON_PIN), clear_isr, RISING);
  attachInterrupt(digitalPinToInterrupt(PLAY_PAUSE_BUTTON_PIN), play_pause_isr, RISING);
  attachInterrupt(digitalPinToInterrupt(NAV_UP_PIN), nav_up, RISING);
  attachInterrupt(digitalPinToInterrupt(NAV_RIGHT_PIN), nav_right, RISING);
  attachInterrupt(digitalPinToInterrupt(NAV_DOWN_PIN), nav_down, RISING);
  attachInterrupt(digitalPinToInterrupt(NAV_LEFT_PIN), nav_left, RISING);
  attachInterrupt(digitalPinToInterrupt(NAV_ACTION_PIN), nav_act, RISING);

  for (;;) {
    unsigned int current_time = millis();
    if (current_time - last_speed_check >= 100) {
      last_speed_check = current_time;
      iteration_delay = get_iteration_speed();
    }

    if (current_time - last_iteration >= iteration_delay && !paused) {
      last_iteration = current_time;
      simulation.iterate(display_buffer_ptr, sim_buffer_ptr);
      iterate = false;
    }

    if (cleared) {
      clear_buffers();
      cleared = false;
    }

    if (paused && iterate) {
      simulation.iterate(display_buffer_ptr, sim_buffer_ptr);
      iterate = false;
    }

    if (xSemaphoreTake(buffer_mutex, portMAX_DELAY) == pdTRUE) {
      bool (*temp)[16] = display_buffer_ptr;
      display_buffer_ptr = sim_buffer_ptr;
      sim_buffer_ptr = temp;
      xSemaphoreGive(buffer_mutex);
    }

    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

void clear_buffers() {
  if (xSemaphoreTake(buffer_mutex, portMAX_DELAY) == pdTRUE) {
    reset_buffer(sim_buffer_ptr);
    reset_buffer(display_buffer_ptr);
    xSemaphoreGive(buffer_mutex);
  }
}

void reset_buffer(bool (*buffer)[16]) {
  for (int i = 0; i < 16; i++) {
    for (int j = 0; j < 16; j++) {
      buffer[i][j] = false;
    }
  }
}

void IRAM_ATTR play_pause_isr() {
  unsigned int current_time = millis();
  if (current_time - last_play_pause_press >= DEBOUNCE_DELAY) {
    if (paused) {
      paused = false;
    } else {
      paused = true;
    }
    last_play_pause_press = current_time;
  }
}

void IRAM_ATTR clear_isr() {
  unsigned int current_time = millis();
  if (current_time - last_cleared_press >= DEBOUNCE_DELAY) {
    cleared = true;
    last_cleared_press = current_time;
  }
}

void IRAM_ATTR iterate_isr() {
  unsigned int current_time = millis();
  if (current_time - last_iterate_press >= DEBOUNCE_DELAY) {
    iterate = true;
    last_iterate_press = current_time;
  }
}

void IRAM_ATTR nav_up() {
  unsigned int current_time = millis();
  if (current_time - last_nav_up_press >= DEBOUNCE_DELAY) {
    nav_driver.move_up();
    last_nav_up_press = current_time;
  }
}

void IRAM_ATTR nav_right() {
  unsigned int current_time = millis();
  if (current_time - last_nav_right_press >= DEBOUNCE_DELAY) {
    nav_driver.move_right();
    last_nav_right_press = current_time;
  }
}

void IRAM_ATTR nav_down() {
  unsigned int current_time = millis();
  if (current_time - last_nav_down_press >= DEBOUNCE_DELAY) {
    nav_driver.move_down();
    last_nav_down_press = current_time;
  }
}

void IRAM_ATTR nav_left() {
  unsigned int current_time = millis();
  if (current_time - last_nav_left_press >= DEBOUNCE_DELAY) {
    nav_driver.move_left();
    last_nav_left_press = current_time;
  }
}

void IRAM_ATTR nav_act() {
  unsigned int current_time = millis();
  if (current_time - last_nav_action_press >= DEBOUNCE_DELAY) {
    nav_driver.centre_select();
    last_nav_action_press = current_time;
  }
}