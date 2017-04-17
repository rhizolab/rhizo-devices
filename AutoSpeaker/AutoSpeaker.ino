#include "CommandParser.h"
#include "CheckStream.h"


// pin definitions
#define SPEAKER_PIN 2
#define LED_PIN 13


// a command parser for messages from controller
void runCommand(const char *deviceId, const char *command, byte argCount, char *args[]);
CommandParser cmd(runCommand);


// add wrappers to insert checksums
CheckStream g_output(Serial);


// other global variables
unsigned long g_lastSensorSend = 0;
unsigned long g_sendInterval = 1000;


// run once on startup
void setup() {
  Serial.begin(9600);
  cmd.requireCheckSum(false);
  pinMode(SPEAKER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(SPEAKER_PIN, 0);
  digitalWrite(LED_PIN, 0);
}


// repeatedly read incoming serial commands from controller
void loop() {
  while (Serial.available()) {
    cmd.feed(Serial.read());
  }
}


// command processor
void runCommand(const char *deviceId, const char *command, byte argCount, char *args[]) {
  bool recognized = true;

  // get list of devices provided by this board
  if (strEq(command, "devices") && argCount == 0) {
    g_output.println("meta:devices s");

  // get info about each device
  } else if (strEq(deviceId, "s") && strEq(command, "info") && argCount == 0) {
    g_output.println("s:dir out");
    g_output.println("s:type speaker");
    g_output.println("s:ver 0.1");
    g_output.println("s:ready");

  // set the speaker output
  } else if (strEq(command, "set") && argCount == 1) {
    int val = atoi(args[0]);
    if (val) {
      tone(SPEAKER_PIN, 300);
    } else {
      noTone(SPEAKER_PIN);
    }
    digitalWrite(LED_PIN, val);

  // enable/disable checksums
  } else if (strEq(command, "checksum") && argCount == 1) {
    cmd.requireCheckSum(atoi(args[0]));

  // set update interval (not used by this device)
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

