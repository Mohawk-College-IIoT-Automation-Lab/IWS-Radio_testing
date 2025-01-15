#define _TASK_SLEEP_ON_IDLE_RUN
#define _TASK_PRIORITY

#include <TaskScheduler.h>
#include <LoRa_E22.h>
#include <DHT.h>
#include <EEPROM.h>

#define DEBUG

/* --- Sensor --- */

#define DHT_PIN 3
#define DHT_TYPE DHT22

void setupDHT();

DHT dht(DHT_PIN, DHT_TYPE);

/* --- END Sensor --- */

/* --- EByte E22 --- */

#define E22_AUX 13
#define E22_M0 5
#define E22_M1 4

#define E22_RSSI true

#define E22_CONFIG_ADDH 0x00
#define E22_CONFIG_ADDL 0x02
#define E22_CONFIG_NETID 0x00
#define E22_CONFIG_CHAN 0x04

#define E22_DEST_ADDH 0x00
#define E22_DEST_ADDL 0x03

void setupE22();

LoRa_E22 e22ttl(&Serial1, E22_AUX, E22_M0, E22_M1);

ResponseStructContainer rsc;
Configuration config;
ResponseStatus rs;

struct Message{
  float temperature;
  float humidity;
};

#define SENSOR_INT 60000UL
#define TX_INT 3600000UL

Scheduler lpr, hpr;

void sensorCallback();
void txCallback();
void setupTaskScheduler();

Task sensorTask(SENSOR_INT, TASK_FOREVER, &sensorCallback, &lpr);
Task txTask(TX_INT, TASK_FOREVER, &txCallback, &hpr);

/* --- END Task Scheduler --- */

Message msgBuffer[255], msg;
uint8_t bufferIndex = 0;


void setup() {

#ifdef DEBUG
  Serial.begin(9600);
  delay(500);
  Serial.println("Beginning setup");
#endif

  setupDHT();
#ifdef DEBUG
  Serial.println("Setup DHT");
#endif

  setupE22();
#ifdef DEBUG
  Serial.println("Setup E22");
#endif

  setupTaskScheduler();
#ifdef DEBUG
  Serial.println("Setup Task Scheduler");
#endif

}

void loop() {
  lpr.execute();
}

void sensorCallback(){
  msg.temperature = dht.readTemperature();
  msg.humidity = dht.readHumidity();

#ifdef DEBUG
  Serial.printf("T: %f, H: %f \n", msg.temperature, msg.humidity);
#endif

  msgBuffer[bufferIndex] = msg;
  bufferIndex++;
}


void txCallback(){
  if(!bufferIndex){

#ifdef DEBUG
    Serial.println("No data to send");
#endif

    return;
  }

#ifdef DEBUG
  Serial.printf("Tx %d messages", bufferIndex);
#endif

  rs = e22ttl.sendFixedMessage(E22_DEST_ADDH, E22_DEST_ADDL, E22_CONFIG_CHAN, msgBuffer, sizeof(Message) * bufferIndex);
    
  if(rs.code != E22_SUCCESS){
#ifdef DEBUG
    Serial.printf("Error code: %d -> %s", rs.code, getResponseDescriptionByParams(rs.code));
#endif
    return;
  }

  Serial.printf("Message successful");
  
  // clear the bufferIndex and Buffer
  memset(msgBuffer, 0, sizeof(Message)*bufferIndex);
  bufferIndex = 0;

}

void setupTaskScheduler(){
  lpr.init(); // init the low priority task runner
  lpr.setHighPriorityScheduler(&hpr); // add the high priority task runner

  lpr.enableAll(); // enables all tasks, both high and low priority
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

#ifdef DEBUG
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

#endif
  rsc.close();
}

void setupDHT(){
  dht.begin();
}