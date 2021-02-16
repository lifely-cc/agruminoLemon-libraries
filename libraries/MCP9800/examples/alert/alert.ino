/********************************************************
 * Example usage of the alert pin
 * 
 * Note: this hasn't been tested as the hardware used for 
 * this library didn't have the alert pin connected
 ********************************************************/

#include <Wire.h>
#include "MCP9800.h"

MCP9800 tempSensor;

bool hasAlert;

void setup()
{
	Serial.begin(57600);
	Serial.print(F("Initializing temperature sensor (alert): "));

	bool initialized = tempSensor.init();

	if (initialized)
	{
		// configure alert settings
		tempSensor.setAlertLimits(20, 30);				// temperatures outside of 20-30c trigger an alert
		tempSensor.setAlertMode(MCP_INTERRUPT, true);	// intterupt based, active high alert
		tempSensor.setFaultQueue(MCP_2_FAULTS);			// allow a couple readings outside of the allowed range before triggering an alert

		// alert pin connected to arduino pin 2, use intterupt 0
		attachInterrupt(2, alertHandler, RISING);

		Serial.println(F("ok"));
	}
	else
	{
		Serial.println(F("failed"));
		while (true) {};
	}
}

void loop()
{
	if (hasAlert)
	{
		Serial.println(F("Alert!"));
		Serial.print(F("Temperature is "));
		Serial.print(tempSensor.readCelsius() / 16);
		Serial.println(F("c"));

		// We've handled the alert, now clear it and wait for another one..
		tempSensor.resetAlert();
		hasAlert = false;
	}
}

void alertHandler()
{
	// as this is an interrupt keep this nice and simple
	// processing happens in loop()
	hasAlert = true;
}