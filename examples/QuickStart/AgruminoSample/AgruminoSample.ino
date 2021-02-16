/*
  AgruminoSample.ino - Sample project for Agrumino board using the Agrumino
  library. Created by giuseppe.broccia@lifely.cc on October 2017.

  This sketch read all the values from the Agrumino Board
  and print them in the serial console every 30 sec.

  @see Agrumino.h for the documentation of the lib
*/

#include <Agrumino.h>

#define SLEEP_TIME_SEC 30

Agrumino agrumino;

void setup() {
  Serial.begin(115200);
  agrumino.setup();
}

void loop() {
  Serial.println("#########################\n");

  agrumino.turnBoardOn();

  boolean isAttachedToUSB = agrumino.isAttachedToUSB();
  boolean isBatteryCharging = agrumino.isBatteryCharging();
  boolean isButtonPressed = agrumino.isButtonPressed();
  float temperature = agrumino.readTempC();
  unsigned int soilMoisture = agrumino.readSoil();
  float illuminance = agrumino.readLux();
  float batteryVoltage = agrumino.readBatteryVoltage();
  unsigned int batteryLevel = agrumino.readBatteryLevel();

  Serial.println("");
  Serial.println("isAttachedToUSB:   " + String(isAttachedToUSB));
  Serial.println("isBatteryCharging: " + String(isBatteryCharging));
  Serial.println("isButtonPressed:   " + String(isButtonPressed));
  Serial.println("temperature:       " + String(temperature) + "°C");
  Serial.println("soilMoisture:      " + String(soilMoisture) + "%");
  Serial.println("illuminance :      " + String(illuminance) + " lux");
  Serial.println("batteryVoltage :   " + String(batteryVoltage) + " V");
  Serial.println("batteryLevel :     " + String(batteryLevel) + "%");
  Serial.println("");

  if (isButtonPressed) {
    agrumino.turnWateringOn();
    delay(2000);
    agrumino.turnWateringOff();
  }

  blinkLed();

  agrumino.turnBoardOff(); // Board off before delay/sleep to save battery :)

  // delaySec(SLEEP_TIME_SEC); // The ESP8266 stays powered, executes the loop
  // repeatedly
  deepSleepSec(
      SLEEP_TIME_SEC); // ESP8266 enter in deepSleep and after the selected time
                       // starts back from setup() and then loop()
}

/////////////////////
// Utility methods //
/////////////////////

void blinkLed() {
  agrumino.turnLedOn();
  delay(200);
  agrumino.turnLedOff();
}

void delaySec(int sec) { delay(sec * 1000); }

void deepSleepSec(int sec) {
  ESP.deepSleep(sec * 1000000); // microseconds
}
