#include <Arduino.h>
#include "Helper.h"

void setup() {
  
  Serial.begin(9600);
  while(!Serial) { }; // wait for serial monitor to connect

  DEBUG_PRINTLN("Started Serial & debug");

  DEBUG_PRINTLN("Trying E22 setup");
  setupE22();
}

void loop() {
  // put your main code here, to run repeatedly

  if(e22ttl.available() > 0){
    /*DEBUG_PRINTLN("Data available from e22");
    if(receive_packetized_message((void*)&rx_buffer, RX_BUFFER_LENGTH)){
      DEBUG_PRINTLN("Data rx succcess");
      message_printer((Message*)&rx_buffer, RX_BUFFER_LENGTH);
    }
    else{
      DEBUG_PRINTLN("Data rx failure");
    }
    */
    
  }

}