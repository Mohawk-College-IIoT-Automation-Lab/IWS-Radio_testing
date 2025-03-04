#ifndef HELPER_H
#define HELPER_H

#define LoRa_E22_DEBUG
#define INCLUDE_eTaskGetState

#include <Arduino.h>
#include <DHT.h>
#include <LoRa_E22.h>

#include <creds.h>
#include <WiFi.h>
#include <ArduinoMqttClient.h>

#define RECEIVER
#ifndef RECEIVER
  #define TRANSMITTER
#endif

#define MAX_PACKET_SIZE_B 239

#define E22_AUX 18 
#define E22_M0 21
#define E22_M1 19
#define E22_TX_PIN 17
#define E22_RX_PIN 16

#define E22_RSSI true

#define E22_DEST_ADDH 0x00
#define E22_DEST_ADDL 0x03

#define E22_CONFIG_ADDH 0x00
#define E22_CONFIG_ADDL_TX 0x02 // if we are tx
#define E22_CONFIG_ADDL_RX E22_DEST_ADDL // if we are rx
#define E22_CONFIG_NETID 0x00
#define E22_CONFIG_CHAN 0x04

typedef struct _Message{
	float temperature;
	float humidity;
	//uint8_t whatever;
}Message;

typedef struct _packet_data{
	uint32_t count;
	uint8_t _msg_index;
}PacketData;

#define MAX_PACKET_SIZE_B 240
#define MESSAGE_SIZE_B sizeof(Message)
#define PACKETDATA_SIZE_B sizeof(PacketData)
#define MESSAGE_COUNT (MAX_PACKET_SIZE_B - PACKETDATA_SIZE_B) / MESSAGE_SIZE_B // (240 - 8) / 8 = 19 messages // ceil so that we always round down and don't try and store data we can't send
#define PACKET_MSG_SIZE_B MESSAGE_SIZE_B * (MESSAGE_COUNT)
#define PADD_SIZE_B MAX_PACKET_SIZE_B - PACKET_MSG_SIZE_B - PACKETDATA_SIZE_B

typedef struct _Packet{
	PacketData packetData;
	Message messages[MESSAGE_COUNT];
	uint8_t padding[PADD_SIZE_B];
}Packet;

#define PACKET_SIZE_B sizeof(Packet)
#define BUFFER_SIZE PACKET_SIZE_B * 10

const long TX_INTERVAL = 1000 * 60 * 2; // 1000 ms * 60s * 2m = 2m in ms 
const long SENSOR_INTERVAL = ceil(TX_INTERVAL / MESSAGE_COUNT); 

const char mqtt_broker[] = "test.mosquitto.org";
int        mqtt_port     = 1883;
const char mqtt_topic[]  = "EPIC_E22/Rx_Packet";


WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);
TaskHandle_t  sensorTask = NULL, txTask = NULL;

LoRa_E22 e22ttl(&Serial2, E22_AUX, E22_M0, E22_M1, UART_BPS_RATE_9600 );

ResponseStructContainer rsc;
ResponseContainer rc;
Configuration *config;
ResponseStatus rs;

Packet rx_packet;
Packet tx_packet;
Message new_message;
char buffer[BUFFER_SIZE];


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

/**
*
*/
void packet_printer(Packet packet){
  DEBUG_PRINTLN("---- Message printer ----");
  /*for(int i = 0; i < length; i++){
    DEBUG_PRINT(i); DEBUG_PRINT(" C: "); DEBUG_PRINT(msgs[i].count); DEBUG_PRINT(" T: "); DEBUG_PRINTLN(msgs[i].temperature);
  }
  DEBUG_PRINTLN(" ");*/
}

void append_packet_message(Packet *packet, Message msg){
  memcpy((void *)&packet->messages[packet->packetData._msg_index], (const void*) &msg, MESSAGE_SIZE_B);
  packet->packetData._msg_index++;
  
}

uint8_t packet_full(Packet packet){ return packet.packetData._msg_index >= MESSAGE_COUNT; }

void clear_packet_messages(Packet *packet){
  memset((void *)&packet->messages, 0, PACKET_MSG_SIZE_B );
  packet->packetData._msg_index = 0;
}

void send_packet(Packet *packet){
  rs = e22ttl.sendFixedMessage(E22_DEST_ADDH, E22_CONFIG_ADDL_TX, E22_CONFIG_CHAN, (const void*)packet, PACKET_SIZE_B);
  
  if(rs.code != E22_SUCCESS){
    DEBUG_PRINTLN("E22 failed to send message");
    DEBUG_PRINTLN(rs.getResponseDescription());
    return;
  }
  else

  
  DEBUG_PRINTLN("All data sent correctly");
  
}

int receive_packet(Packet *packet){
  if(e22ttl.available())
    rc = e22ttl.receiveMessageRSSI();
  else{
    DEBUG_PRINTLN("No E22 data");
    return -1;
  }
  
  if(rc.status.code != E22_SUCCESS){
    DEBUG_PRINTLN("E22 failed to receive message");
    DEBUG_PRINTLN(rc.status.getResponseDescription());
    return -1;
  }

  memcpy((void *)packet, (const void *)&rc.data, PACKET_SIZE_B);

  return rc.rssi;
}

void mqttPublishData(Packet *packet, uint8_t rssi){
  memset((void *)&buffer, 0, BUFFER_SIZE);

  sprintf((char *)&buffer, "Packet Count: %ld , RSSI: %d\n", packet->packetData.count, rssi);

  for(int i = 0; i < packet->packetData._msg_index; i++)
    sprintf((char *)&buffer, "  c: %d, T: %f \n", i, packet->messages[i].temperature);

  mqttClient.beginMessage(mqtt_topic);
  mqttClient.print(buffer);
  mqttClient.endMessage();

  DEBUG_PRINTLN("MQTT Data Publishers: ")
  DEBUG_PRINT(buffer);

}

void sensor_task_code(void * params){
  DEBUG_PRINTLN("Starting Sensor Task");
  while(1){
    if(packet_full(tx_packet)){
      DEBUG_PRINTLN("Messages are maxed out");
    }
    else{
      new_message.temperature = (float)random(0, 40);
      new_message.humidity = (float)random(0, 100);
      DEBUG_PRINT("T: "); DEBUG_PRINT(new_message.temperature); DEBUG_PRINT("H: "); DEBUG_PRINTLN(new_message.humidity);
      
      DEBUG_PRINTLN("Appending packet")
      append_packet_message(&tx_packet, new_message);
    }

    DEBUG_PRINT("Delaying "); DEBUG_PRINTLN(SENSOR_INTERVAL);
    vTaskDelay(SENSOR_INTERVAL);
  }
  return;
}

void tx_task_code(void * params){
  DEBUG_PRINTLN("Starting Tx Task");
  DEBUG_PRINT("Delaying "); DEBUG_PRINTLN(TX_INTERVAL);
  vTaskDelay(TX_INTERVAL);

  while(1){
    DEBUG_PRINTLN("Tx'ing packet");
    send_packet(&tx_packet);
    
    tx_packet.packetData.count++;
    DEBUG_PRINTLN("Count increased")
    
    clear_packet_messages(&tx_packet);
    DEBUG_PRINTLN("Wiped packet");

    DEBUG_PRINT("Delaying "); DEBUG_PRINTLN(TX_INTERVAL);
    vTaskDelay(TX_INTERVAL);

  }
  return;
}

void setupWiFi(){
  DEBUG_PRINTLN("Starting WiFi with:");
  DEBUG_PRINT("SSID: "); DEBUG_PRINT(wifi_ssid); DEBUG_PRINT(" PASS: "); DEBUG_PRINTLN(wifi_password);
 
  do{
    DEBUG_PRINTLN("Could not connect to WiFi, trying again in 5s");
    delay(5000);
  }while(WiFi.status() != WL_CONNECTED);
 
  DEBUG_PRINTLN("WiFi Connected! ");
 
}

void setupMqtt(){
  DEBUG_PRINTLN("Starting MQTT Client");
  DEBUG_PRINT("BROKER: "); DEBUG_PRINT(mqtt_broker); DEBUG_PRINT(" PORT: "); DEBUG_PRINT(mqtt_port); DEBUG_PRINT(" TOPIC: "); DEBUG_PRINTLN(mqtt_topic);

  while(!mqttClient.connect(mqtt_broker, mqtt_port)){
    DEBUG_PRINTLN("Could not connect to broker, trying again in 5s");
    DEBUG_PRINTLN(mqttClient.connectError());
    delay(5000);
  }

  DEBUG_PRINTLN("Mqtt Broker connected! ");
}


uint8_t setupTasks(){
  DEBUG_PRINTLN("Tring to create sensor task");
  if(xTaskCreate(sensor_task_code, "Sensor Task", 4096, (void*) 1, 1, &sensorTask) != pdPASS){
    DEBUG_PRINTLN("Could not init sensor task");
    return 0;
  }

  DEBUG_PRINTLN("Tring to create tx task");
  if(xTaskCreate(tx_task_code, "Tx Task", 4096, (void*) 1, 2, &txTask) != pdPASS){
    DEBUG_PRINTLN("Could not init task");
    return 0;
  }

  DEBUG_PRINTLN("Tasks successfully started");
  return 1;
  
}


uint8_t setupE22(){
  Serial2.begin(9600, SERIAL_8N1, E22_RX_PIN, E22_TX_PIN);
  e22ttl.begin(); // begin the e22

  rsc = e22ttl.getConfiguration(); // get the current config from the E22
  config = (Configuration*)rsc.data; // extract the config data

  config->ADDH = E22_CONFIG_ADDH;
#ifdef TRANSMITTER
  config->ADDL = E22_CONFIG_ADDL_TX; // transmitter
#endif
#ifdef RECEIVER  
  config->ADDL = E22_CONFIG_ADDL_RX; // receiver
#endif
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

#ifdef TRANSMITTER
  config->TRANSMISSION_MODE.WORTransceiverControl = WOR_TRANSMITTER;
#endif
#ifdef RECEIVER  
  config->TRANSMISSION_MODE.WORTransceiverControl = WOR_RECEIVER;
#endif
  
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
