#ifndef HELPER_H
#define HELPER_H

#define _TASK_PRIORITY
#define _TASK_SLEEP_ON_IDLE_RUN

#define LoRa_E22_DEBUG

#include <DHT.h>
#include <LoRa_E22.h>
#include <TaskScheduler.h>

#define DATA_BUFFER_SIZE_BYTES 65535
#define MESSAGE_SIZE 8
#define PACKET_SIZE 234
#define PACKET_HEADER_SIZE 2
#define PACKET_DATA_SIZE PACKET_SIZE - PACKET_HEADER_SIZE
#define RX_BUFFER_LENGTH DATA_BUFFER_SIZE_BYTES / MESSAGE_SIZE

#define E22_AUX 4
#define E22_M0 3
#define E22_M1 5

#define E22_RSSI true

#define E22_DEST_ADDH 0x00
#define E22_DEST_ADDL 0x03

#define E22_CONFIG_ADDH 0x00
//#define E22_CONFIG_ADDL 0x02 // if we are tx
#define E22_CONFIG_ADDL E22_DEST_ADDL // if we are rx
#define E22_CONFIG_NETID 0x00
#define E22_CONFIG_CHAN 0x04


typedef struct _Message{
  float temperature;
  float humidity;
}Message;

typedef struct _Packet{
  uint8_t num_following;
  void data[PACKET_DATA_SIZE];
}Packet

LoRa_E22 e22ttl(&Serial2, E22_AUX, E22_M0, E22_M1, UART_BPS_RATE_9600 );

ResponseStructContainer rsc;
ResponseContainer rc;
Configuration *config;
ResponseStatus rs;

Message rx_buffer[RX_BUFFER_LENGTH];
Packet rx_packet;

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

void message_printer(Message *msgs, uint16_t length){
  DEBUG_PRINTLN("---- Message printer ----");
  for(int i = 0; i < length; i++){
    DEBUG_PRINT(i); DEBUG_PRINT(" T: "); DEBUG_PRINT(msgs[i].temperature); DEBUG_PRINT(" H: "); DEBUG_PRINTLN(msgs[i].humidity);
  }
  DEBUG_PRINTLN(" ");
}

uint8_t send_packetized_message(Packet *packet, void *buffer, uint16_t size){
  // send the first packet,

  DEBUG_PRINT("Size (B): "); DEBUG_PRINTLN(size);

  
  DEBUG_PRIN("Num Packets "); DEBUG_PRINTLN(packet->num_following);

  for(uint16_t i = 0; i < size; i += PACKET_DATA_SIZE){
    
    packet->num_following = min( (size - i)  / PACKET_DATA_SIZE);

    if(packet->num_following > 0)
      memcpy((void*)packet->data, (const void*)buffer[i], PACKET_DATA_SIZE);
    else

  }

  return 0;
}

uint8_t receive_packetized_message(void *buffer, uint16_t size){
  // need to only fill buffer with as much data as it can fit, not how much is available
  // still need to make sure we rx the correct size of data packet
  // remove all padding
  return 0;
}


uint8_t setupE22(){
  Serial2.begin(9600);
  e22ttl.begin(); // begin the e22

  rsc = e22ttl.getConfiguration(); // get the current config from the E22
  config = (Configuration*)rsc.data; // extract the config data

  config->ADDH = E22_CONFIG_ADDH;
  config->ADDL = E22_CONFIG_ADDL;
  config->NETID = E22_CONFIG_NETID;
  config->CHAN = E22_CONFIG_CHAN;

  config->SPED.uartBaudRate = UART_BPS_9600;
  config->SPED.airDataRate = AIR_DATA_RATE_010_24;
  config->SPED.uartParity = MODE_00_8N1;

  config->OPTION.subPacketSetting = SPS_240_00;
  config->OPTION.RSSIAmbientNoise = RSSI_AMBIENT_NOISE_DISABLED;
  config->OPTION.transmissionPower = POWER_22;

  config->TRANSMISSION_MODE.enableRSSI = RSSI_ENABLED;
  config->TRANSMISSION_MODE.fixedTransmission = FT_FIXED_TRANSMISSION;
  config->TRANSMISSION_MODE.enableRepeater = REPEATER_DISABLED; 
  config->TRANSMISSION_MODE.enableLBT = LBT_DISABLED;
  config->TRANSMISSION_MODE.WORTransceiverControl = WOR_RECEIVER;
  config->TRANSMISSION_MODE.WORPeriod = WOR_2000_011;

  e22ttl.setConfiguration(*config, WRITE_CFG_PWR_DWN_SAVE);

  DEBUG_PRINTLN(rsc.status.getResponseDescription());
  DEBUG_PRINTLN(rsc.status.code);

  // read the config we just wrote
  rsc = e22ttl.getConfiguration(); // get the current config from the E22
  config = (Configuration*)rsc.data; // extract the config data

  DEBUG_PRINTLN("Re read the config from device");
  DEBUG_PRINTLN(rsc.status.getResponseDescription());
  DEBUG_PRINTLN(rsc.status.code);
  
  DEBUG_PRINTLN("----------------------------------------");

  DEBUG_PRINT(F("HEAD : "));  DEBUG_PRINT(config->COMMAND, HEX);DEBUG_PRINT(" ");DEBUG_PRINT(config->STARTING_ADDRESS, HEX);DEBUG_PRINT(" ");DEBUG_PRINTLN(config->LENGHT, HEX);
  DEBUG_PRINTLN(F(" "));
  DEBUG_PRINT(F("AddH : "));  DEBUG_PRINTLN(config->ADDH, HEX);
  DEBUG_PRINT(F("AddL : "));  DEBUG_PRINTLN(config->ADDL, HEX);
  DEBUG_PRINT(F("NetID : "));  DEBUG_PRINTLN(config->NETID, HEX);
  DEBUG_PRINTLN(F(" "));
  DEBUG_PRINT(F("Chan : "));  DEBUG_PRINT(config->CHAN, DEC); DEBUG_PRINT(" -> "); DEBUG_PRINTLN(config->getChannelDescription());
  DEBUG_PRINTLN(F(" "));
  DEBUG_PRINT(F("SpeedParityBit     : "));  DEBUG_PRINT(config->SPED.uartParity, BIN);DEBUG_PRINT(" -> "); DEBUG_PRINTLN(config->SPED.getUARTParityDescription());
  DEBUG_PRINT(F("SpeedUARTDatte     : "));  DEBUG_PRINT(config->SPED.uartBaudRate, BIN);DEBUG_PRINT(" -> "); DEBUG_PRINTLN(config->SPED.getUARTBaudRateDescription());
  DEBUG_PRINT(F("SpeedAirDataRate   : "));  DEBUG_PRINT(config->SPED.airDataRate, BIN);DEBUG_PRINT(" -> "); DEBUG_PRINTLN(config->SPED.getAirDataRateDescription());
  DEBUG_PRINTLN(F(" "));
  DEBUG_PRINT(F("OptionSubPacketSett: "));  DEBUG_PRINT(config->OPTION.subPacketSetting, BIN);DEBUG_PRINT(" -> "); DEBUG_PRINTLN(config->OPTION.getSubPacketSetting());
  DEBUG_PRINT(F("OptionTranPower    : "));  DEBUG_PRINT(config->OPTION.transmissionPower, BIN);DEBUG_PRINT(" -> "); DEBUG_PRINTLN(config->OPTION.getTransmissionPowerDescription());
  DEBUG_PRINT(F("OptionRSSIAmbientNo: "));  DEBUG_PRINT(config->OPTION.RSSIAmbientNoise, BIN);DEBUG_PRINT(" -> "); DEBUG_PRINTLN(config->OPTION.getRSSIAmbientNoiseEnable());
  DEBUG_PRINTLN(F(" "));
  DEBUG_PRINT(F("TransModeWORPeriod : "));  DEBUG_PRINT(config->TRANSMISSION_MODE.WORPeriod, BIN);DEBUG_PRINT(" -> "); DEBUG_PRINTLN(config->TRANSMISSION_MODE.getWORPeriodByParamsDescription());
  DEBUG_PRINT(F("TransModeTransContr: "));  DEBUG_PRINT(config->TRANSMISSION_MODE.WORTransceiverControl, BIN);DEBUG_PRINT(" -> "); DEBUG_PRINTLN(config->TRANSMISSION_MODE.getWORTransceiverControlDescription());
  DEBUG_PRINT(F("TransModeEnableLBT : "));  DEBUG_PRINT(config->TRANSMISSION_MODE.enableLBT, BIN);DEBUG_PRINT(" -> "); DEBUG_PRINTLN(config->TRANSMISSION_MODE.getLBTEnableByteDescription());
  DEBUG_PRINT(F("TransModeEnableRSSI: "));  DEBUG_PRINT(config->TRANSMISSION_MODE.enableRSSI, BIN);DEBUG_PRINT(" -> "); DEBUG_PRINTLN(config->TRANSMISSION_MODE.getRSSIEnableByteDescription());
  DEBUG_PRINT(F("TransModeEnabRepeat: "));  DEBUG_PRINT(config->TRANSMISSION_MODE.enableRepeater, BIN);DEBUG_PRINT(" -> "); DEBUG_PRINTLN(config->TRANSMISSION_MODE.getRepeaterModeEnableByteDescription());
  DEBUG_PRINT(F("TransModeFixedTrans: "));  DEBUG_PRINT(config->TRANSMISSION_MODE.fixedTransmission, BIN);DEBUG_PRINT(" -> "); DEBUG_PRINTLN(config->TRANSMISSION_MODE.getFixedTransmissionDescription());

  DEBUG_PRINTLN("----------------------------------------");

  rsc.close();

  return rsc.status.code != 1?0:1;

}

#endif