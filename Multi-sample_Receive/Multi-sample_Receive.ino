#include "Arduino.h"
#include "LoRa_E22.h"
#include <SoftwareSerial.h>

// ---------------------- Hardware Pins ------------------------
// Adjust these pin numbers based on your wiring.
#define RX_PIN 14   // Arduino pin connected to E22 TX
#define TX_PIN 12   // Arduino pin connected to E22 RX
#define M0_PIN 13
#define M1_PIN 5
#define AUX_PIN 4

// -------------------------------------------------------------

// Create a SoftwareSerial for communication with E22 module
SoftwareSerial mySerial(RX_PIN, TX_PIN);

// Create the E22 object
LoRa_E22 e22ttl(&mySerial, M0_PIN, M1_PIN, AUX_PIN);

// Struct must match what transmitter sends
struct Message {
  float temperature;
  float humidity;
};

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  while(!Serial) { ; }
  delay(100);

  // Start communication with the E22 module
  e22ttl.begin();

  // Get and print current configuration
  ResponseStructContainer c = e22ttl.getConfiguration();
  Configuration configuration = *(Configuration*)c.data;

  // Configure this device as a receiver with the same CHAN as transmitter
  configuration.ADDH = 0x00;    // Receiver high address byte
  configuration.ADDL = 0x03;    // Receiver low address byte (must match transmitter's DESTINATION_ADDL)
  configuration.NETID = 0x00;
  configuration.CHAN = 4;       // Must match transmitter's channel

  // UART and Air parameters
  configuration.SPED.uartBaudRate = UART_BPS_9600;
  configuration.SPED.airDataRate = AIR_DATA_RATE_010_24;
  configuration.SPED.uartParity = MODE_00_8N1;

  // Options
  configuration.OPTION.subPacketSetting = SPS_240_00;
  configuration.OPTION.RSSIAmbientNoise = RSSI_AMBIENT_NOISE_DISABLED;
  configuration.OPTION.transmissionPower = POWER_22;

  // Transmission mode options
  configuration.TRANSMISSION_MODE.enableRSSI = RSSI_ENABLED;  // Enable RSSI
  configuration.TRANSMISSION_MODE.fixedTransmission = FT_FIXED_TRANSMISSION; 
  configuration.TRANSMISSION_MODE.enableRepeater = REPEATER_DISABLED;
  configuration.TRANSMISSION_MODE.enableLBT = LBT_DISABLED;
  configuration.TRANSMISSION_MODE.WORTransceiverControl = WOR_RECEIVER;
  configuration.TRANSMISSION_MODE.WORPeriod = WOR_2000_011;

  // Apply the new configuration
  e22ttl.setConfiguration(configuration, WRITE_CFG_PWR_DWN_SAVE);
  Serial.println(c.status.getResponseDescription());
  Serial.println(c.status.code);

  printParameters(configuration);
  c.close();

  Serial.println("Receiver ready.");
}

void loop() {
  // Check if data is available
  while (e22ttl.available() > 0) {
    ResponseContainer rc = e22ttl.receiveMessageRSSI();
    if (rc.status.code == 1) {
      int msgLength = rc.data.length();

      // We expect 8 bytes: 4 bytes for temperature float + 4 bytes for humidity float
      if (msgLength != sizeof(Message)) {
        Serial.print("Unexpected data length. Received ");
        Serial.print(msgLength);
        Serial.print(" bytes, expected ");
        Serial.println(sizeof(Message));
        continue;
      }

      // Interpret the received data as a Message struct
      const Message* m = (const Message*)rc.data.c_str();
      float temp = m->temperature;
      float hum = m->humidity;

      // Print the received values and RSSI
      Serial.print("Received Temp: ");
      Serial.print(temp);
      Serial.print("°C, Hum: ");
      Serial.print(hum);
      Serial.print("%, RSSI: ");
      Serial.println(rc.rssi, DEC);

      // Blink LED briefly to indicate data reception
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
      digitalWrite(LED_BUILTIN, HIGH);

    } else {
      // Print error if reception failed
      Serial.print("Error receiving data: ");
      Serial.println(rc.status.getResponseDescription());
    }
  }
}

void printParameters(struct Configuration configuration) {
  Serial.println("----------------------------------------");

  Serial.print(F("HEAD : "));
  Serial.print(configuration.COMMAND, HEX);Serial.print(" ");
  Serial.print(configuration.STARTING_ADDRESS, HEX);Serial.print(" ");
  Serial.println(configuration.LENGHT, HEX);
  Serial.println(F(" "));

  Serial.print(F("AddH : "));
  Serial.println(configuration.ADDH, HEX);
  Serial.print(F("AddL : "));
  Serial.println(configuration.ADDL, HEX);
  Serial.print(F("NetID : "));
  Serial.println(configuration.NETID, HEX);
  Serial.println(F(" "));

  Serial.print(F("Chan : "));
  Serial.print(configuration.CHAN, DEC); 
  Serial.print(" -> "); 
  Serial.println(configuration.getChannelDescription());
  Serial.println(F(" "));

  Serial.print(F("SpeedParityBit     : "));
  Serial.print(configuration.SPED.uartParity, BIN);
  Serial.print(" -> "); 
  Serial.println(configuration.SPED.getUARTParityDescription());
  Serial.print(F("SpeedUARTDatte     : "));
  Serial.print(configuration.SPED.uartBaudRate, BIN);
  Serial.print(" -> "); 
  Serial.println(configuration.SPED.getUARTBaudRateDescription());
  Serial.print(F("SpeedAirDataRate   : "));
  Serial.print(configuration.SPED.airDataRate, BIN);
  Serial.print(" -> "); 
  Serial.println(configuration.SPED.getAirDataRateDescription());
  Serial.println(F(" "));

  Serial.print(F("OptionSubPacketSett: "));
  Serial.print(configuration.OPTION.subPacketSetting, BIN);
  Serial.print(" -> "); 
  Serial.println(configuration.OPTION.getSubPacketSetting());
  Serial.print(F("OptionTranPower    : "));
  Serial.print(configuration.OPTION.transmissionPower, BIN);
  Serial.print(" -> "); 
  Serial.println(configuration.OPTION.getTransmissionPowerDescription());
  Serial.print(F("OptionRSSIAmbientNo: "));
  Serial.print(configuration.OPTION.RSSIAmbientNoise, BIN);
  Serial.print(" -> "); 
  Serial.println(configuration.OPTION.getRSSIAmbientNoiseEnable());
  Serial.println(F(" "));

  Serial.print(F("TransModeWORPeriod : "));
  Serial.print(configuration.TRANSMISSION_MODE.WORPeriod, BIN);
  Serial.print(" -> "); 
  Serial.println(configuration.TRANSMISSION_MODE.getWORPeriodByParamsDescription());
  Serial.print(F("TransModeTransContr: "));
  Serial.print(configuration.TRANSMISSION_MODE.WORTransceiverControl, BIN);
  Serial.print(" -> "); 
  Serial.println(configuration.TRANSMISSION_MODE.getWORTransceiverControlDescription());
  Serial.print(F("TransModeEnableLBT : "));
  Serial.print(configuration.TRANSMISSION_MODE.enableLBT, BIN);
  Serial.print(" -> "); 
  Serial.println(configuration.TRANSMISSION_MODE.getLBTEnableByteDescription());
  Serial.print(F("TransModeEnableRSSI: "));
  Serial.print(configuration.TRANSMISSION_MODE.enableRSSI, BIN);
  Serial.print(" -> "); 
  Serial.println(configuration.TRANSMISSION_MODE.getRSSIEnableByteDescription());
  Serial.print(F("TransModeEnabRepeat: "));
  Serial.print(configuration.TRANSMISSION_MODE.enableRepeater, BIN);
  Serial.print(" -> "); 
  Serial.println(configuration.TRANSMISSION_MODE.getRepeaterModeEnableByteDescription());
  Serial.print(F("TransModeFixedTrans: "));
  Serial.print(configuration.TRANSMISSION_MODE.fixedTransmission, BIN);
  Serial.print(" -> "); 
  Serial.println(configuration.TRANSMISSION_MODE.getFixedTransmissionDescription());

  Serial.println("----------------------------------------");
}
