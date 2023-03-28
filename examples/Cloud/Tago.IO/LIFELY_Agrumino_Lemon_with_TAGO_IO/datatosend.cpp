/*
  Created by gabriele.foddis@lifely.cc on December 2022.
  for LIFELY_Agrumino_Lemon_with_TAGO_IO.ino
  
  Latest changes and updates 03/2023 by:  
  Gabriele Foddis gabriele.foddis@lifely.cc
  
  for a better experience use Visual Studio Code and Platformio

*/

#include <Arduino.h>
//#define ARDUINOJSON_USE_LONG_LONG 0 //set 1 if necessary
//#define ARDUINOJSON_USE_DOUBLE 0 //set 1 if necessary
#include <ArduinoJson.h>
#include "C:\Users\gabri\Documents\Arduino\Lifely_Agrumino_Lemon_Tago_IO\datatosend.h" ///add your path for datasotosend.h
unsigned int SLEEP_TIME_MIN = 5;//Time in minute, max value is 60 minutes min value is 1
const char* networkName = "PlumCake"; // your SSID network
const char* networkPassword = "ImVeryHappy"; // SSID password 
const char* serverName = "http://api.tago.io/data"; //don't edit this
String apiTagoDeviceToken = "cd8ccc84-1b8a-4fe1-bb85-0terf14c2cc1"; //Your TAGO.IO Token
long int alwaysOnTime = 5000; ///Set this time in millisecond for your "always on delay" (default 5000 (5 seconds)) min value is 1
boolean isBatteryCharging;
boolean alwaysON = false; // Set true if you need real time monitor data, default is false.
double temperature;
double lux;
double batteryVoltage;
unsigned int soilMoisture;
unsigned int batteryLevel;
String outputData;

void dataToSend(){

StaticJsonDocument<1024> data;

JsonObject data_T = data.createNestedObject();
data_T["variable"] = "Temperature";
data_T["value"] = temperature;
data_T["group"] = "1";
data_T["unit"] = "°C";

JsonObject data_SM = data.createNestedObject();
data_SM["variable"] = "SoilMoisture";
data_SM["value"] = soilMoisture;
data_SM["group"] = "1";
data_SM["unit"] = "%";

JsonObject data_L = data.createNestedObject();
data_L["variable"] = "Lux";
data_L["value"] = lux;
data_L["group"] = "1";
data_L["unit"] = "Lux";

JsonObject data_BL = data.createNestedObject();
data_BL["variable"] = "BatteryLevel";
data_BL["value"] = batteryLevel;
data_BL["group"] = "1";
data_BL["unit"] = "%";

JsonObject data_BV = data.createNestedObject();
data_BV["variable"] = "BatteryVoltage";
data_BV["value"] = batteryVoltage;
data_BV["group"] = "1";
data_BV["unit"] = "V";

JsonObject data_BC = data.createNestedObject();
data_BC["variable"] = "BatteryCharging";
data_BC["value"] = isBatteryCharging;
data_BC["group"] = "1";
data_BC["unit"] = "T/F";

delay(300);
serializeJsonPretty(data, outputData);
//Serial.print("json is: " + outputData); ///print to debug json data
}
