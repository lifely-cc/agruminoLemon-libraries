/*
  Agrumino.cpp - Library for Agrumino Lemon board - Version 0.2 for Board R3
  Created by giuseppe.broccia@lifely.cc on October 2017.
  
  Updated on March 2023 for Lifely Agrumino Lemon rev4 and rev5 by:
  giuseppe.broccia@lifely.cc
  gabriele.foddis@lifely.cc
  
  For details @see Agrumino.h
*/

#include "Agrumino.h"
#include <Wire.h>
#include "libraries/MCP9800/MCP9800.cpp"
#include "libraries/PCA9536/PCA9536.cpp" // PCA9536.h lib has been modified (REG_CONFIG renamed to REG_CONFIG_PCA) to avoid name clashing with mcp9800.h
#include "libraries/MCP3221/MCP3221.cpp"

// PINOUT Agrumino        Implemented
#define PIN_SDA          2 // [X] BOOT: Must be HIGH at boot
#define PIN_SCL         14 // [X] 
#define PIN_PUMP        12 // [X] 
#define PIN_BTN_S1       4 // [X] Same as Internal WT8266 LED
#define PIN_USB_DETECT   5 // [X] 
#define PIN_MOSFET      15 // [X] BOOT: Must be LOW at boot
#define PIN_BATT_STAT   13 // [X] 
#define PIN_LEVEL        0 // [ ] BOOT: HIGH for Running and LOW for Program

// Addresses I2C sensors       Implemented
#define I2C_ADDR_SOIL      0x4D // [X] TODO: Save air and water values to EEPROM
#define I2C_ADDR_LUX       0x44 // [X] 
#define I2C_ADDR_TEMP      0x48 // [X] 
#define I2C_ADDR_GPIO_EXP  0x41 // [-] BOTTOM LED OK, TODO: GPIO 2-3-4 
#define DELAY_ADDR_LUX      110 // delay for new Lux sensor

////////////
// CONFIG //
////////////

// GPIO Expander
#define IO_PCA9536_LED                IO0 // Bottom Green led is connected to GPIO0 of the PCA9536 GPIO expander
// Soil
#define DEFAULT_MV_SOIL_RAW_AIR      2750 // Millivolt reading of a Lifely R4 Capacitive Flat, sensor in the air
#define DEFAULT_MV_SOIL_RAW_WATER    1700 // Millivolt reading of a Lifely R4 Capacitive Flat, sensor immersed by 3/4 in water
// Battery
#define BATTERY_MV_LEVEL_0           3500 // Voltage in millivolt of a fully discharged battery (Safe value, cut-off of a LIR2450 is 2.75 V)
#define BATTERY_MV_LEVEL_100         4200 // Voltage in millivolt of a fully charged battery
#define BATTERY_VOLT_DIVIDER_Z1      1800 // Value of the Z1(R25) resistor in the voltage divider used for read the batt voltage
#define BATTERY_VOLT_DIVIDER_Z2       424 // Value of the Z2(R26) resistor (470 original). Adjusted considering the ADC internal resistance
#define BATTERY_VOLT_SAMPLES           20 // Number of reading (samples) needed to calculate the battery voltage

///////////////
// Variables //
///////////////

MCP9800 mcpTempSensor;
PCA9536 pcaGpioExpander;
MCP3221 mcpSoilSensor(I2C_ADDR_SOIL);
unsigned int millivoltSoilRawAir = DEFAULT_MV_SOIL_RAW_AIR;
unsigned int millivoltSoilRawWater = DEFAULT_MV_SOIL_RAW_WATER;

/////////////////
// Constructor //
/////////////////

Agrumino::Agrumino() {
}

void Agrumino::setup() {
  setupGpioModes();
  printLogo();
  // turnBoardOn(); // Decomment to have the board On by Default
}

void Agrumino::deepSleepSec(unsigned int sec) {
  if (sec > 4294) {
    // ESP.deepSleep argument is an unsigned int, so the max allowed walue is 0xffffffff (4.294.967.295).
    sec = 4294;
    Serial.println("Warning: deepSleep can be max 4294 sec (~71 min). Value has been constrained!");
  }

  Serial.print("\nGoing to deepSleep for ");
  Serial.print(sec);
  Serial.println(" seconds... (ー。ー) zzz\n");
  ESP.deepSleep(sec * 1000000L); // microseconds
  // deepSleep is not executed istantly to this delay fix the issue
  delay(1000);
}

/////////////////////////
// Public methods GPIO //
/////////////////////////

boolean Agrumino::isAttachedToUSB() {
  return digitalRead(PIN_USB_DETECT) == HIGH;
}

// Return true if the battery is in charging state
boolean Agrumino::isBatteryCharging() {
  return digitalRead(PIN_BATT_STAT) == LOW;
}

boolean Agrumino::isButtonPressed() {
  return digitalRead(PIN_BTN_S1) == LOW;
}

boolean Agrumino::isBoardOn() {
  return digitalRead(PIN_MOSFET) == HIGH;
}

void Agrumino::turnBoardOn() {
  if (!isBoardOn()) {
    Serial.println("Turning board on...");
    digitalWrite(PIN_MOSFET, HIGH);
    delay(5); // Ensure that the ICs are booted up properly
    initBoard();
    checkBattery();
  }
  else {
    Serial.println("Toggling board off/on...");
    digitalWrite(PIN_MOSFET, LOW);
    delay(5);
    digitalWrite(PIN_MOSFET, HIGH);
    delay(5);
    initBoard();
    checkBattery();
  }
}

void Agrumino::turnBoardOff() {
  digitalWrite(PIN_MOSFET, LOW);
}

void Agrumino::turnWateringOn() {
  digitalWrite(PIN_PUMP, HIGH);
}

void Agrumino::turnWateringOff() {
  digitalWrite(PIN_PUMP, LOW);
}

////////////////////////
// Public methods I2C //
////////////////////////

float Agrumino::readTempC() {
  return mcpTempSensor.readCelsiusf();
}

float Agrumino::readTempF() {
  return mcpTempSensor.readFahrenheitf();
}

void Agrumino::turnLedOn() {
  pcaGpioExpander.setState(IO_PCA9536_LED, IO_HIGH);
}

void Agrumino::turnLedOff() {
  pcaGpioExpander.setState(IO_PCA9536_LED, IO_LOW);
}

boolean Agrumino::isLedOn() {
  return pcaGpioExpander.getState(IO_PCA9536_LED) == 1;
}

void Agrumino::toggleLed() {
  pcaGpioExpander.toggleState(IO_PCA9536_LED);
}

void Agrumino::calibrateSoilWater() {
  millivoltSoilRawWater = readSoilRaw();
}

void Agrumino::calibrateSoilAir() {
  millivoltSoilRawAir = readSoilRaw();
}

void Agrumino::calibrateSoilWater(unsigned int rawValue) {
  millivoltSoilRawWater = rawValue;
}

void Agrumino::calibrateSoilAir(unsigned int rawValue) {
  millivoltSoilRawAir = rawValue;
}

unsigned int Agrumino::readSoil() {
  unsigned int soilRaw = readSoilRaw();
  soilRaw = constrain(soilRaw, millivoltSoilRawWater, millivoltSoilRawAir);
  return map(soilRaw, millivoltSoilRawAir, millivoltSoilRawWater, 0, 100);
}

float Agrumino::readLux() {
  // Logic for Light-to-Digital Output Sensor ISL29003
  Wire.beginTransmission(I2C_ADDR_LUX);
  Wire.write(0x02); // Data registers are 0x02->LSB and 0x03->MSB
  Wire.endTransmission();
  Wire.requestFrom(I2C_ADDR_LUX, 2); // Request 2 bytes of data
  unsigned int data;
  if (Wire.available() == 2) {
    byte lsb = Wire.read();
    byte  msb = Wire.read();
    data = (msb << 8) | lsb;
  } else {
    Serial.println("readLux Error!");
    return 0;
  }
  // Convert the data from the ADC to lux
  // 0-64000 is the selected range of the ALS (Lux)
  // 0-65536 is the selected range of the ADC (16 bit)
  return (64000.0 * (float) data) / 65536.0;
}

float Agrumino::readBatteryVoltage() {
  float voltSum = 0.0;
  for (int i = 0; i < BATTERY_VOLT_SAMPLES; ++i) {
    voltSum += readBatteryVoltageSingleShot();
  }
  float volt = voltSum / BATTERY_VOLT_SAMPLES;
  // Serial.println("readBatteryVoltage: " + String(volt) + " V");
  return volt;
}

unsigned int Agrumino::readBatteryLevel() {
  float voltage = readBatteryVoltage();
  unsigned int milliVolt = (int) (voltage * 1000.0);
  milliVolt = constrain(milliVolt, BATTERY_MV_LEVEL_0, BATTERY_MV_LEVEL_100);
  return map(milliVolt, BATTERY_MV_LEVEL_0, BATTERY_MV_LEVEL_100, 0, 100);
}

// set the given gpio_pin to the given gpio_mode
// gpio: { GPIO_1, GPIO_2, LIV_1 }
// mode: { GPIO_INPUT, GPIO_OUTPUT }
void Agrumino::setGPIOMode(gpio_pin pin, gpio_mode mode) {
  // values are mapped to PCA9536 values
  pcaGpioExpander.setMode((pin_t) pin, (Pca9536::mode_t) mode); 
}

gpio_mode Agrumino::getGPIOMode(gpio_pin pin) {
  // values are mapped to/from PCA9536 values
  return (gpio_mode) pcaGpioExpander.getMode((pin_t) pin);
}

unsigned int Agrumino::readGPIO(gpio_pin pin) {
  if (getGPIOMode(pin) == GPIO_INPUT) {
    // values are mapped to/from PCA9536 values
    return pcaGpioExpander.getState((pin_t) pin) == IO_LOW ? LOW : HIGH; 
  } else {
    Serial.println("Error! gpio pin [" + String(pin) + "] is not set as OUTPUT, unable to write");
	return 0;
  }
}

// write to the given GPIO "pin" the given "value"
// pin: { GPIO_1, GPIO_2, LIV_1 }
// value: { LOW, HIGH }
void Agrumino::writeGPIO(gpio_pin pin, unsigned int value) {
  // Check param "value"
  if (value != LOW && value != HIGH) {
    Serial.println("Error! Invalid value [" + String(value) + "]. Allowed values are { LOW, HIGH }");
    return;
  }
  if (getGPIOMode(pin) == GPIO_OUTPUT) {
    // values are mapped to/from PCA9536 values
    pcaGpioExpander.setState((pin_t) pin, value == LOW ? IO_LOW : IO_HIGH);
  } else {
    Serial.println("Error! gpio pin [" + String(pin) + "] is not set as OUTPUT, unable to write");
  }
}

/////////////////////
// Private methods //
/////////////////////

void Agrumino::initGPIOExpander() {
  Serial.print("initGPIOExpander → ");
  byte result = pcaGpioExpander.ping();;
  if (result == 0) {
    pcaGpioExpander.reset();
    pcaGpioExpander.setMode((pin_t) GPIO_1, IO_INPUT);
    pcaGpioExpander.setMode((pin_t) GPIO_2, IO_INPUT);
    pcaGpioExpander.setMode((pin_t) LIV_1, IO_INPUT);
    pcaGpioExpander.setMode(IO_PCA9536_LED, IO_OUTPUT); // Back green LED
    pcaGpioExpander.setState(IO_PCA9536_LED, IO_LOW);
    Serial.println("OK");
  } else {
    Serial.println("FAIL!");
  }
}

void Agrumino::initTempSensor() {
  Serial.print("initTempSensor   → ");
  boolean success = mcpTempSensor.init();
  if (success) {
    mcpTempSensor.setResolution(MCP_ADC_RES_11); // 11bit (0.125c)
    mcpTempSensor.setOneShot(true);
    Serial.println("OK");
  } else {
    Serial.println("FAIL!");
  }
}

void Agrumino::initSoilSensor() {
  Serial.print("initSoilSensor   → ");
  byte response = mcpSoilSensor.ping();;
  if (response == 0) {
    mcpSoilSensor.reset();
    mcpSoilSensor.setSmoothing(EMAVG);
    mcpSoilSensor.setVref(3300); // This will make the reading of the MCP3221 voltage accurate.
    Serial.println("OK");
  } else {
    Serial.println("FAIL!");
  }
}

void Agrumino::initLuxSensor() {
  // Logic for Light-to-Digital Output Sensor ISL29003
  Serial.println("Lux Ver. " + String(DELAY_ADDR_LUX));
  Serial.print("initLuxSensor    → ");
  Wire.beginTransmission(I2C_ADDR_LUX);
  byte result = Wire.endTransmission();
  if (result == 0) {
    Wire.beginTransmission(I2C_ADDR_LUX);
    Wire.write(0x00); // Select "Command-I" register
    Wire.write(0xA0); // Select "ALS continuously" mode
    Wire.endTransmission();
    Wire.beginTransmission(I2C_ADDR_LUX);
    Wire.write(0x01); // Select "Command-II" register
    Wire.write(0x03); // Set range = 64000 lux, ADC 16 bit
    Wire.endTransmission();
    Serial.println("OK");
  } else {
    Serial.println("FAIL!");
  }
}

unsigned int Agrumino::readSoilRaw() {
  return mcpSoilSensor.getVoltage();
}

float Agrumino::readBatteryVoltageSingleShot() {
  float z1 = (float) BATTERY_VOLT_DIVIDER_Z1;
  float z2 = (float) BATTERY_VOLT_DIVIDER_Z2;
  float vread = (float) analogRead(A0) / 1024.0; // RAW Value from the ADC, Range 0-1V
  float volt = ((z1 + z2) / z2) * vread;
  return volt;
}

// Return true if the battery is ok or if powered via USB
// Return false and put the ESP to sleep if not
boolean Agrumino::checkBattery() {
  if (isAttachedToUSB()) {
    return true;
  } else if (readBatteryLevel() > 0) {
    return true;
  } else {
    Serial.print("\nturnBoardOn Fail! Battery is too low!!!\n");
    deepSleepSec(3600); // Sleep for 1 hour
    return false;
  }
}

void Agrumino::initWire() {
  Wire.begin(PIN_SDA, PIN_SCL);
}

void Agrumino::initBoard() {
  initWire();
  initLuxSensor();  // Boot time depends on the selected ADC resolution (16bit first reading after ~90ms)
  initSoilSensor(); // First reading after ~30ms
  initTempSensor(); // First reading after ~?ms
  initGPIOExpander(); // First operation after ~?ms
  delay(DELAY_ADDR_LUX); // Ensure that the ICs are init properly (new lux sensor work in RV4 and RV5)
}

void Agrumino::setupGpioModes() {
  pinMode(PIN_PUMP, OUTPUT);
  pinMode(PIN_BTN_S1, INPUT_PULLUP);
  pinMode(PIN_USB_DETECT, INPUT);
  pinMode(PIN_MOSFET, OUTPUT);
  pinMode(PIN_BATT_STAT, INPUT);
}

void Agrumino::printLogo() {
  Serial.println("\n\n\n");
  Serial.println(" ____________________________________________");
  Serial.println("/\\      _                       _            \\");
  Serial.println("\\_|    /_\\  __ _ _ _ _  _ _ __ (_)_ _  ___   |");
  Serial.println("  |   / _ \\/ _` | '_| || | '  \\| | ' \\/ _ \\  |");
  Serial.println("  |  /_/ \\_\\__, |_|  \\_,_|_|_|_|_|_||_\\___/  |");
  Serial.println("  |        |___/         Lemon By Lifely.cc  |");
  Serial.println("  |  ________________________________________|_");
  Serial.println("  \\_/_________________________________________/");
  Serial.println("");
}
