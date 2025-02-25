#include <Arduino.h>
#include "Helper.h"

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  while(!Serial) { }; // wait for serial monitor to connect

  DEBUG_PRINTLN("Started Serial & debug");

  DEBUG_PRINTLN("Trying E22 setup");
  setupE22();

  DEBUG_PRINTLN("Trying Task Setup");
  setupTasks();
}

void loop() {
  // put your main code here, to run repeatedly:
  vTaskDelay(10);
}

