#ifndef HELPER_H
#define HELPER_H

#define _TASK_PRIORITY
#define _TASK_SLEEP_ON_IDLE_RUN

#define LoRa_E22_DEBUG

#include <LoRa_E22.h>

#define DATA_BUFFER_SIZE 255

#define E22_AUX 4
#define E22_M0 3
#define E22_M1 5

#define E22_RSSI true

#define E22_CONFIG_ADDH 0x00
#define E22_CONFIG_ADDL 0x03
#define E22_CONFIG_NETID 0x00
#define E22_CONFIG_CHAN 0x04

#define E22_DEST_ADDH 0x00
#define E22_DEST_ADDL 0x03

typedef struct _Message{
  float temperature;
  float humidity;
}Message;



#endif