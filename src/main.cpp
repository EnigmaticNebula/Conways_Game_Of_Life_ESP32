#include <Arduino.h>
#include <iostream>
#include <simulation.hpp>
#include <led_array_driver.hpp>

// put function declarations here:
int get_iteration_speed();

bool sim_buffer[16][16];
bool array_buffer[16][16];

bool (*sim_buffer_ptr)[16] = sim_buffer;
bool (*array_buffer_ptr)[16] = array_buffer;

unsigned int last_iteration = 0;
unsigned int last_speed_check = 0;
int iteration_delay = 0;

Simulation simulation;
LedDriver driver{array_buffer_ptr};

SemaphoreHandle_t buffer_mutex;

void setup() {
  // ------------- PIN DEFINITIONS -------------
  pinMode(34, INPUT); // Iteration speed
  pinMode(32, INPUT); // Iterate
  pinMode(33, INPUT); // Clear
  pinMode(4, INPUT); // Play/Pause

  pinMode(23, INPUT); // Nav up
  pinMode(16, INPUT); // Nav left
  pinMode(21, INPUT); // Nav right
  pinMode(22, INPUT); // Nav action
  pinMode(18, INPUT); // Nav down
  pinMode(17, INPUT); // Rotary encoder channel A
  pinMode(19, INPUT); // Rotary encoder channel B

  pinMode(14, OUTPUT); // Shift register serial
  pinMode(27, OUTPUT); // RCLK
  pinMode(26, OUTPUT); // SRCLK
  pinMode(25, OUTPUT); // SRCLR

  buffer_mutex = xSemaphoreCreateMutex();

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
    int raw_potentiometer_output = analogRead(34);
    int iteration_delay = map(raw_potentiometer_output, 0, 4095, 0, 1000);
    return iteration_delay;
}

static void matrix_refresh(void* pvParameters) {
  for (;;) {

    // Ensure that buffers are not currently being used by the simulation loop
    if (xSemaphoreTake(buffer_mutex, (TickType_t) 10) == pdTRUE) {
      driver.refresh();
      xSemaphoreGive(buffer_mutex);
    }
    // Prevent task watchdog timer panics
    vTaskDelay(pdMS_TO_TICKS(1)); 
  }
}

static void simulation_loop(void* pvParameters) {
  for (;;) {
    unsigned int current_time = millis();
    if (current_time - last_speed_check >= 100) {
      last_speed_check = current_time;
      iteration_delay = get_iteration_speed();
    }

    if (current_time - last_iteration >= iteration_delay) {
      last_iteration = current_time;
      simulation.iterate(array_buffer_ptr, sim_buffer_ptr);
    }

    if (xSemaphoreTake(buffer_mutex, portMAX_DELAY) == pdTRUE) {
      bool (*temp)[16] = array_buffer_ptr;
      array_buffer_ptr = sim_buffer_ptr;
      sim_buffer_ptr = temp;
    }

    vTaskDelay(pdMS_TO_TICKS(1));
  }
}