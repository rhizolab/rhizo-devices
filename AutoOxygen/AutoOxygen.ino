#include "CommandParser.h"
#include "CheckStream.h"
#include "SoftwareSerial.h"


// pin/hardware definitions
#define LED_PIN 13
#define OX_RX_PIN 11
#define OX_TX_PIN 12 // not used


// a command parser for messages from controller
void runCommand(const char *deviceId, const char *command, byte argCount, char *args[]);
CommandParser cmd(runCommand);


// add wrappers to insert checksums
CheckStream g_output(Serial);


// serial connection to oxygen sensor
SoftwareSerial g_oxSerial(OX_RX_PIN, OX_TX_PIN);


// buffer for oxygen data
#define OX_BUF_LEN 80
char g_oxBuf[OX_BUF_LEN];
int g_oxBufIndex = 0;


// latest sensor reading
float g_ox = 0;


// other global variables
unsigned long g_lastSensorSend = 0;
unsigned long g_sendInterval = 0;


// run once on startup
void setup() {
  Serial.begin(9600);
  g_oxSerial.begin(9600);
  cmd.requireCheckSum(false);
}


// run repeatedly as long as microcontroller has power
void loop() {

  // read incoming serial commands from controller node
  while (Serial.available()) {
    cmd.feed(Serial.read());
  }

  // read oxygen sensor data
  checkOx();

  // periodically read and send sensor value 
  unsigned long time = millis();
  if (g_sendInterval && time - g_lastSensorSend > g_sendInterval) {
    if (g_ox > 0) {
      digitalWrite(LED_PIN, HIGH);
      g_output.print("o:v ");
      g_output.println(g_ox);
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
    g_output.println("meta:devices o");

  // get info about each device
  } else if (strEq(deviceId, "o") && strEq(command, "info") && argCount == 0) {
    g_output.println("o:dir in");
    g_output.println("o:type O2");
    g_output.println("o:model CM-0201");
    g_output.println("o:units percent");
    g_output.println("o:ver 0.1");
    g_output.println("o:ready");

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


// check for incoming serial data from oxygen sensor; if whole message is received, parse it
void checkOx() {
  while (g_oxSerial.available()) {
    char c = g_oxSerial.read();
    if (c == 10) {
      parseOxBuf();
    } else if (g_oxBufIndex + 1 < OX_BUF_LEN) {
      g_oxBuf[g_oxBufIndex] = c;
      g_oxBufIndex++;
    }
  } 
}


// parse message from oxygen sensor
// example: "O 0213.7 T +22.2 P 1010 % 021.16 e 0000"
void parseOxBuf() {
  g_oxBuf[g_oxBufIndex] = 0;
  int len = g_oxBufIndex;
  int start = indexOf(g_oxBuf, '%');
  if (start > 0 && start + 2 < len) {
    g_ox = atof(g_oxBuf + start + 2);
  }
  g_oxBufIndex = 0;
}


