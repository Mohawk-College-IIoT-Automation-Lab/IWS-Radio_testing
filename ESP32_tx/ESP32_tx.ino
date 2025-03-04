#include <Arduino.h>
#include "Helper.h"

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  while(!Serial) { }; // wait for serial monitor to connect

  DEBUG_PRINTLN("Started Serial & debug");

  DEBUG_PRINTLN("Trying E22 setup");
  setupE22();

  DEBUG_PRINT("Packet Msgs: "); DEBUG_PRINT(PACKET_MSG_SIZE_B); DEBUG_PRINT(" Message: "); DEBUG_PRINT(MESSAGE_SIZE_B); DEBUG_PRINT(" PackData: "); DEBUG_PRINTLN(PACKETDATA_SIZE_B);
  DEBUG_PRINT("Msg Count: "); DEBUG_PRINT(MESSAGE_COUNT); DEBUG_PRINT(" Padd: "); DEBUG_PRINTLN(PADD_SIZE_B);
  DEBUG_PRINT("Packet: "); DEBUG_PRINTLN(sizeof(Packet));
  DEBUG_PRINT("TX INT: "); DEBUG_PRINT(TX_INTERVAL); DEBUG_PRINT(" Sensor Int: "); DEBUG_PRINTLN(SENSOR_INTERVAL);

  DEBUG_PRINTLN("Trying Task Setup");
  setupTasks();
}

void loop() {
  // put your main code here, to run repeatedly:
  vTaskDelay(10);
}

