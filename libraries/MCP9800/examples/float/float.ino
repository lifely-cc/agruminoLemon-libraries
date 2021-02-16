/********************************************************
 * The library can do floating point conversion for you
 * to get the exact value directly.
 * 
 * However this is discouraged as it's slow and adds
 * ~1.5kb to the flash size
 ********************************************************/

#include <Wire.h>
#include "MCP9800.h"

MCP9800 tempSensor;

bool useFahrenheit = false;

void setup()
{
	Serial.begin(57600);
	Serial.print(F("Initializing temperature sensor (float): "));

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
	float temp;
	if (useFahrenheit) 
	{
		temp = tempSensor.readFahrenheitf() / 16;
	}
	else
	{
		temp = tempSensor.readCelsiusf() / 16;
	}
	Serial.print(F("Temperature: "));
	Serial.print(temp);
	Serial.println(useFahrenheit ? F("f") : F("c"));

	delay(500);
}