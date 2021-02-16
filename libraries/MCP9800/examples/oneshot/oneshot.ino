/********************************************************
 * Enables the sensor's one-shot mode
 * This allows super-low current consuption as it only
 * wakes up when temperature is requested
 * 
 * This is recommended for battery powered devices where
 * the temperature is actively read by the MCU
 * 
 * Note that alerts are disabled in this mode
 ********************************************************/

#include <Wire.h>
#include "MCP9800.h"

MCP9800 tempSensor;

void setup()
{
	Serial.begin(57600);
	Serial.print(F("Initializing temperature sensor (one-shot): "));

	bool initialized = tempSensor.init();
	
	if (initialized)
	{
		Serial.println(F("ok"));

		Serial.println(F("Setting one-shot mode"));
		tempSensor.setOneShot(true);
	}
	else
	{
		Serial.println(F("failed. Check connections."));
		while (true) {};
	}
}

void loop()
{
	int16_t temp = tempSensor.readCelsius() / 16;
	Serial.print(F("Temperature: "));
	Serial.print(temp);
	Serial.println(F("c"));

	delay(2500);
}