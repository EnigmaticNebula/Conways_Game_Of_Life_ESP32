#include <Arduino.h>
#include <iostream>
#include <simulation.hpp>
#include <led_array_driver.hpp>

// put function declarations here:
double get_iteration_speed();
Simulation simulation;
LedDriver driver;

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

  // ------------- LED DRIVER TASK -------------
  TaskHandle_t array_driver;
  xTaskCreatePinnedToCore(
    driver.array_begin,
    "LedDriver",
    10000,
    NULL,
    20, // Do not exceed priority of ~20 as core functions (Wi-Fi, BT, scheduling) operate at these priorities 
    &array_driver,
    0
  );
}

void loop() {
  double iteration_delay = get_iteration_speed();
  simulation.iterate();
  bool** cell_states = simulation.get_cell_states();
  driver.update_cell_states(cell_states);
}

// put function definitions here:

double get_iteration_speed() {
    int raw_potentiometer_output = analogRead(34);
    double iteration_delay = raw_potentiometer_output / 1000;
    return iteration_delay;
}