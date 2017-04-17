#include "CommandParser.h"
#include "CheckStream.h"


// pin definitions
#define LED_PIN 13


// a command parser for messages from controller node
void runCommand(const char *boardId, const char *command, byte argCount, char *args[]);
CommandParser cmd(runCommand);


// add wrappers to insert checksums
CheckStream g_output(Serial);


// other global variables
unsigned long g_lastSensorSend = 0;
unsigned long g_sendInterval = 0;


// run once on setup
void setup() {
  Serial.begin(9600);
  cmd.requireCheckSum(false);
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
    long val = analogRead(0) * 100L / 1023;
    g_output.print("p:v ");
    g_output.println(val, DEC);
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
    g_output.println("meta:devices p");

  // get info about each device
  } else if (strEq(boardId, "p") && strEq(command, "info") && argCount == 0) {
    g_output.println("p:dir in");
    g_output.println("p:type potentiometer");
    g_output.println("p:model 10k linear");
    g_output.println("p:ver 0.1");
    g_output.println("p:ready");

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

