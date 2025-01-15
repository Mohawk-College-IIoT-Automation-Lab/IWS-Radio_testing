#include <LoRa_E22.h>
#include <SoftwareSerial.h>

/* --- EByte E22 --- */

#define E22_TX 12
#define E22_RX 14
#define E22_AUX 4
#define E22_M0 13
#define E22_M1 5

#define E22_RSSI true

#define E22_CONFIG_ADDH 0x00
#define E22_CONFIG_ADDL 0x03
#define E22_CONFIG_NETID 0x00
#define E22_CONFIG_CHAN 0x04

void setupE22();

LoRa_E22 e22ttl(&Serial1, E22_AUX, E22_M0, E22_M1);

ResponseStructContainer rsc;
ResponseContainer rc;
Configuration config;
ResponseStatus rs;

typedef struct _Message{
  float temperature;
  float humidity;
}Message;

/* --- END EByte E22 --- */

Message *msg, msgBuffer[255];
uint8_t bufferIndex = 0;
uint8_t msgLength = 0, totalMsgs = 0;

void setup() {

  Serial.begin(9600);
  delay(500);
  Serial.println("Beginning setup");

  setupE22();
  Serial.println("Setup E22");

}

void loop(){
  if(e22ttl.available()){
    rc = e22ttl.receiveMessageRSSI();
    if(rc.status.code == 1){
      msgLength = rc.data.length();
      totalMsgs = msgLength / sizeof(Message);

      if(!totalMsgs){
        Serial.printf("Received less data than expected \n");
        return;
      }
      memcpy(msgBuffer, rc.data.c_str(), msgLength);
      Serial.printf("Rx'ed %d messages with RSSI: %d \n", totalMsgs, rc.rssi);

      for(int i = 0 ; i < totalMsgs; i++)
        Serial.printf("Msg %d : T -> %f, H -> %f \n", i, msgBuffer[i].temperature, msgBuffer[i].humidity);

    }
  }
  else
    Serial.printf("Error receiving msg: %s \n", rc.status.getResponseDescription());

  memset(msgBuffer, 0, msgLength);
  msgLength = 0;
  totalMsgs = 0;
}



void setupE22(){
  e22ttl.begin(); // begin the e22

  
  rsc = e22ttl.getConfiguration(); // get the current config from the E22
  config = *(Configuration*)rsc.data; // extract the config data

  config.ADDH = E22_CONFIG_ADDH;
  config.ADDL = E22_CONFIG_ADDL;
  config.NETID = E22_CONFIG_NETID;
  config.CHAN = E22_CONFIG_CHAN;

  /* Default SPED settings
  config.SPED.uartBaudRate = UART_BPS_9600;
  config.SPED.airDataRate = AIR_DATA_RATE_010_24;
  config.SPED.uartParity = MODE_00_8N1;
  */

  /* Default OPTION settings
  config.OPTION.subPacketSetting = SPS_240_00;
  config.OPTION.RSSIAmbientNoise = RSSI_AMBIENT_NOISE_DISABLED;
  config.OPTION.transmissionPower = POWER_22;
  */

  config.TRANSMISSION_MODE.enableRSSI = RSSI_ENABLED;
  config.TRANSMISSION_MODE.fixedTransmission = FT_FIXED_TRANSMISSION;
  config.TRANSMISSION_MODE.enableRepeater = REPEATER_DISABLED; 
  config.TRANSMISSION_MODE.enableLBT = LBT_DISABLED;
  config.TRANSMISSION_MODE.WORTransceiverControl = WOR_TRANSMITTER;
  config.TRANSMISSION_MODE.WORPeriod = WOR_2000_011;

  e22ttl.setConfiguration(config, WRITE_CFG_PWR_DWN_SAVE);

  Serial.println(rsc.status.getResponseDescription());
  Serial.println(rsc.status.code);
  
  Serial.println("----------------------------------------");

  Serial.print(F("HEAD : "));  Serial.print(config.COMMAND, HEX);Serial.print(" ");Serial.print(config.STARTING_ADDRESS, HEX);Serial.print(" ");Serial.println(config.LENGHT, HEX);
  Serial.println(F(" "));
  Serial.print(F("AddH : "));  Serial.println(config.ADDH, HEX);
  Serial.print(F("AddL : "));  Serial.println(config.ADDL, HEX);
  Serial.print(F("NetID : "));  Serial.println(config.NETID, HEX);
  Serial.println(F(" "));
  Serial.print(F("Chan : "));  Serial.print(config.CHAN, DEC); Serial.print(" -> "); Serial.println(config.getChannelDescription());
  Serial.println(F(" "));
  Serial.print(F("SpeedParityBit     : "));  Serial.print(config.SPED.uartParity, BIN);Serial.print(" -> "); Serial.println(config.SPED.getUARTParityDescription());
  Serial.print(F("SpeedUARTDatte     : "));  Serial.print(config.SPED.uartBaudRate, BIN);Serial.print(" -> "); Serial.println(config.SPED.getUARTBaudRateDescription());
  Serial.print(F("SpeedAirDataRate   : "));  Serial.print(config.SPED.airDataRate, BIN);Serial.print(" -> "); Serial.println(config.SPED.getAirDataRateDescription());
  Serial.println(F(" "));
  Serial.print(F("OptionSubPacketSett: "));  Serial.print(config.OPTION.subPacketSetting, BIN);Serial.print(" -> "); Serial.println(config.OPTION.getSubPacketSetting());
  Serial.print(F("OptionTranPower    : "));  Serial.print(config.OPTION.transmissionPower, BIN);Serial.print(" -> "); Serial.println(config.OPTION.getTransmissionPowerDescription());
  Serial.print(F("OptionRSSIAmbientNo: "));  Serial.print(config.OPTION.RSSIAmbientNoise, BIN);Serial.print(" -> "); Serial.println(config.OPTION.getRSSIAmbientNoiseEnable());
  Serial.println(F(" "));
  Serial.print(F("TransModeWORPeriod : "));  Serial.print(config.TRANSMISSION_MODE.WORPeriod, BIN);Serial.print(" -> "); Serial.println(config.TRANSMISSION_MODE.getWORPeriodByParamsDescription());
  Serial.print(F("TransModeTransContr: "));  Serial.print(config.TRANSMISSION_MODE.WORTransceiverControl, BIN);Serial.print(" -> "); Serial.println(config.TRANSMISSION_MODE.getWORTransceiverControlDescription());
  Serial.print(F("TransModeEnableLBT : "));  Serial.print(config.TRANSMISSION_MODE.enableLBT, BIN);Serial.print(" -> "); Serial.println(config.TRANSMISSION_MODE.getLBTEnableByteDescription());
  Serial.print(F("TransModeEnableRSSI: "));  Serial.print(config.TRANSMISSION_MODE.enableRSSI, BIN);Serial.print(" -> "); Serial.println(config.TRANSMISSION_MODE.getRSSIEnableByteDescription());
  Serial.print(F("TransModeEnabRepeat: "));  Serial.print(config.TRANSMISSION_MODE.enableRepeater, BIN);Serial.print(" -> "); Serial.println(config.TRANSMISSION_MODE.getRepeaterModeEnableByteDescription());
  Serial.print(F("TransModeFixedTrans: "));  Serial.print(config.TRANSMISSION_MODE.fixedTransmission, BIN);Serial.print(" -> "); Serial.println(config.TRANSMISSION_MODE.getFixedTransmissionDescription());

  Serial.println("----------------------------------------");

  rsc.close();
}