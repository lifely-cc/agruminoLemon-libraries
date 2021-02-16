/*AgruminoThingspeak.ino - Very seimple project for Agrumino board using the Agrumino library.
  Created by gabriele.foddis@lifely.cc
  This sketch send data in your thingspeak account.
  For this sketch is necessary a thingspeak account ( free account available)*/

#include <Agrumino.h>
#include <ESP8266WiFi.h>
#define SSID_NAME ""///Insert your wifi name" 
#define SSID_PASSWORD  "" ///Insert your wifi password"
#define CONNECTION_ATTEMPS 10 ///Maximum number of connection attempt
#define BAUD_RATE 115200 ///Serial baud rate
#define MIN_TO_MS (1000000 * 60)
int agruminoId = ESP.getChipId();
const char* host = "api.thingspeak.com";
const char* writeAPIKey = ""; ////API key of your channel
unsigned int SLEEP_TIME_MIN = 2; /*only minute sleep, max 60 minute */
int wifiCount = 0;
Agrumino agrumino;

void blinkLedConnect() {
  agrumino.turnLedOn();
  delay(200);
  agrumino.turnLedOff();
  delay(200);
  agrumino.turnLedOn();
  delay(200);
  agrumino.turnLedOff();
  delay(200);
  agrumino.turnLedOn();
  delay(200);
  agrumino.turnLedOff();
}

void blinkLed() {
  agrumino.turnLedOn();
  delay(200);
  agrumino.turnLedOff();
}

void setup() {

  Serial.begin(BAUD_RATE);
  agrumino.setup();
  agrumino.turnBoardOn();
  wifiSetup();
}

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
    Serial.println("Failed to connect,...sleep....");
    agrumino.turnBoardOff(); // Turn Board off before sleep to save battery :)
    ESP.deepSleep(MIN_TO_MS * SLEEP_TIME_MIN);
  }
  else if (WiFi.status() == WL_CONNECTED) {

    Serial.println("I'm Connected with:"  );
    Serial.println(SSID_NAME);
    delay(100);
  }
}

void loop() {
  

  if (WiFi.status() == WL_CONNECTED)
  {
    blinkLedConnect();
  }
  
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    return;
  }
  Serial.println("#########################\n");

  boolean isAttachedToUSB =   agrumino.isAttachedToUSB();
  boolean isBatteryCharging = agrumino.isBatteryCharging();
  boolean isButtonPressed =   agrumino.isButtonPressed();
  float temperature =         agrumino.readTempC();
  float soilMoistureMv =      agrumino.readSoilRaw();
  float soilMoisturePerc =    agrumino.readSoil();
  float illuminance =         agrumino.readLux();
  float batteryVoltage =      agrumino.readBatteryVoltage();
  unsigned int batteryLevel = agrumino.readBatteryLevel();
  int agruminoId = ESP.getChipId();


  Serial.println("");
  Serial.println("isAttachedToUSB:   " + String(isAttachedToUSB));
  Serial.println("isBatteryCharging: " + String(isBatteryCharging));
  Serial.println("isButtonPressed:   " + String(isButtonPressed));
  Serial.println("temperature:       " + String(temperature) + " C");
  Serial.println("soilMoistureMv:    " + String(soilMoistureMv) + " mV");
  Serial.println("soilMoisture%:     " + String(soilMoisturePerc) + "%");
  Serial.println("illuminance :      " + String(illuminance) + " lux");
  Serial.println("batteryVoltage :   " + String(batteryVoltage) + " V");
  Serial.println("batteryLevel :     " + String(batteryLevel) + "%");
  Serial.println("");

  //Write data, channel settings.

  String url = "/update?key=";
  url += writeAPIKey;
  url += "&field1="; 
  url += String(temperature * 1000);
  url += "&field2=";
  url += String(soilMoistureMv);
  url += "&field3=";
  url += String(soilMoisturePerc);
  url += "&field4=";
  url += String(illuminance);
  url += "&field5=";
  url += String(batteryVoltage);
  url += "&field6=";
  url += String(batteryLevel);
  url += "&field7=";
  url += String(agruminoId);
  url += "\r\n";


  // Request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");

  blinkLed();
  agrumino.turnBoardOff(); // Board off before delay/sleep to save battery :)
  Serial.println("sleep mode enabled.....bye");
  ESP.deepSleep(MIN_TO_MS * SLEEP_TIME_MIN);
}
