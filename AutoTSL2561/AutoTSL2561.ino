#include "CommandParser.h"
#include "CheckStream.h"
#include "TSL2561.h"


// pin definitions
#define LED_PIN 13


// a command parser for messages from controller node
void runCommand(const char *boardId, const char *command, byte argCount, char *args[]);
CommandParser cmd(runCommand);


// add wrappers to insert checksums
CheckStream g_output(Serial);


// other global variables
TSL2561 tsl(TSL2561_ADDR_FLOAT);
unsigned long g_lastSensorSend = 0;
unsigned long g_sendInterval = 0;


// run once on startup
void setup() {
  Serial.begin(9600);
  cmd.requireCheckSum(false);
  tsl.begin();
  tsl.setGain(TSL2561_GAIN_0X);
  tsl.setTiming(TSL2561_INTEGRATIONTIME_101MS);
}


// run repeatedly as long as microcontroller has power
void loop() {

  // read incoming serial commands from controller node
  while (Serial.available()) {
    cmd.feed(Serial.read());
  }

  // periodically read sensor value 
  unsigned long time = millis();
  if (g_sendInterval && time - g_lastSensorSend > g_sendInterval) {
    digitalWrite(LED_PIN, HIGH);
    uint32_t lum = tsl.getFullLuminosity();
    uint16_t ir, full;
    ir = lum >> 16;
    full = lum & 0xFFFF;
    int lux = tsl.calculateLux(full, ir);
    g_output.print("l:v ");
    g_output.println(lux);
    g_lastSensorSend = time;
    delay(10);  // delay a moment so LED is visible
    digitalWrite(LED_PIN, LOW);
  }
}


// command processor
void runCommand(const char *boardId, const char *command, byte argCount, char *args[]) {
  bool recognized = true;
  
  // get list of devices provided by this board
  if (strEq(command, "devices") && argCount == 0) {
    g_output.println("meta:devices l");

  // get info about each device
  } else if (strEq(boardId, "l") && strEq(command, "info") && argCount == 0) {
    g_output.println("l:dir in");
    g_output.println("l:type light");
    g_output.println("l:model TSL2561");
    g_output.println("l:units lux");
    g_output.println("l:ver 0.1");
    g_output.println("l:ready");

  // enable/disable checksums
  } else if (strEq(command, "checksum") && argCount == 1) {
    cmd.requireCheckSum(atoi(args[0]));

  // set how often to send sensor readings (in seconds)
  } else if (strEq(command, "interval") && argCount == 1) {
    g_sendInterval = round(1000.0 * atof(args[0]));

  // invalid command
  } else {
    recognized = false;
  }

  // ack/nack the message
  if (recognized) {
    g_output.print(boardId);
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

