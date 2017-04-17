#include "CommandParser.h"
#include "CheckStream.h"
#include "DHT.h"


// pin definitions
#define DHT_PIN 2
#define LED_PIN 13


// a command parser for messages from controller node
void runCommand(const char *deviceId, const char *command, byte argCount, char *args[]);
CommandParser cmd(runCommand);


// add wrappers to insert checksums
CheckStream g_output(Serial);


// other global variables
DHT g_dht(DHT_PIN, DHT22);
float g_temperature = 0;
float g_humidity = 0;
bool g_sensorOk = false;
unsigned long g_lastSensorSend = 0;
unsigned long g_sendInterval = 0;


// run once on startup
void setup() {
  Serial.begin(9600);
  cmd.requireCheckSum(false);
  g_dht.begin();
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
    float temperature = g_dht.readTemperature();
    if (isnan(temperature) == false && temperature > -90) {
      g_temperature = temperature;
      g_humidity = g_dht.readHumidity();
      g_sensorOk = true;
    }
    if (g_sensorOk) {
      digitalWrite(LED_PIN, HIGH);
      g_output.print("t:v ");
      g_output.println(g_temperature);
      g_output.print("h:v ");
      g_output.println(g_humidity);
      g_lastSensorSend = time;
      delay(10);
      digitalWrite(LED_PIN, LOW);
    }
  }
}


// command processor
void runCommand(const char *deviceId, const char *command, byte argCount, char *args[]) {
  bool recognized = true;
  
  // get list of devices provided by this board
  if (strEq(command, "devices") && argCount == 0) {
    g_output.println("meta:devices t h");

  // get info about temperature sensor
  } else if (strEq(deviceId, "t") && strEq(command, "info") && argCount == 0) {
    g_output.println("t:dir in");
    g_output.println("t:type temperature");
    g_output.println("t:model AM2302");
    g_output.println("t:units degrees C");
    g_output.println("t:ver 0.1");
    g_output.println("t:ready");

  // get info about humidity sensor
  } else if (strEq(deviceId, "h") && strEq(command, "info") && argCount == 0) {
    g_output.println("t:dir in");
    g_output.println("h:type humidity");
    g_output.println("h:model AM2302");
    g_output.println("h:units percent");
    g_output.println("h:ver 0.1");
    g_output.println("h:ready");

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

