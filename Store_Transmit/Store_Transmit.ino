#include "Arduino.h"
#include "LoRa_E22.h"
#include "DHT.h"
#include <EEPROM.h>
#include <SoftwareSerial.h>

// ----------------------- USER CONFIG ---------------------------
#define DHTPIN 3 // GPIO2 - Pin D4 on ESP8266, adjust if needed
#define DHTTYPE DHT22
#define DESTINATION_ADDH 0
#define DESTINATION_ADDL 3
#define DESTINATION_CHAN 4

// Enable RSSI
#define ENABLE_RSSI true

// EEPROM CONFIG
// We'll store number_of_records at EEPROM addresses [0,1] (two bytes for safety) and 
// start data from address 2.
#define EEPROM_SIZE 512 // Enough to store many records
// Each record is a Message struct with 2 floats: 4 bytes each float = 8 bytes total
// If one reading every 10s and transmit every 60s, we store ~6 messages per cycle.
// But we want to accumulate on failure, so more space is given.

// Timing intervals (in ms)
#define READ_INTERVAL 60000UL     // 60 seconds
#define TRANSMIT_INTERVAL 3600000UL // 60 minutes

// ---------------------------------------------------------------

DHT dht(DHTPIN, DHTTYPE);

// LoRa pins based on your code
SoftwareSerial mySerial(14, 12);  // Arduino RX <-- e22 TX, Arduino TX --> e22 RX
LoRa_E22 e22ttl(&mySerial, 13, 5, 4);

struct Message {
  float temperature;
  float humidity;
};

// timing variables
unsigned long lastReadTime = 0;
unsigned long lastTransmitTime = 0;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(9600);
  delay(500);

  dht.begin();

  // Startup all pins and UART
  e22ttl.begin();

  // Configure E22 module as previously done
  ResponseStructContainer c;
  c = e22ttl.getConfiguration();
  Configuration configuration = *(Configuration*)c.data;
  configuration.ADDH = DESTINATION_ADDH;
  configuration.ADDL = 0x02; // local ADDL for sender
  configuration.NETID = 0x00;
  configuration.CHAN = DESTINATION_CHAN;

  configuration.SPED.uartBaudRate = UART_BPS_9600;
  configuration.SPED.airDataRate = AIR_DATA_RATE_010_24;
  configuration.SPED.uartParity = MODE_00_8N1;

  configuration.OPTION.subPacketSetting = SPS_240_00;
  configuration.OPTION.RSSIAmbientNoise = RSSI_AMBIENT_NOISE_DISABLED;
  configuration.OPTION.transmissionPower = POWER_22;

  configuration.TRANSMISSION_MODE.enableRSSI = RSSI_ENABLED;
  configuration.TRANSMISSION_MODE.fixedTransmission = FT_FIXED_TRANSMISSION;
  configuration.TRANSMISSION_MODE.enableRepeater = REPEATER_DISABLED;
  configuration.TRANSMISSION_MODE.enableLBT = LBT_DISABLED;
  configuration.TRANSMISSION_MODE.WORTransceiverControl = WOR_TRANSMITTER;
  configuration.TRANSMISSION_MODE.WORPeriod = WOR_2000_011;
  e22ttl.setConfiguration(configuration, WRITE_CFG_PWR_DWN_SAVE);

  Serial.println(c.status.getResponseDescription());
  Serial.println(c.status.code);

  printParameters(configuration);
  c.close();

  // Initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);

  // If it's the first run or not sure, can initialize stored count if needed
  // But we won't overwrite if there's existing data.
  // Check if no data count is set (unlikely first run)
  // It's safer not to reset here to allow retention after reset if needed.
  // If you want a clean start every reboot:
  writeDataCount(0);

  lastReadTime = millis();
  lastTransmitTime = millis();
}

void loop() {
  unsigned long currentMillis = millis();

  // Every 10 seconds: read and store data
  if (currentMillis - lastReadTime >= READ_INTERVAL) {
    lastReadTime = currentMillis;
    readAndStoreData();
  }

  // Every 60 seconds: try to transmit all data
  if (currentMillis - lastTransmitTime >= TRANSMIT_INTERVAL) {
    lastTransmitTime = currentMillis;
    transmitStoredData();
  }

  // Put your device to sleep if needed or handle other tasks
  // Removed deep sleep for now since it conflicts with regular intervals.
  // You can reintroduce deep sleep strategies if needed.
}

void readAndStoreData() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  Serial.print("Reading T: ");
  Serial.print(t);
  Serial.print(" H: ");
  Serial.println(h);

  // Store to EEPROM
  Message m;
  m.temperature = t;
  m.humidity = h;
  storeMessage(m);

  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
}

void transmitStoredData() {
  int count = readDataCount();
  if (count <= 0) {
    Serial.println("No data to transmit.");
    return;
  }

  Serial.print("Transmitting ");
  Serial.print(count);
  Serial.println(" messages...");

  // Read all messages
  Message* allData = new Message[count];
  for (int i = 0; i < count; i++) {
    allData[i] = readMessage(i);
  }

  // Attempt to send all data as a single payload or in chunks
  // The payload size might be large, consider splitting if needed.
  // E22 can handle certain sizes. Let's assume we can send it in multiple transmissions.
  // We'll send them one by one in this example.

  bool success = true;
  for (int i = 0; i < count; i++) {
    ResponseStatus rs = e22ttl.sendFixedMessage(0, DESTINATION_ADDL, DESTINATION_CHAN, &allData[i], sizeof(Message)); // sends 8B of data
    if (rs.code != 1) {
      // Transmission error
      Serial.print("Error transmitting message index ");
      Serial.println(i);
      success = false;
      break;
    } else {
      Serial.print("Message ");
      Serial.print(i);
      Serial.println(" transmitted successfully!");
    }
  }

  delete[] allData;

  if (success) {
    // Clear EEPROM if all sent
    Serial.println("All data transmitted successfully, clearing stored data.");
    writeDataCount(0);
    EEPROM.commit();
  } else {
    // If fail, do nothing special, data remains in EEPROM. Next cycle we'll retry plus new data.
    Serial.println("Transmission failed, will retry next cycle.");
  }
}

// Helper functions

int readDataCount() {
  int highByte = EEPROM.read(0);
  int lowByte = EEPROM.read(1);
  int count = (highByte << 8) | lowByte;
  return count;
}

void writeDataCount(int count) {
  EEPROM.write(0, (count >> 8) & 0xFF);
  EEPROM.write(1, count & 0xFF);
}

void storeMessage(Message m) {
  int count = readDataCount();
  // Write message at position count
  int startAddr = 2 + count * sizeof(Message);
  if (startAddr + sizeof(Message) > EEPROM_SIZE) {
    Serial.println("EEPROM Full! Cannot store more data.");
    return; // or you could implement a circular buffer
  }

  // Write the message bytes
  byte* p = (byte*)&m;
  for (int i = 0; i < (int)sizeof(Message); i++) {
    EEPROM.write(startAddr + i, p[i]);
  }

  // Increment count
  count++;
  writeDataCount(count);
  EEPROM.commit();

  Serial.print("Message stored, total count: ");
  Serial.println(count);
}

Message readMessage(int index) {
  Message m;
  int startAddr = 2 + index * sizeof(Message);
  byte* p = (byte*)&m;
  for (int i = 0; i < (int)sizeof(Message); i++) {
    p[i] = EEPROM.read(startAddr + i);
  }
  return m;
}

void printParameters(struct Configuration configuration) {
  Serial.println("----------------------------------------");

  Serial.print(F("HEAD : "));  Serial.print(configuration.COMMAND, HEX);Serial.print(" ");Serial.print(configuration.STARTING_ADDRESS, HEX);Serial.print(" ");Serial.println(configuration.LENGHT, HEX);
  Serial.println(F(" "));
  Serial.print(F("AddH : "));  Serial.println(configuration.ADDH, HEX);
  Serial.print(F("AddL : "));  Serial.println(configuration.ADDL, HEX);
  Serial.print(F("NetID : "));  Serial.println(configuration.NETID, HEX);
  Serial.println(F(" "));
  Serial.print(F("Chan : "));  Serial.print(configuration.CHAN, DEC); Serial.print(" -> "); Serial.println(configuration.getChannelDescription());
  Serial.println(F(" "));
  Serial.print(F("SpeedParityBit     : "));  Serial.print(configuration.SPED.uartParity, BIN);Serial.print(" -> "); Serial.println(configuration.SPED.getUARTParityDescription());
  Serial.print(F("SpeedUARTDatte     : "));  Serial.print(configuration.SPED.uartBaudRate, BIN);Serial.print(" -> "); Serial.println(configuration.SPED.getUARTBaudRateDescription());
  Serial.print(F("SpeedAirDataRate   : "));  Serial.print(configuration.SPED.airDataRate, BIN);Serial.print(" -> "); Serial.println(configuration.SPED.getAirDataRateDescription());
  Serial.println(F(" "));
  Serial.print(F("OptionSubPacketSett: "));  Serial.print(configuration.OPTION.subPacketSetting, BIN);Serial.print(" -> "); Serial.println(configuration.OPTION.getSubPacketSetting());
  Serial.print(F("OptionTranPower    : "));  Serial.print(configuration.OPTION.transmissionPower, BIN);Serial.print(" -> "); Serial.println(configuration.OPTION.getTransmissionPowerDescription());
  Serial.print(F("OptionRSSIAmbientNo: "));  Serial.print(configuration.OPTION.RSSIAmbientNoise, BIN);Serial.print(" -> "); Serial.println(configuration.OPTION.getRSSIAmbientNoiseEnable());
  Serial.println(F(" "));
  Serial.print(F("TransModeWORPeriod : "));  Serial.print(configuration.TRANSMISSION_MODE.WORPeriod, BIN);Serial.print(" -> "); Serial.println(configuration.TRANSMISSION_MODE.getWORPeriodByParamsDescription());
  Serial.print(F("TransModeTransContr: "));  Serial.print(configuration.TRANSMISSION_MODE.WORTransceiverControl, BIN);Serial.print(" -> "); Serial.println(configuration.TRANSMISSION_MODE.getWORTransceiverControlDescription());
  Serial.print(F("TransModeEnableLBT : "));  Serial.print(configuration.TRANSMISSION_MODE.enableLBT, BIN);Serial.print(" -> "); Serial.println(configuration.TRANSMISSION_MODE.getLBTEnableByteDescription());
  Serial.print(F("TransModeEnableRSSI: "));  Serial.print(configuration.TRANSMISSION_MODE.enableRSSI, BIN);Serial.print(" -> "); Serial.println(configuration.TRANSMISSION_MODE.getRSSIEnableByteDescription());
  Serial.print(F("TransModeEnabRepeat: "));  Serial.print(configuration.TRANSMISSION_MODE.enableRepeater, BIN);Serial.print(" -> "); Serial.println(configuration.TRANSMISSION_MODE.getRepeaterModeEnableByteDescription());
  Serial.print(F("TransModeFixedTrans: "));  Serial.print(configuration.TRANSMISSION_MODE.fixedTransmission, BIN);Serial.print(" -> "); Serial.println(configuration.TRANSMISSION_MODE.getFixedTransmissionDescription());

  Serial.println("----------------------------------------");
}
