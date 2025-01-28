
#include <Arduino.h>
#include "Helper.h"

#define TX_METHOD 1 // 1 - indv packets , 2 - one big data packet

void sensorCallback();
void txCallback();

void switch_data_buffer();
void wipe_data_buffer(Message * buffer);

void setupDHT();
void setupTasks();
void setupE22();

Scheduler runner;

Task sensorTask(SENSOR_INT, TASK_FOREVER, &sensorCallback);
Task txTask(TX_INT, TASK_FOREVER, &txCallback);

Message data_buffer[2][DATA_BUFFER_SIZE];
Message *reading, *writing;
uint8_t buff_index = 0;
uint8_t writing_index = 0;
uint8_t reading_index = 0;

DHT dht(DHT_PIN, DHT_TYPE);

LoRa_E22 e22ttl(&Serial2, E22_AUX, E22_M0, E22_M1);

ResponseStructContainer rsc;
Configuration config;
ResponseStatus rs;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  delay(250);
  DEBUG_PRINTLN("Started Serial & debug");

  DEBUG_PRINTLN("Trying DHT setup");
  setupDHT();

  DEBUG_PRINTLN("Trying E22 setup");
  setupE22();

  DEBUG_PRINTLN("Trying task setup");
  setupTasks();

}

void loop() {
  runner.execute();

}

void sensorCallback(){
  if(writing_index > DATA_BUFFER_SIZE)
    return;
  
  writing[writing_index].temperature = dht.readTemperature();
  writing[writing_index].humidity = dht.readHumidity();

  writing_index++;
  DEBUG_PRINT("Read T: "); DEBUG_PRINT(writing[writing_index].temperature); DEBUG_PRINT(" , "); DEBUG_PRINTLN(writing[writing_index].humidity);
}

void txCallback(){
  if(!reading_index)
    return;

  DEBUG_PRINT("Starting tx, sending: "); DEBUG_PRINT(reading_index); DEBUG_PRINTLN(" packets");

#if TX_METHOD == 1

  DEBUG_PRINTLN("Using tx method 1");

  for(int i = 0; i < reading_index; i++){
    DEBUG_PRINT("SENDING T: "); DEBUG_PRINT(reading[i].temperature); DEBUG_PRINT(" , "); DEBUG_PRINTLN(raeding[i].humidity);
    rs = e22ttl.sendFixedMessage(E22_DEST_ADDH, E22_DEST_ADDL, E22_CONFIG_CHAN, (const void*)&reading[i], sizeof(Message));

    if(rs.code != E22_SUCCESS){
      DEBUG_PRINTLN("E22 failed to send message");
      return;
    }

    DEBUG_PRINT("Message "); DEBUG_PRINT(i); DEBUG_PRINTLN(" successful");

  }

#elif TX_METHOD == 2
  DEBUG_PRINTLN("Using 
  tx method 2");
  rs = e22ttl.sendFixedMessage(E22_DEST_ADDH, E22_DEST_ADDL, E22_CONFIG_CHAN, (const void*)reading, sizeof(Message) * reading_index);

  if(rs.code != E22_SUCCESS){
    DEBUG_PRINTLN("E22 failed to send message");
    return;
  }

  DEBUG_PRINTLN("All data sent correctly");

#endif
  
  DEBUG_PRINTLN("Wiping reading buffer");
  wipe_buffer(reading);

  DEBUG_PRINTLN("Switching data buffers");
  switch_data_buffer();
}


void switch_data_buffer(){
  buff_index = !buff_index;
  reading = (Message*)&data_buffer[buff_index];
  writing = (Message*)&data_buffer[!buff_index];

  reading_index = writing_index;
  writing_index = 0;
}

void wipe_buffer(Message * buffer){ memset(buffer, 0, sizeof(Message) * DATA_BUFFER_SIZE); }

void setupDHT(){ 
  dht.begin(); 
  DEBUG_PRINTLN("DHT22 Setup");
}

void setupTasks(){
  runner.init();
  DEBUG_PRINTLN("runner init");

  runner.addTask(txTask);
  runner.addTask(sensorTask);
  DEBUG_PRINTLN("Tasks added");

  runner.enableAll();
  DEBUG_PRINTLN("Tasks enabled");
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

  DEBUG_PRINTLN(rsc.status.getResponseDescription());
  DEBUG_PRINTLN(rsc.status.code);
  
  DEBUG_PRINTLN("----------------------------------------");

  DEBUG_PRINT(F("HEAD : "));  DEBUG_PRINT(config.COMMAND, HEX);DEBUG_PRINT(" ");DEBUG_PRINT(config.STARTING_ADDRESS, HEX);DEBUG_PRINT(" ");DEBUG_PRINTLN(config.LENGHT, HEX);
  DEBUG_PRINTLN(F(" "));
  DEBUG_PRINT(F("AddH : "));  DEBUG_PRINTLN(config.ADDH, HEX);
  DEBUG_PRINT(F("AddL : "));  DEBUG_PRINTLN(config.ADDL, HEX);
  DEBUG_PRINT(F("NetID : "));  DEBUG_PRINTLN(config.NETID, HEX);
  DEBUG_PRINTLN(F(" "));
  DEBUG_PRINT(F("Chan : "));  DEBUG_PRINT(config.CHAN, DEC); DEBUG_PRINT(" -> "); DEBUG_PRINTLN(config.getChannelDescription());
  DEBUG_PRINTLN(F(" "));
  DEBUG_PRINT(F("SpeedParityBit     : "));  DEBUG_PRINT(config.SPED.uartParity, BIN);DEBUG_PRINT(" -> "); DEBUG_PRINTLN(config.SPED.getUARTParityDescription());
  DEBUG_PRINT(F("SpeedUARTDatte     : "));  DEBUG_PRINT(config.SPED.uartBaudRate, BIN);DEBUG_PRINT(" -> "); DEBUG_PRINTLN(config.SPED.getUARTBaudRateDescription());
  DEBUG_PRINT(F("SpeedAirDataRate   : "));  DEBUG_PRINT(config.SPED.airDataRate, BIN);DEBUG_PRINT(" -> "); DEBUG_PRINTLN(config.SPED.getAirDataRateDescription());
  DEBUG_PRINTLN(F(" "));
  DEBUG_PRINT(F("OptionSubPacketSett: "));  DEBUG_PRINT(config.OPTION.subPacketSetting, BIN);DEBUG_PRINT(" -> "); DEBUG_PRINTLN(config.OPTION.getSubPacketSetting());
  DEBUG_PRINT(F("OptionTranPower    : "));  DEBUG_PRINT(config.OPTION.transmissionPower, BIN);DEBUG_PRINT(" -> "); DEBUG_PRINTLN(config.OPTION.getTransmissionPowerDescription());
  DEBUG_PRINT(F("OptionRSSIAmbientNo: "));  DEBUG_PRINT(config.OPTION.RSSIAmbientNoise, BIN);DEBUG_PRINT(" -> "); DEBUG_PRINTLN(config.OPTION.getRSSIAmbientNoiseEnable());
  DEBUG_PRINTLN(F(" "));
  DEBUG_PRINT(F("TransModeWORPeriod : "));  DEBUG_PRINT(config.TRANSMISSION_MODE.WORPeriod, BIN);DEBUG_PRINT(" -> "); DEBUG_PRINTLN(config.TRANSMISSION_MODE.getWORPeriodByParamsDescription());
  DEBUG_PRINT(F("TransModeTransContr: "));  DEBUG_PRINT(config.TRANSMISSION_MODE.WORTransceiverControl, BIN);DEBUG_PRINT(" -> "); DEBUG_PRINTLN(config.TRANSMISSION_MODE.getWORTransceiverControlDescription());
  DEBUG_PRINT(F("TransModeEnableLBT : "));  DEBUG_PRINT(config.TRANSMISSION_MODE.enableLBT, BIN);DEBUG_PRINT(" -> "); DEBUG_PRINTLN(config.TRANSMISSION_MODE.getLBTEnableByteDescription());
  DEBUG_PRINT(F("TransModeEnableRSSI: "));  DEBUG_PRINT(config.TRANSMISSION_MODE.enableRSSI, BIN);DEBUG_PRINT(" -> "); DEBUG_PRINTLN(config.TRANSMISSION_MODE.getRSSIEnableByteDescription());
  DEBUG_PRINT(F("TransModeEnabRepeat: "));  DEBUG_PRINT(config.TRANSMISSION_MODE.enableRepeater, BIN);DEBUG_PRINT(" -> "); DEBUG_PRINTLN(config.TRANSMISSION_MODE.getRepeaterModeEnableByteDescription());
  DEBUG_PRINT(F("TransModeFixedTrans: "));  DEBUG_PRINT(config.TRANSMISSION_MODE.fixedTransmission, BIN);DEBUG_PRINT(" -> "); DEBUG_PRINTLN(config.TRANSMISSION_MODE.getFixedTransmissionDescription());

  DEBUG_PRINTLN("----------------------------------------");

  rsc.close();

}

