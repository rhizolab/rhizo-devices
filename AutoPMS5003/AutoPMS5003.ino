#include "CommandParser.h"
#include "CheckStream.h"
#include "SoftwareSerial.h"


// pin/hardware definitions
#define LED_PIN 13
#define SENSOR_RX_PIN 11
#define SENSOR_TX_PIN 12 // not used


// a command parser for messages from controller
void runCommand(const char *deviceId, const char *command, byte argCount, char *args[]);
CommandParser cmd(runCommand);


// add wrappers to insert checksums
CheckStream g_output(Serial);


// serial connection to oxygen sensor
SoftwareSerial g_serial(SENSOR_RX_PIN, SENSOR_TX_PIN);


// structure for data from PMS5003 sensor
// from Adafruit PMS5003 sample code
struct pms5003data {
  uint16_t framelen;
  uint16_t pm10_standard, pm25_standard, pm100_standard;
  uint16_t pm10_env, pm25_env, pm100_env;
  uint16_t particles_03um, particles_05um, particles_10um, particles_25um, particles_50um, particles_100um;
  uint16_t unused;
  uint16_t checksum;
};
pms5003data g_data;
bool g_dataValid = false;


// other global variables
unsigned long g_lastSensorSend = 0;
unsigned long g_sendInterval = 0;


// run once on startup
void setup() {
  Serial.begin(9600);
  g_serial.begin(9600);
  cmd.requireCheckSum(false);
}


// run repeatedly as long as microcontroller has power
void loop() {

  // read incoming serial commands from controller node
  while (Serial.available()) {
    cmd.feed(Serial.read());
  }

  // read PMS5003 sensor data
  g_dataValid = readPMSdata();

  // periodically read and send sensor value 
  unsigned long time = millis();
  if (g_sendInterval && time - g_lastSensorSend > g_sendInterval) {
    if (g_dataValid) {
      digitalWrite(LED_PIN, HIGH);
      g_output.print("p:v ");
      g_output.println(g_data.pm25_env);
      g_lastSensorSend = time;
      delay(10);  // delay a moment so LED is visible
      digitalWrite(LED_PIN, LOW);
    }
  }
}


// command processor
void runCommand(const char *deviceId, const char *command, byte argCount, char *args[]) {
  bool recognized = true;
  
  // get list of devices provided by this board
  if (strEq(command, "devices") && argCount == 0) {
    g_output.println("meta:devices p");

  // get info about each device
  } else if (strEq(deviceId, "p") && strEq(command, "info") && argCount == 0) {
    g_output.println("p:dir in");
    g_output.println("p:type particulates");
    g_output.println("p:model PMS5003");
    g_output.println("p:units PM2.5");
    g_output.println("p:ver 0.1");
    g_output.println("p:ready");

  // enable/disable checksums
  } else if (strEq(command, "checksum") && argCount == 1) {
    cmd.requireCheckSum(atoi(args[0]));

  // set how often to send sensor readings (in seconds)
  } else if (strEq(command, "interval") && argCount == 1) {
    g_sendInterval = round(1000.0 * atof(args[0]));
    if (g_sendInterval < 1000) {
      g_sendInterval = 1000;  // require interval of at least 1 second
    }

  // invalid command
  } else {
    recognized = false;
  }

  // ack/nack the message
  if (recognized) {
    g_output.print(deviceId);
    g_output.print(":");
    g_output.print("ack ");
    g_output.print(command);
    for (int i = 0; i < argCount; i++) {
      g_output.print(" ");
      g_output.print(args[ i ]); 
    }
  } else {
    g_output.print("nack");
  }
  g_output.println();
  g_output.flush();
}


// read serial data from the given stream
// based on Adafruit sample PMS5003 code
boolean readPMSdata() {
  if (g_serial.available() == false) {
    return false;
  }
  
  // Read a byte at a time until we get to the special '0x42' start-byte
  if (g_serial.peek() != 0x42) {
    g_serial.read();
    return false;
  }

  // Now read all 32 bytes
  if (g_serial.available() < 32) {
    return false;
  }
    
  uint8_t buffer[32];    
  uint16_t sum = 0;
  g_serial.readBytes(buffer, 32);

  // get checksum ready
  for (uint8_t i=0; i<30; i++) {
    sum += buffer[i];
  }

  /* debugging
  for (uint8_t i=2; i<32; i++) {
    Serial.print("0x"); Serial.print(buffer[i], HEX); Serial.print(", ");
  }
  Serial.println();
  */
  
  // The data comes in endian'd, this solves it so it works on all platforms
  uint16_t buffer_u16[15];
  for (uint8_t i=0; i<15; i++) {
    buffer_u16[i] = buffer[2 + i*2 + 1];
    buffer_u16[i] += (buffer[2 + i*2] << 8);
  }

  // put it into a nice struct :)
  memcpy((void *)&g_data, (void *)buffer_u16, 30);

  if (sum != g_data.checksum) {
    Serial.println("Checksum failure");
    return false;
  }

  // success!
  return true;
}
