/*LifelyAgruminoLemonForTAGO_IO- 
  Created by Gabriele Foddis on 12/2022, last update 03/2023
  gabriele.foddis@lifely.cc
  With this Sketch you can send data from your Fantastic Lifely Agrumino Lemon Device in the Tago.IO Cloud Platform
  For better experience use this with Visual Studio Code and Platformio. You can also use it with Arduino Ide.
  Important, edit data in datatosend.ccp. You can configure the sending timing and you can use it in always-on mode as well 
  Build your amazing dashboard in the Tago.IO Cloud
  This sketch need a internet connection and an account from www.tago.io
  Find Lifely Agrumino Lemon (REV4 AND REV5) in www.lifely.cc */

#include <Arduino.h>
//#define ARDUINOJSON_USE_LONG_LONG 0 //set 1 if necessary
//#define ARDUINOJSON_USE_DOUBLE 0 //set 1 if necessary
#include <ArduinoJson.h> 
#include "datatosend.h"  /// before burn and upload edit datatosend.cpp data
#include <Agrumino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#define SERIAL_BAUD 115200
#define MIN_TO_MS (1000000 * 60)
#define HTTPCLIENT_1_1_COMPATIBLE 1

Agrumino agrumino;

void readDataFromDevice(){
  isBatteryCharging = agrumino.isBatteryCharging();
  temperature = agrumino.readTempC();
  soilMoisture = agrumino.readSoil();
  lux = agrumino.readLux();
  batteryVoltage = agrumino.readBatteryVoltage();
  batteryLevel = agrumino.readBatteryLevel();
  delay(200);
}


void blinkAndSleep() {
  agrumino.turnLedOn();
  delay(200);
  agrumino.turnLedOff();
  if(alwaysON == false && SLEEP_TIME_MIN > 1 ){ 
  Serial.println("I sleep for: "+String(SLEEP_TIME_MIN) + " minutes...");
  ESP.deepSleep(MIN_TO_MS * SLEEP_TIME_MIN);
  }
  if(alwaysON == false && SLEEP_TIME_MIN == 1){ 
  Serial.println("I sleep for: "+String(SLEEP_TIME_MIN) + " minute...");
  ESP.deepSleep(MIN_TO_MS * SLEEP_TIME_MIN);
  }
  else if (alwaysON == true)
  Serial.println("I read and send data every : "+ String(alwaysOnTime) + "milliseconds"); 
  delay(alwaysOnTime);

}

void setup() {
  Serial.begin(SERIAL_BAUD);
  agrumino.setup();
  agrumino.turnBoardOn();
  WiFi.begin(networkName, networkPassword);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Connected to WiFi network ("+String(networkName)+") with IP Address: ");
  Serial.print(WiFi.localIP());   
  agrumino.turnLedOn();
  delay(200);
  agrumino.turnLedOff();
}

void loop() {   
  readDataFromDevice();
  dataToSend();
  WiFiClient tagoClient;
  delay(100);
  HTTPClient httpTagoClient; 
  delay(100);
  httpTagoClient.begin(tagoClient, serverName);
  httpTagoClient.addHeader("Content-Type", "application/json; charset=UTF-8");     
  httpTagoClient.addHeader("Device-Token", apiTagoDeviceToken);
  int httpCode = httpTagoClient.POST(outputData);
  delay(200);
  //Serial.println("Data Sent is: " + String(output)); //print JsonData

    if (httpCode > 0) {
      Serial.printf("[HTTP] POST... code: %d\n", httpCode);
      if (httpCode == HTTP_CODE_OK) {
        const String& payload = httpTagoClient.getString();
        Serial.println("I Received this payload:\n<<");
        Serial.println(payload);
        Serial.println(">>");
      }
    } else {
      Serial.printf("[HTTP] POST... failed, error: %s\n", httpTagoClient.errorToString(httpCode).c_str());
    }
      Serial.println("HTTP Response code: ");
      Serial.println(httpCode);    
    delay(100);      
    httpTagoClient.end();
    tagoClient.stopAll();
    Serial.println(ESP.getFreeHeap());
    blinkAndSleep(); 
  }
