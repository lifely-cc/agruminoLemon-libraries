/*AgruminoLemonTesting.ino - 
  Created by gabriele.foddis@lifely.cc         05/2021
  With this sketch and the Fritizing img you can try if your device work properly. 
  Testing: built-in led, GPIO connector, I2c connector, Pump Connector, Liv connector, WiFi Connection, and also... Lux, temperature and Soil Moisture sensors. 
  Look at the images in the folder library or in this link https://github.com/lifely-cc/agruminoLemonTesting 
  ******WARNING******, for this sketch need a internet connection */

#include <Arduino.h>
#include <Agrumino.h>
#include <ESP8266WiFi.h>
#include <U8g2lib.h>
#include "logo.h"  ///Lifely's logo
#define SERIAL_PORT 115200
#define CONNECTION_ATTEMPS 30

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
Agrumino agrumino;

#define SSID_NAME "Lifely"///Change with your SSID" 
#define SSID_PASSWORD  "Agrumino" ///Change with your wifi password"
#define CONNECTION_ATTEMPS 30
int wifiCount = 0;
int countLogo = 0;

void wifiSetup() {

  Serial.print("Connecting to ");
  Serial.println(SSID_NAME);
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID_NAME, SSID_PASSWORD);

  while ((WiFi.status() != WL_CONNECTED) && (wifiCount < CONNECTION_ATTEMPS))
  {
    delay(500);
    wifiCount++;
    Serial.print(".");

  }

  if (wifiCount >= CONNECTION_ATTEMPS)
  {
    Serial.println("Failed to connect,....Please check your internet connection...I'm rebooting now...");
    ESP.restart();
  }
  else if (WiFi.status() == WL_CONNECTED) {

    Serial.println("I'm Connected with:"  );
    Serial.println(SSID_NAME);
    delay(100);
  }
}

void testLevel() {
  agrumino.setGPIOMode(LIV_1, GPIO_INPUT);
  int valueLiv = 0;
  valueLiv = agrumino.readGPIO(LIV_1);
  Serial.println("Level Pin is :   " + String(valueLiv));
  delay(5000);

}

void testGpio1() {
  agrumino.setGPIOMode(GPIO_1, GPIO_OUTPUT);
  agrumino.writeGPIO(GPIO_1, HIGH);
  Serial.println("Set Gpio1 HIGH");
  delay(1500);
  agrumino.writeGPIO(GPIO_1, LOW);
  Serial.println("Set Gpio1 LOW");

}

void testGpio2() {
  agrumino.setGPIOMode(GPIO_2 , GPIO_OUTPUT);
  agrumino.writeGPIO(GPIO_2, HIGH);
  Serial.println("Set Gpio2 HIGH");
  delay(1500);
  agrumino.writeGPIO(GPIO_2, LOW);
  Serial.println("Set Gpio2 LOW");

}

void testPump() {
  agrumino.turnWateringOn();
  Serial.println("Pump On");
  delay(5000);
  agrumino.turnWateringOff();
  Serial.println("Pump Off");

}

void writeTestInProgress() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB12_tr);
  u8g2.drawStr(40, 12, "TEST");
  u8g2.drawStr(15, 36, "in progress");
  u8g2.sendBuffer();
  delay(500);
  
}

void drawlogo(void)
{
  u8g2.clearDisplay();
  u8g2.clearBuffer();

  for (int i = -48; i <= 10; i++)
  {
    ESP.wdtFeed();
    u8g2.drawXBM(0, i, LOGO_WIDTH, LOGO_HEIGHT, LOGO_bits);
    u8g2.sendBuffer();
  }
  delay(3000);

}

void writeDataSensors()
{
  int temperature = agrumino.readTempC();
  int soilMoisture = agrumino.readSoil();
  float illuminance = agrumino.readLux();
  int soilMoistureRaw = agrumino.readSoilRaw();
  Serial.println("Temperature:       " + String(temperature) + "°C");
  Serial.println("SoilMoisture:      " + String(soilMoisture) + "%");
  Serial.println("Illuminance        " + String(illuminance) + "Lum");
  u8g2.clearBuffer();
  u8g2.clearDisplay();
  u8g2.setFont(u8g2_font_6x10_mf);
  u8g2.drawStr(2,10, "Temperature C =");
  u8g2.setCursor(94,10);
  u8g2.print(temperature);
  u8g2.drawStr(2,25, "Soil_moist %  =");
  u8g2.setCursor(94,25);
  u8g2.print(soilMoisture);
  u8g2.drawStr(2,40, "Soil_moist_Mv =");
  u8g2.setCursor(94,40);
  u8g2.print(soilMoistureRaw);
  u8g2.drawStr(2,55, "Illum. =");
  u8g2.setCursor(85,55);
  u8g2.print(illuminance);
  u8g2.sendBuffer();
  delay(5000);
  u8g2.clearBuffer();
  u8g2.clearDisplay();
}

void blinkLed()
{
  agrumino.turnLedOn();
  delay(200);
  agrumino.turnLedOff();
}

void delaySec(int sec)
{
  delay(sec * 1000);
}

void setup()
{
  Serial.begin(SERIAL_PORT);
  agrumino.setup();
  agrumino.turnBoardOn();
  u8g2.begin();
  wifiSetup();
  delay(500);
}

void loop(void)

{ agrumino.turnBoardOn();

  if (countLogo <= 1 ) {
    drawlogo();
  }
  long int agruminoChipId = ESP.getChipId();
  Serial.println("My ChipId is : " + String(agruminoChipId));
  countLogo++;
  writeTestInProgress();
  blinkLed(); 
  testPump();
  testGpio1();
  testGpio2();
  u8g2.clearDisplay();
  writeDataSensors();
  //testLevel(); //View status only from serial monitor
  Serial.println("Restart.....");
  delay(3000);
  u8g2.clearDisplay();
  ESP.restart();
}