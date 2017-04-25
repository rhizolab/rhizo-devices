#include "CommandParser.h"
#include "CheckStream.h"
#include "Adafruit_TCS34725.h"


// pin definitions
#define LED_PIN 13


// a command parser for messages from controller node
void runCommand(const char *deviceId, const char *command, byte argCount, char *args[]);
CommandParser cmd(runCommand);


// add wrappers to insert checksums
CheckStream g_output(Serial);


// initialize the sensor library
Adafruit_TCS34725 g_tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_700MS, TCS34725_GAIN_1X);


// other global variables
float g_temperature = 0;
float g_humidity = 0;
bool g_sensorOk = false;
unsigned long g_lastSensorSend = 0;
unsigned long g_sendInterval = 0;


// run once on startup
void setup() {
  Serial.begin(9600);
  cmd.requireCheckSum(false);
  g_tcs.begin();
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
    digitalWrite(LED_PIN, HIGH);
    uint16_t r = 0, g = 0, b = 0, c = 0;
    g_tcs.getRawData(&r, &g, &b, &c);
    uint16_t lux = g_tcs.calculateLux(r, g, b);
    g_output.print("r:v ");
    g_output.println(r);
    g_output.print("g:v ");
    g_output.println(g);
    g_output.print("b:v ");
    g_output.println(b);
    g_output.print("l:v ");
    g_output.println(lux);
    g_lastSensorSend = time;
    delay(5);
    digitalWrite(LED_PIN, LOW);
  }
}


// command processor
void runCommand(const char *deviceId, const char *command, byte argCount, char *args[]) {
  bool recognized = true;

  // get list of devices provided by this board
  if (strEq(command, "devices") && argCount == 0) {
    g_output.println("meta:devices r g b l");

  // get info about red channel
  } else if (strEq(deviceId, "r") && strEq(command, "info") && argCount == 0) {
    g_output.println("r:dir in");
    g_output.println("r:type red");
    g_output.println("r:model TCS34725");
    g_output.println("r:ver 0.1");
    g_output.println("r:ready");

  // get info about green channel
  } else if (strEq(deviceId, "g") && strEq(command, "info") && argCount == 0) {
    g_output.println("g:dir in");
    g_output.println("g:type green");
    g_output.println("g:model TCS34725");
    g_output.println("g:ver 0.1");
    g_output.println("g:ready");

  // get info about blue channel
  } else if (strEq(deviceId, "b") && strEq(command, "info") && argCount == 0) {
    g_output.println("b:dir in");
    g_output.println("b:type blue");
    g_output.println("b:model TCS34725");
    g_output.println("b:ver 0.1");
    g_output.println("b:ready");

  // get info about lux measurement
  } else if (strEq(deviceId, "l") && strEq(command, "info") && argCount == 0) {
    g_output.println("l:dir in");
    g_output.println("l:type lux");
    g_output.println("l:model TCS34725");
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

