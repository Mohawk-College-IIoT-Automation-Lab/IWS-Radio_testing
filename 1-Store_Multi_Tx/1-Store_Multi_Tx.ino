#define ACTIVATE_SOFTWARE_SERIAL
#define _TASK_PRIORITY
#define _TASK_SLEEP_ON_IDLE_RUN

#include <TaskScheduler.h>
#include <LoRa_E22.h>
#include <DHT.h>
#include <SoftwareSerial.h>

#define DEBUG 1 

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
#define E22_TX 14
#define E22_RX 12

#define E22_RSSI true

#define E22_CONFIG_ADDH 0x00
#define E22_CONFIG_ADDL 0x02
#define E22_CONFIG_NETID 0x00
#define E22_CONFIG_CHAN 0x04

#define E22_DEST_ADDH 0x00
#define E22_DEST_ADDL 0x03

void setupE22();
LoRa_E22 e22ttl(E22_TX, E22_RX, E22_AUX, E22_M0, E22_M1 );

ResponseStructContainer rsc;
Configuration config;
ResponseStatus rs;

typedef struct _Message{
  float temperature;
  float humidity;
}Message;

/* --- END EByte E22 --- */

/* --- Task Scheduler --- */

#define SENSOR_INT 1000UL // 1 second   //60000UL // 1 minute
#define TX_INT 60000UL // 1 minute      // 3600000UL // 1 hour

Scheduler lpr, hpr; // low priorirty runner

void sensorCallback();
void txCallback();
void setupTaskScheduler();

Task sensorTask(SENSOR_INT, TASK_FOREVER, &sensorCallback);
Task txTask(TX_INT, TASK_FOREVER, &txCallback);

/* --- END Task Scheduler --- */

Message msgBuffer[255], msg;
char buff[8];
const uint16_t buffSize = 255 * sizeof(Message);
uint8_t bufferIndex = 0;

void setup() {
  ESP.wdtDisable();

#ifdef DEBUG
  Serial.begin(115200);
  delay(500);
  Serial.println("\n Beginning setup");
#endif

  setupDHT();
#ifdef DEBUG
  Serial.println("Setup DHT");
#endif
  delay(250);

  setupE22();
#ifdef DEBUG
  Serial.println("Setup E22");
#endif
  delay(250);

  setupTaskScheduler();
#ifdef DEBUG
  Serial.println("Setup Task Scheduler");
#endif
  delay(250);

// total setup delay ~1250 ms
}

void loop() {
  lpr.execute();
}

void sensorCallback(){
  msg.temperature = dht.readTemperature();
  msg.humidity = dht.readHumidity();

  msgBuffer[bufferIndex] = msg;
  bufferIndex++;
  Serial.printf("%d %f %f \n", bufferIndex, msg.temperature, msg.humidity);
  ESP.wdtFeed();
}

void txCallback(){
  if(!bufferIndex){
    return;
  }

  for(int i = 0; i < bufferIndex; i++){
    ESP.wdtFeed();
    
    Serial.printf("%f %f \n", msgBuffer[i].temperature, msgBuffer[i].humidity);
    rs = e22ttl.sendFixedMessage(E22_DEST_ADDH, E22_DEST_ADDL, E22_CONFIG_CHAN, (const void*)&msgBuffer[i], sizeof(Message));
    
    if(rs.code != E22_SUCCESS){
      return;
    }

    Serial.printf("Message: %d successful \n", i);

  }

  memset(&msgBuffer, 0, buffSize);
  bufferIndex = 0;

}

void setupTaskScheduler(){
  lpr.init(); // init the low priority task runner
  hpr.init();

  lpr.setHighPriorityScheduler(&hpr);

  lpr.addTask(sensorTask);
  hpr.addTask(txTask);

  sensorTask.enable();
  txTask.enable();
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
