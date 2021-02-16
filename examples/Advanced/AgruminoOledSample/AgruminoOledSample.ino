/*
  AgruminoOledSample.ino - Sample project for using an I2C OLED display with the Agrumino board.
  @see https://github.com/olikraus/u8g2
  
  Created by giuseppe.broccia@lifely.cc on June 2018.
*/

#include <Agrumino.h>
#include <U8g2lib.h>

#define SLEEP_TIME_SEC 1

Agrumino agrumino;
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R2, U8X8_PIN_NONE); // Adafruit ESP8266/32u4/ARM Boards + FeatherWing OLED

void setup() {
  Serial.begin(115200);
  agrumino.setup();
  agrumino.turnBoardOn();

  u8g2.begin();
}

void loop() {
  Serial.println("#########################\n");

  boolean isAttachedToUSB =   agrumino.isAttachedToUSB();
  boolean isBatteryCharging = agrumino.isBatteryCharging();
  boolean isButtonPressed =   agrumino.isButtonPressed();
  float temperature =         agrumino.readTempC();
  unsigned int soilMoisture = agrumino.readSoil();
  float illuminance =         agrumino.readLux();
  float batteryVoltage =      agrumino.readBatteryVoltage();

  Serial.println("");
  Serial.println("isAttachedToUSB:   " + String(isAttachedToUSB));
  Serial.println("isBatteryCharging: " + String(isBatteryCharging));
  Serial.println("isButtonPressed:   " + String(isButtonPressed));
  Serial.println("temperature:       " + String(temperature) + "°C");
  Serial.println("soilMoisture:      " + String(soilMoisture) + "%");
  Serial.println("illuminance :      " + String(illuminance) + " lux");
  Serial.println("batteryVoltage :   " + String(batteryVoltage) + " V");
  Serial.println("");

  writeDataToOled(temperature, soilMoisture, illuminance, batteryVoltage);

  blinkLed();

  delaySec(SLEEP_TIME_SEC); // The ESP8266 stays powered, executes the loop repeatedly
  // agrumino.deepSleepSec(SLEEP_TIME_SEC); // ESP8266 enter in deepSleep and after the selected time starts back from setup() and then loop()
}

void writeDataToOled(float temp, unsigned int  soil, float lux, float batt) {
  String tempString = String(temp) + " C";
  String soilString = String(soil) + " %";
  String luxString  = String(lux)  + " lx";
  String battString = String(batt) + " V";

  u8g2.clearBuffer();          // clear the internal memory
  u8g2.setFont(u8g2_font_ncenB10_tr); // choose a suitable font
  u8g2.drawStr(0, 12, tempString.c_str()); // write something to the internal memory
  u8g2.drawStr(84, 12, soilString.c_str()); // write something to the internal memory
  u8g2.drawStr(0, 32, luxString.c_str()); // write something to the internal memory
  u8g2.drawStr(84, 32, battString.c_str()); // write something to the internal memory

  u8g2.sendBuffer();          // transfer internal memory to the display
}

/////////////////////
// Utility methods //
/////////////////////

void blinkLed() {
  agrumino.turnLedOn();
  delay(200);
  agrumino.turnLedOff();
}

void delaySec(int sec) {
  delay (sec * 1000);
}