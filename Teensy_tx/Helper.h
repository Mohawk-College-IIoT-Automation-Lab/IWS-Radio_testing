#ifndef HELPER_H
#define HELPER_H

#define _TASK_PRIORITY
#define _TASK_SLEEP_ON_IDLE_RUN

#define LoRa_E22_DEBUG

#include <DHT.h>
#include <LoRa_E22.h>
#include <TaskScheduler.h>

#define SENSOR_INT TASK_SECOND * 1// 1 sample per second 
#define TX_INT TASK_SECOND * 20 // 1 tx every 2 minute
#define FLOATS_PER_TX TX_INT / SENSOR_INT // 2m * 60smpl/m + 1 = 121

#define DATA_BUFFER_SIZE FLOATS_PER_TX

#define DHT_PIN 14 
#define DHT_TYPE DHT22

#define E22_AUX 3
#define E22_M0 4
#define E22_M1 5

#define E22_RSSI true

#define E22_CONFIG_ADDH 0x00
#define E22_CONFIG_ADDL 0x02
#define E22_CONFIG_NETID 0x00
#define E22_CONFIG_CHAN 0x04

#define E22_DEST_ADDH 0x00
#define E22_DEST_ADDL 0x03

typedef struct _Message{
  float temperature;
  float humidity;
}Message;

void buffer_dump(uint8_t *buffer, uint8_t length){
  DEBUG_PRINTLN("---- dump ----");
  for(int i = 0; i < length; i++){
    if(!(i%8))
      DEBUG_PRINTLN(" ");
      
    DEBUG_PRINT(buffer[i], HEX); 
    DEBUG_PRINT(" , ");
    
  }
  DEBUG_PRINTLN(" ");
}
#endif