/*AgruminoThingerCloudWithPump.ino - Sample project for Agrumino board using the Agrumino library.
  Created by gabriele.foddis@lifely.cc
  This sketch send data on Thinger.io. The pump can be activated only in real time monitoring 
  for this sketch if necessary a thinger.io account (free for 2 device)*/

//#define _DEBUG_ //Uncomment if you need a debug
//#define _DISABLE_TLS_ //Uncomment for a unsecure connection
#include <ESP8266WiFi.h>///ESP8266 LIBRARY
#include <Agrumino.h>///Agrumino Library
#include <ThingerESP8266.h>///Thinger.io Library
#define USERNAME ""///Insert your username 
#define DEVICE_ID ""///Insert the name of yours device 
#define DEVICE_CREDENTIAL ""///Insert your device credential 
#define BUCKET_NAME ""///////////Insert your Bucket name
#define MIN_TO_MS (1000000 * 60)
#define SSID ""///Insert your wifi name" 
#define SSID_PASSWORD  "" ///Insert your wifi password"
#define PIN_PUMP 12
#define CONNECTION_ATTEMPS 30 ///Maximum number of connection attempt
#define BAUD_RATE 115200 ///Serial baud rate
int agruminoName = ESP.getChipId();
unsigned int SLEEP_TIME_MIN = 5; //Set this to min 1 to 60 min max.
bool realTimeMonitoring = false; //Set false for maximum energy saving or true for Real Time Monitoring
int wifiCount = 0;
ThingerESP8266 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);
Agrumino agrumino;

void setup() {
  agrumino.setup();
  Serial.begin(BAUD_RATE);
  pinMode(PIN_PUMP, OUTPUT);
  wifiSetup();
  thing["Pump"] << digitalPin(PIN_PUMP);


  thing[DEVICE_ID] >> [](pson & out) {
    out["Soil Moisture Raw (mV)"] =  agrumino.readSoilRaw();
    out["Soil Moisture Perc.(%)"] =  agrumino.readSoil();
    out["Temperature (C)"] = agrumino.readTempC();
    out["Lux"] = agrumino.readLux();
    out["Battery"] = agrumino.readBatteryVoltage();
    out["Battery Charge"] = agrumino.isBatteryCharging();
    out["Battery Level"] = agrumino.readBatteryLevel();
    out["Id Agrumino"] = agruminoName;

  };
}


void blinkLed() {

  agrumino.turnLedOn();
  delay(700);
  agrumino.turnLedOff();

}

void wifiSetup() {

  delay(100);
  Serial.print("Connecting to ");
  Serial.println(SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, SSID_PASSWORD);


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
    Serial.println(SSID);
    delay(100);
  }
}

void loop() {

  if (realTimeMonitoring == true) {
    agrumino.turnBoardOn();
    thing.handle();
    //thing.write_bucket(BUCKET_NAME, DEVICE_ID);////Write the data into your bucket
    agrumino.turnLedOn();

  }

  else if (realTimeMonitoring == false) {
    agrumino.turnBoardOn();
    thing.handle();
    thing.write_bucket(BUCKET_NAME, DEVICE_ID);////Write the data in your bucket
    delay(1000);
    blinkLed();
    Serial.print("By By.......sleep");
    WiFi.mode(WIFI_OFF);
    ESP.deepSleep(MIN_TO_MS * SLEEP_TIME_MIN);

  }
}
