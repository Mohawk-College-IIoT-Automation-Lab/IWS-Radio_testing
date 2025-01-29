#include <Arduino.h>
#include "Helper.h"

#define TX_METHOD 2

uint8_t setupE22();

LoRa_E22 e22ttl(&Serial2, E22_AUX, E22_M0, E22_M1, UART_BPS_RATE_9600 );

ResponseStructContainer rsc;
ResponseContainer rc;
Configuration *config;
ResponseStatus rs;

#if TX_METHOD == 1
Message temp_message;

#elif TX_METHOD == 2
Message buffer[DATA_BUFFER_SIZE];

#endif

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
    DEBUG_PRINTLN("Data available from e22");
 
    DEBUG_PRINT("Tx mode: "); DEBUG_PRINTLN(TX_METHOD);

 #if TX_METHOD == 1

    rc = e22ttl.receiveMessage();

    DEBUG_PRINT("rc code: "); DEBUG_PRINTLN(rc.status.code);
    DEBUG_PRINT("Data Rx: "); DEBUG_PRINTLN(rc.data.length());

    if(rc.status.code == 1){
      rc.data.getBytes((unsigned char*)&temp_message, 8);
      Serial.printf("Received data: T %f , H %f \n", temp_message.temperature, temp_message.humidity);
    }
    else{
      DEBUG_PRINT("Error receiving data -> "); DEBUG_PRINTLN(rc.status.getResponseDescription());
    }


#elif TX_METHOD == 2

    rc = e22ttl.receiveMessageRSSI();

    DEBUG_PRINT("rc code: "); DEBUG_PRINTLN(rc.status.code);
    DEBUG_PRINT("Data Rx: "); DEBUG_PRINTLN(rc.data.length());
    DEBUG_PRINT("RSSI: "); DEBUG_PRINTLN(rc.rssi);

    if(rc.status.code == 1){
      rc.data.getBytes((unsigned char*)&buffer, DATA_BUFFER_SIZE);
      Serial.println("-----   -----   -----");
      Serial.printf("Rx %d message\n", rc.data.length() / sizeof(Message));
      Serial.printf("   [0] t-> %f h-> %f \n", buffer[0].temperature, buffer[0].humidity);
      Serial.printf("   [1] t-> %f h-> %f \n", buffer[1].temperature, buffer[1].humidity);
      Serial.printf("   [2] t-> %f h-> %f \n", buffer[2].temperature, buffer[2].humidity);
      Serial.println("-----   -----   -----");
    }
    else{
      DEBUG_PRINT("Error receiving data -> "); DEBUG_PRINTLN(rc.status.getResponseDescription());
    }

#endif
  }

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

