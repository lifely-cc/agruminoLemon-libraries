#ifndef MCP9800_H_
#define MCP9800_H_

#if (ARDUINO >= 100)
#include <Arduino.h>
#include <Wire.h>
#else
#include <stdlib.h>
#include <avr/io.h>
#include "i2cmaster.h"
#endif

#include "MCP9800Reg.h"

#define MCP9800_ADDRESS		0x48

typedef enum {
	MCP_ADC_RES_9 = 0,		/*!< 9bit  (0.5c) */
	MCP_ADC_RES_10,			/*!< 10bit (0.25c) */
	MCP_ADC_RES_11,			/*!< 11bit (0.125c) */
	MCP_ADC_RES_12			/*!< 12bit (0.0625c) */
} mcp9800_adc_resolution_t;

typedef enum {
	MCP_COMPARATOR = 0,		/*!< Alert pin is kept active (see setAlertPolarity()) when temperature greater than alert value */
	MCP_INTERRUPT			/*!< Alert pin asserted and reset from a microcontroller using resetAlert() */
} mcp9800_alert_mode_t;

typedef enum {
	MCP_0_FAULTS = 0,		/*!< Immediately trigger alert */
	MCP_1_FAULT,			/*!< Allow 1 fault before alert */
	MCP_2_FAULTS,			/*!< Allow 2 faults before alert */
	MCP_4_FAULTS,			/*!< Allow 4 faults before alert */
	MCP_6_FAULTS,			/*!< Allow 6 faults before alert */
} mcp9800_fault_queue_t;

class MCP9800
{

	public:
		/**
		 * @brief   Initializes the chip
		 * 
		 * @return  true if chip was succesfully initialized
		 */
		bool init(bool initWire);

		/**
		 * @brief   Toggles one shot mode on/off
		 * @details One shot keeps the chip powered down and only wakes up when temperature is requested. 
		 *          Consumes 0.1uA between reads and wakes up for 200uA for 30ms when reading temp
		 * 
		 * @param   enabled true to enable one shot mode
		 */
		 void setOneShot(bool enabled);

		/**
		 * @brief   ADC conversion resolution
		 * @details Sets conversion resolution from 9 to 12 bit, 0.5-0.0625c accuracy. 
		 *          Higher resolutions take longer to acquire
		 * 
		 * @param   resolution ADC resolution to use
		 * 
		 * @see     mcp9800_adc_resolution_t
		 */
		void setResolution(mcp9800_adc_resolution_t resolution);

		/**
		 * @brief   Number of temperatures outside of the temperature range before triggering an alarm
		 * @details The fault queue feature can be used as a filter to lessen 
		 *          the probability of spurious activation of the ALERT pin
		 */
		void setFaultQueue(mcp9800_fault_queue_t numFaults);

		/**
		 * @brief   Shut down the chip
		 * 
		 * @param   shutdown True to shutdown active reading
		 */
		void setShutdown(bool shutdown);

		/**
		 * @brief   Sets alert pin behavior
		 * @details Sets the pin to either continuously keep the alert pin active or only assert it until an MCU has reacted to it and cleared it using resetAlert()
		 * @see     resetAlert()
		 * @see     setTemperatureAlarmLimits()
		 * 
		 * @param   alertMode MCP_COMPARATOR: pin remains active until temperature drops under hysterisis. MCP_INTERRUPT: pin can be reset using resetAlert()
		 * @param   polarity Alarm pin polarity. High for active high, low for active low
		 */
		void setAlertMode(mcp9800_alert_mode_t alertMode, bool polarity);

		/**
		 * @brief   Temperature alarm limits
		 * @details Sets temperature alarm limits (in celsius)
		 * 
		 * @param   lower Lowest allowed temperature
		 * @param   upper Highest allowed temperature
		 */
		void setAlertLimits(int8_t lower, int8_t upper);

		/**
		 * @brief   Resets alarm if alert mode is set to MCP_INTERRUPT. In MCP_INTERRUPT mode a new alert is not triggered until this is called.
		 */
		void resetAlert();

		/**
		 * @brief   Reads raw temperature data
		 * @details Returns number of bits as defined in setResolution()
		 * @see     setResolution()
		 * @return  Raw temperature reading
		 */
		int16_t readRawData();

		/**
		 * @brief   Reads temperature in celcius * 16
		 * @return  Current temperature in celsius. Divide by 16 to get actual reading
		 */
		int16_t readCelsius();

		/**
		 * @brief   Reads temperature in fahrenheit * 16. There is some error due to integer precision
		 * @return  Current temperature in fahrenheit. Divide by 16 to get actual reading
		 */
		int16_t readFahrenheit();

		/**
		 * @brief   Reads temperature in celcius as a float
		 * @return  Current temperature as floating point in celsius
		 */
		float readCelsiusf();

		/**
		 * @brief   Reads temperature in fahrenheit as a float
		 * @return  Current temperature as floating point in celsius
		 */
		float readFahrenheitf();

	private:
		void write(uint8_t reg, uint8_t *data, int8_t len);
		void read(uint8_t reg, uint8_t *buffer, int8_t numBytes);
		uint8_t adc;
};


#endif // MCP9800_H_