#include <Arduino.h>
#include <iostream>
#include <algorithm>
#include <simulation.hpp>
#include <led_array_driver.hpp>
#include <navigation_driver.hpp>
#include <pin_definitions.hpp>
using namespace std;

// Function Declarations
int get_iteration_speed();
static void matrix_refresh(void* pvParameters);
static void simulation_loop(void* pvParameters);
void clear_sim_buffer();
void reset_buffer(bool (*buffer)[16]);
void IRAM_ATTR play_pause_isr();
void IRAM_ATTR clear_isr();
void IRAM_ATTR iterate_isr();
void IRAM_ATTR nav_up();
void IRAM_ATTR nav_right();
void IRAM_ATTR nav_down();
void IRAM_ATTR nav_left();
void IRAM_ATTR nav_act();
void IRAM_ATTR encoder_a_change();
void IRAM_ATTR encoder_b_change();

// Buffers
bool sim_buffer[16][16];
bool display_buffer[16][16];

bool (*sim_buffer_ptr)[16] = sim_buffer;
bool (*display_buffer_ptr)[16] = display_buffer;

// Inputs / Debounce timers
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
volatile unsigned int last_encoder_a_change = 0;
volatile unsigned int last_encoder_b_change = 0;
volatile unsigned int last_joystick_input = 0; // This variable is necessary to prevent joystick spam and filter out unnecessary joystick actions when moving left, right and down (excl. up)
volatile unsigned int last_joystick_act = 0; // For every joystick movement in any direction, the joystick action is triggered. For up, left and right, this happens after. For up, it happens before.

volatile unsigned int rotary_encoder_pos = 0;
volatile bool clockwise_rotation = false;
volatile bool anticlockwise_rotation = false;

const unsigned int BUTTON_DEBOUNCE_DELAY = 150;
const unsigned int JOYSTICK_DEBOUNCE_DELAY = 100;
const unsigned int JOYSTICK_ACTION_DELAY = 200; // Rejects joystick actions occuring within x ms of another

Simulation simulation{sim_buffer_ptr};
LedDriver led_driver{display_buffer_ptr};
NavigationDriver nav_driver{display_buffer_ptr, sim_buffer_ptr}; 

SemaphoreHandle_t buffer_mutex;

void setup() {
  // Pin definitions

  //-- Input pins
  pinMode(ITERATION_SPEED_PIN, INPUT);
  pinMode(ITERATE_BUTTON_PIN, INPUT_PULLUP); 
  pinMode(CLEAR_BUTTON_PIN, INPUT_PULLUP);
  pinMode(PLAY_PAUSE_BUTTON_PIN, INPUT_PULLUP); 

  //-- Joystick pins
  pinMode(NAV_UP_PIN, INPUT_PULLUP);
  pinMode(NAV_LEFT_PIN, INPUT_PULLUP);
  pinMode(NAV_RIGHT_PIN, INPUT_PULLUP); 
  pinMode(NAV_ACTION_PIN, INPUT_PULLUP); 
  pinMode(NAV_DOWN_PIN, INPUT_PULLUP); 
  pinMode(ROTARY_ENCODER_A_PIN, INPUT_PULLUP); 
  pinMode(ROTARY_ENCODER_B_PIN, INPUT_PULLUP); 

  //-- Array driver pins
  pinMode(HIGH_SIDE_SERIAL_PIN, OUTPUT); 
  pinMode(HIGH_SIDE_SRCLK_PIN, OUTPUT); 
  pinMode(HIGH_SIDE_RCLK_PIN, OUTPUT);
  pinMode(LOW_SIDE_CLK_PIN, OUTPUT);
  pinMode(LOW_SIDE_LATCH_PIN, OUTPUT);
  pinMode(LOW_SIDE_SERIAL_PIN, OUTPUT);

  buffer_mutex = xSemaphoreCreateMutex();

  Serial.begin(9600);


  for (int i = 0; i < 16; i++) {
    for (int j = 0; j < 16; j++) {
      sim_buffer[i][j] = false;
    }
  }
  // Tasks

  // Note: NEVER pin the matrix refresh task to core 0 
  xTaskCreatePinnedToCore(
    matrix_refresh,
    "LedDriver",
    10000,
    NULL,
    3, // Do not exceed priority of ~20 as core functions (Wi-Fi, BT, scheduling) operate at these priorities 
    NULL,
    1
  );

  xTaskCreatePinnedToCore(
    simulation_loop,
    "SimulationLoop",
    10000,
    NULL,
    0,
    NULL,
    0
  );
}

void loop() {
}

int get_iteration_speed() {
    int raw_potentiometer_output = analogRead(ITERATION_SPEED_PIN);
    int iteration_delay = map(raw_potentiometer_output, 0, 4095, 1000, 0);
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
    vTaskDelay(pdMS_TO_TICKS(2)); 
  }
}

static void simulation_loop(void* pvParameters) {
  // Input interrupts

  // Note that whether a pin gets pulled high (+3.3V) or low (GND) on input is what determines whether the interrupt should trigger on the falling or rising edge.
  // i.e. pulled high = rising edge, pulled low = falling edge
  attachInterrupt(digitalPinToInterrupt(ITERATE_BUTTON_PIN), iterate_isr, FALLING);
  attachInterrupt(digitalPinToInterrupt(CLEAR_BUTTON_PIN), clear_isr, FALLING);
  attachInterrupt(digitalPinToInterrupt(PLAY_PAUSE_BUTTON_PIN), play_pause_isr, FALLING);
  attachInterrupt(digitalPinToInterrupt(NAV_UP_PIN), nav_up, FALLING);
  attachInterrupt(digitalPinToInterrupt(NAV_RIGHT_PIN), nav_right, FALLING);
  attachInterrupt(digitalPinToInterrupt(NAV_DOWN_PIN), nav_down, FALLING);
  attachInterrupt(digitalPinToInterrupt(NAV_LEFT_PIN), nav_left, FALLING);
  attachInterrupt(digitalPinToInterrupt(NAV_ACTION_PIN), nav_act, FALLING); // Nav action must be triggered on rising edge so actions always occur after joystick movements
  attachInterrupt(digitalPinToInterrupt(ROTARY_ENCODER_A_PIN), encoder_a_change, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ROTARY_ENCODER_B_PIN), encoder_b_change, CHANGE);

  for (;;) {
    unsigned int current_time = millis();
    if (current_time - last_speed_check >= 100) {
      last_speed_check = current_time;
      iteration_delay = get_iteration_speed();
    }

    if (current_time - last_iteration >= iteration_delay && !paused) {
      last_iteration = current_time;
      simulation.iterate();
      iterate = false;
    }

    if (cleared) {
      clear_sim_buffer();
      cleared = false;
    }

    if (paused && iterate) {
      simulation.iterate();
      iterate = false;
    }

    if (xSemaphoreTake(buffer_mutex, portMAX_DELAY) == pdTRUE) {
      for (int i = 0; i < 16; i++) {
        memcpy(display_buffer_ptr, sim_buffer_ptr, 16*16*sizeof(bool));
      }
      
      xSemaphoreGive(buffer_mutex);
    }

    vTaskDelay(pdMS_TO_TICKS(1)); // Prevent task watchdog timer panics
  }
}

void clear_sim_buffer() {
  if (xSemaphoreTake(buffer_mutex, portMAX_DELAY) == pdTRUE) {
      for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 16; j++) {
          sim_buffer[i][j] = false;
        }
    }
    xSemaphoreGive(buffer_mutex);
  }
}

void IRAM_ATTR play_pause_isr() {
  unsigned int current_time = millis();
  if (current_time - last_play_pause_press >= BUTTON_DEBOUNCE_DELAY) {
    Serial.println("PLAY/PAUSE");
    if (paused) {
      paused = false;
      nav_driver.game_paused = false;
    } else {
      paused = true;
      nav_driver.game_paused = true;
    }
    last_play_pause_press = current_time;
  }
}

void IRAM_ATTR clear_isr() {
  unsigned int current_time = millis();
  if (current_time - last_cleared_press >= BUTTON_DEBOUNCE_DELAY) {
    Serial.println("CLEAR");
    cleared = true;
    last_cleared_press = current_time;
  }
}

void IRAM_ATTR iterate_isr() {
  unsigned int current_time = millis();
  if (current_time - last_iterate_press >= BUTTON_DEBOUNCE_DELAY) {
    Serial.println("ITERATE");
    iterate = true;
    last_iterate_press = current_time;
  }
}

void IRAM_ATTR nav_up() {
  unsigned int current_time = millis();
  if (current_time - last_nav_up_press >= JOYSTICK_DEBOUNCE_DELAY) {
    nav_driver.move_up();
    last_nav_up_press = current_time;
    last_joystick_input = current_time;
    Serial.println("UP");
  }
}

void IRAM_ATTR nav_right() {
  unsigned int current_time = millis();
  if (current_time - last_nav_right_press >= JOYSTICK_DEBOUNCE_DELAY) {
    nav_driver.move_right();
    last_nav_right_press = current_time;
    last_joystick_input = current_time;
  }
}

void IRAM_ATTR nav_down() {
  unsigned int current_time = millis();
  if (current_time - last_nav_down_press >= JOYSTICK_DEBOUNCE_DELAY) {
    nav_driver.move_down();
    last_nav_down_press = current_time;
    last_joystick_input = current_time;
  }
}

void IRAM_ATTR nav_left() {
  unsigned int current_time = millis();
  if (current_time - last_nav_left_press >= JOYSTICK_DEBOUNCE_DELAY) {
    nav_driver.move_left();
    last_nav_left_press = current_time;
    last_joystick_input = current_time;
  }
}

void IRAM_ATTR nav_act() {
  unsigned int current_time = millis();
  if (current_time - last_nav_action_press >= JOYSTICK_DEBOUNCE_DELAY) {
    nav_driver.centre_select();
    last_nav_action_press = current_time;
    Serial.println("ACT");
  }
}

void IRAM_ATTR encoder_a_change() {
  unsigned int current_time = millis();
  if (current_time - last_encoder_a_change >= JOYSTICK_DEBOUNCE_DELAY) {
    if (!anticlockwise_rotation) {
      clockwise_rotation = true;
      rotary_encoder_pos++;
    } else {
      anticlockwise_rotation = false;
    }
    last_encoder_a_change = current_time;
  }
}

void IRAM_ATTR encoder_b_change() {
  unsigned int current_time = millis();
  if (current_time - last_encoder_b_change >= JOYSTICK_DEBOUNCE_DELAY) {
    if (!clockwise_rotation) {
      anticlockwise_rotation = true;
      if (rotary_encoder_pos > 0) {
        rotary_encoder_pos--;
      }
    } else {
      clockwise_rotation = false;
    }
    last_encoder_b_change = current_time;
  }
}
