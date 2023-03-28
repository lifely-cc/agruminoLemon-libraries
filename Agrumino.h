/*
  Agrumino.cpp - Library for Agrumino Lemon board - Version 0.2 for Board R3
  Created by giuseppe.broccia@lifely.cc on October 2017.
  
  Updated on March 2023 for Lifely Agrumino Lemon rev4 and rev5 by:  
  giuseppe.broccia@lifely.cc
  gabriele.foddis@lifely.cc
  

  Future developements:
    - Watering with time duration
    - Add Serial logs in the lib
    - Expose PCA9536 GPIO 2-3-4 Pins
    - Save and read soil water/air values from EEPROM
*/

#ifndef Agrumino_h
#define Agrumino_h

#include "Arduino.h"
#include "EEPROM.h"


#define N_SAMPLES 4u  // Number of objects with samples to store

// Datatypes for data sensors
typedef struct
{
  float temp;
  uint16_t soil;
  float lux;
  float batt;
  uint16_t battLevel;
  boolean usb;
  boolean charge;
  unsigned long seconds;
}SensorData_t;

typedef struct
{
  SensorData_t  vector[N_SAMPLES];
}SensorDataVector_t;

typedef struct
{
  uint16_t index;
  SensorDataVector_t data;
}Fields_t;

typedef union flashMemory
{
  Fields_t Fields;
  uint8_t Bytes[sizeof(Fields_t)];
}flashMemory_t;

// Internal map of the PCA9536 pin_t typedef. See PCA9536.h
typedef enum:byte {
  // GPIO_0 = 0, Green led, manages via specific functions.
  GPIO_1 = 1, // External GPIO pin connected to GPIO1 of the PCA9536 GPIO expander
  GPIO_2 = 2, // External GPIO pin connected to GPIO2 of the PCA9536 GPIO expander
  LIV_1  = 3  // External LIV1 pin connected to GPIO3 of the PCA9536 GPIO expander
} gpio_pin;

// Internal map of the PCA9536 mode_t typedef. See PCA9536.h
typedef enum:byte {
   GPIO_OUTPUT = 0,
   GPIO_INPUT  = 1
} gpio_mode;

class Agrumino {

  public:
    // Constructor
    Agrumino();
    void setup();
    void deepSleepSec(unsigned int sec);
    
    // Public methods GPIO
    void turnWateringOn();
    void turnWateringOff(); // Default Off
    boolean isButtonPressed();
    boolean isAttachedToUSB(); 
    boolean isBatteryCharging();
    boolean isBoardOn();
    void turnBoardOn(); // Also call initBoard()
    void turnBoardOff(); 
    float readBatteryVoltage(); 
    unsigned int readBatteryLevel();
    
    // Public methods I2C
    float readTempC();
    float readTempF();
    void turnLedOn();
    void turnLedOff(); // Default Off
    boolean isLedOn();
    void toggleLed();
    unsigned int readSoil();
    unsigned int readSoilRaw();
    void calibrateSoilWater();
    void calibrateSoilAir();
    void calibrateSoilWater(unsigned int rawValue);
    void calibrateSoilAir(unsigned int rawValue);
    float readLux();
    void setGPIOMode(gpio_pin pin, gpio_mode mode);
    gpio_mode getGPIOMode(gpio_pin pin);
    unsigned int readGPIO(gpio_pin pin);
    void writeGPIO(gpio_pin pin, unsigned int value);
 
  private:
    // Private methods
    void setupGpioModes();
    void printLogo();
    void initBoard();
    void initWire();
    void initGPIOExpander();
    void initTempSensor();
    void initSoilSensor();
    void initLuxSensor();
    float readBatteryVoltageSingleShot(); 
    boolean checkBattery();
};

#endif

