/********************************************************
 * Simple example for reading the data from MCP9800
 ********************************************************/

#include <Wire.h>
#include "MCP9800.h"

MCP9800 tempSensor;

bool useFahrenheit = false;

void setup()
{
	Serial.begin(57600);
	Serial.print(F("Initializing temperature sensor: "));

	bool initialized = tempSensor.init();
	
	if (initialized)
	{
		Serial.println(F("ok"));
	}
	else
	{
		Serial.println(F("failed. Check connections."));
		while (true) {};
	}
}

void loop()
{
	// note: temperature is returned back multiplied by 16
	// this allows extraction of the fraction if needed
	int16_t temp;
	if (useFahrenheit) 
	{
		temp = tempSensor.readFahrenheit();
	}
	else
	{
		temp = tempSensor.readCelsius();
	}
	Serial.print(F("Temperature: "));
	Serial.print(temp);
	Serial.println(useFahrenheit ? F("f") : F("c"));

	delay(500);
}