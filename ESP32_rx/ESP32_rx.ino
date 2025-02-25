#include <Arduino.h>
#include "Helper.h"

extern Packet rx_packet;
uint8_t rssi = 0;

void setup() {
  
  Serial.begin(9600);
  while(!Serial) { }; // wait for serial monitor to connect

  DEBUG_PRINTLN("Started Serial & debug");

  DEBUG_PRINTLN("Trying E22 setup");
  setupE22();

  DEBUG_PRINTLN("Trying Wifi setup");
  setupWiFi();

  DEBUG_PRINTLN("Trying MQTT setup");
  setupMqtt();

}

void loop() {
  // put your main code here, to run repeatedly
  rssi = receive_packet(&rx_packet);
  
  if(rssi < 0){
    DEBUG_PRINTLN("Failed to receive a packet");
    return;
  }

  packet_printer(rx_packet);

  mqttPublishData(&rx_packet, rssi);

  clear_packet_messages(&rx_packet);

}