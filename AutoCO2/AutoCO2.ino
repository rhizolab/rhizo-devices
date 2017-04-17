#include "CommandParser.h"
#include "CheckStream.h"
#include "Wire.h"


// pin/hardware definitions
#define LED_PIN 13
#define CO2_ADDR 0x7F


// a command parser for messages from controller
void runCommand(const char *deviceId, const char *command, byte argCount, char *args[]);
CommandParser cmd(runCommand);


// add wrappers to insert checksums
CheckStream g_output(Serial);


// other global variables
unsigned long g_lastSensorSend = 0;
unsigned long g_sendInterval = 0;


// run once on startup
void setup() {
  Serial.begin(9600);
  cmd.requireCheckSum(false);
  Wire.begin();
}


// run repeatedly as long as microcontroller has power
void loop() {

  // read incoming serial commands from controller node
  while (Serial.available()) {
    cmd.feed(Serial.read());
  }

  // periodically read and send sensor value 
  unsigned long time = millis();
  if (g_sendInterval && time - g_lastSensorSend > g_sendInterval) {
    int co2 = readCO2();
    if (co2 > 0) {
      digitalWrite(LED_PIN, HIGH);
      g_output.print("c:v ");
      g_output.println(co2);
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
    g_output.println("meta:devices c");

  // get info about each device
  } else if (strEq(deviceId, "c") && strEq(command, "info") && argCount == 0) {
    g_output.println("c:dir in");
    g_output.println("c:type CO2");
    g_output.println("c:model K-30");
    g_output.println("c:units PPM");
    g_output.println("c:ver 0.1");
    g_output.println("c:ready");

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


// read currently connected CO2 sensor via I2C; this will return 0 on read error
// based on code from co2meter.com
float readCO2() {

  // begin write sequence
  Wire.beginTransmission( CO2_ADDR );
  Wire.write( 0x22 );
  Wire.write( 0x00 );
  Wire.write( 0x08 );
  Wire.write( 0x2A );
  Wire.endTransmission();
  delay(10);

  // begin read sequence
  Wire.requestFrom( CO2_ADDR, 4 );
  byte i = 0;
  byte buffer[ 4 ] = { 0, 0, 0, 0 };
  while (Wire.available()) {
    buffer[ i ] = Wire.read();
    i++;
  }

  // prep/process value
  int co2Value = 0;
  co2Value |= buffer[ 1 ] & 0xFF;
  co2Value = co2Value << 8;
  co2Value |= buffer[ 2 ] & 0xFF;

  // check checksum
  byte sum = buffer[ 0 ] + buffer[ 1 ] + buffer[ 2 ];
  if (sum == buffer[ 3 ]) {
    return co2Value;
  } else {
    return -1;
  }
}

