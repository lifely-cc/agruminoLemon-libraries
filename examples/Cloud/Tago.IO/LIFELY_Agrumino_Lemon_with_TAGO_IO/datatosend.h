/*
  Created by gabriele.foddis@lifely.cc on December 2022.
  for LIFELY_Agrumino_Lemon_with_TAGO_IO.ino
  
  Latest changes and updates 03/2023 by:  
  Gabriele Foddis gabriele.foddis@lifely.cc
  
  for a better experience use Visual Studio Code and Platformio

*/

#ifndef DATATOSEND_H
#define DATATOSEND_H

#include <Arduino.h>

extern unsigned int SLEEP_TIME_MIN;
extern const char* networkName; 
extern const char* networkPassword;
extern const char* serverName;
extern String apiTagoDeviceToken; 
extern long int alwaysOnTime; 
extern boolean isBatteryCharging;
extern boolean alwaysON; 
extern long int alwaysOnTime; 
extern double temperature;
extern double lux;
extern double batteryVoltage;
extern unsigned int soilMoisture;
extern unsigned int batteryLevel;
extern String outputData;

void dataToSend();

#endif
