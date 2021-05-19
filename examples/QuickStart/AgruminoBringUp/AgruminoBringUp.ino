/*
  AgruminoBringUp.ino - Sample project for Agrumino board using the Agrumino
  library.
  !!!WARNING!!! You need to program the board with option
  Tools->Erase Flash->All Flash Contents

  Created June 2020 by:
  Giuseppe Broccia giuseppe.broccia@lifely.cc
  Gabriele Foddis gabriele.foddis@lifely.cc  
  Modified May 2021 by:
  Giuseppe Broccia giuseppe.broccia@lifely.cc
  Gabriele Foddis gabriele.foddis@lifely.cc

  Program to test Agrumino board and FLASH Memory of ESP8266 Module.
  Select option following the guide on serial monitor.

  @see Agrumino.h for the documentation of the lib
*/

#include <Agrumino.h>

#define SECTOR_SIZE 4096u

flashMemory_t *PtrFlashMemory = NULL;

uint32_t crc32b = 0;
uint16_t tmp = 0;
int8_t currentIndex = 0;
uint8_t crc8b = 0;
uint8_t inByte = 0;

Agrumino agrumino;

void setup() {

  Serial.begin(115200);

  agrumino.setup();

  // Turn on the board to allow the usage of the Led
  agrumino.turnBoardOn();

  clearAndHome();

  EEPROM.begin(SECTOR_SIZE);
  PtrFlashMemory =
      (flashMemory_t *)EEPROM.getDataPtr(); // Assigning pointer address to
                                            // flash memory block dumped in RAM
  Serial.println("Memory assignement successful!");

  // Calculate checksum and validate memory area
  crc32b = calculateCRC32(PtrFlashMemory->Bytes, sizeof(Fields_t));
  crc8b = EEPROM.read(SECTOR_SIZE - 1); // Read crc at the end of sector
  Serial.println("Calculated CRC32=" + String(crc32b & 0xff) + " readed=" +
                 String(crc8b));
  if (((uint8_t)crc32b & 0xff) == crc8b) {
    Serial.println("CRC32 pass!");
    currentIndex = PtrFlashMemory->Fields.index;
  } else {
    Serial.println("CRC32 fail!");
    Serial.println(
        "-----------------------> You need to initilize the memory!");
  }

  /*Serial.println("sizeof(SensorData_t)=" + String(sizeof(SensorData_t)));
    Serial.println("sizeof(SensorDataVector_t)=" +
    String(sizeof(SensorDataVector_t)));
    Serial.println("sizeof(Fields_t)=" + String(sizeof(Fields_t)));
    Serial.println("sizeof(flashMemory_t)=" + String(sizeof(flashMemory_t)));
    Serial.println("currentIndex=" + String(currentIndex));*/

  Serial.println("\n\n**************WELCOME*************************");
  Serial.println("Data struct to EEPROM test program:");
  Serial.println("Enter 'i' to initialize memory");
  Serial.println("Enter '+' or '-' to increment or decrement the index of "
                 "structure sensor data");
  Serial.println(
      "Enter 'a' to add a data collection of sensors at the current index");
  Serial.println("Enter 'r' to read from flash data collection of sensors at "
                 "the current index");
  Serial.println("Enter 'c' to commit: store data from temporary RAM to FLASH");
  Serial.println("Enter 'v' to verify checksum memory");
  Serial.println("Enter 'd' to make dump of whole memory sector");
  Serial.println("Enter 's' to enter deepSpleep modality");
  Serial.println("Enter 'p' to place space from an action to another");
  Serial.println("Enter 'h' to print this menu");
}

void loop() {
  parse_serial();
}

void clearAndHome(void) {
  Serial.write(0x1b);     // ESC
  Serial.print(F("[2J")); // clear screen
  Serial.write(0x1b);     // ESC
  Serial.print(F("[H"));  // cursor to home
}

void parse_serial(void) {
  if (Serial.available() > 0) // Serial parser to speed up operations
  {
    inByte = Serial.read();
    switch (inByte) {
    case 'h':
      Serial.println("\n\n**************HELP*************************");
      Serial.println("Enter 'i' to initialize memory");
      Serial.println("Enter '+' or '-' to increment or decrement the index of "
                     "structure sensor data");
      Serial.println(
          "Enter 'a' to add a data collection of sensors at the current index");
      Serial.println("Enter 'r' to read from flash data collection of sensors "
                     "at the current index");
      Serial.println(
          "Enter 'c' to commit: store data from temporary RAM to FLASH");
      Serial.println("Enter 'v' to verify checksum memory");
      Serial.println("Enter 'd' to make dump of whole memory sector");
      Serial.println("Enter 's' to enter deepSpleep modality");
      Serial.println("Enter 'p' to place space from an action to another");
      Serial.println("Enter 'h' to print this menu\n\n");
      break;
    case 'i':
      // Memory initialization
      // Same of doing memset(PtrFlashMemory->Bytes, 0, sizeof(Fields_t)) ?
      Serial.println("RAM memory manual initialization");
      PtrFlashMemory->Fields.index = 0;
      currentIndex = PtrFlashMemory->Fields.index;
      for (uint8_t i = 0; i < N_SAMPLES; i++) {
        PtrFlashMemory->Fields.data.vector[i].temp = 0.0f;
        PtrFlashMemory->Fields.data.vector[i].soil = 0u;
        PtrFlashMemory->Fields.data.vector[i].lux = 0.0f;
        PtrFlashMemory->Fields.data.vector[i].batt = 0.0f;
        PtrFlashMemory->Fields.data.vector[i].battLevel = 0u;
        PtrFlashMemory->Fields.data.vector[i].usb = false;
        PtrFlashMemory->Fields.data.vector[i].charge = false;
      }
      Serial.println("\nATTENTION: you need to do the commit if you want to "
                     "save your data from temporary RAM to FLASH");
      Serial.println();
      break;
    case 'a':
      // Fills struct array with sensors data at the current index
      /*tmp = PtrFlashMemory->Fields.index;
        if(tmp>N_SAMPLES-1) { // Circular buffer behaviour
        tmp = 0;
        PtrFlashMemory->Fields.index = tmp;
        }*/
      Serial.println("*****   I'm writing at index:  " + String(currentIndex));
      PtrFlashMemory->Fields.data.vector[currentIndex].temp =
          agrumino.readTempC();
      PtrFlashMemory->Fields.data.vector[currentIndex].soil =
          agrumino.readSoil();
      PtrFlashMemory->Fields.data.vector[currentIndex].lux = agrumino.readLux();
      PtrFlashMemory->Fields.data.vector[currentIndex].batt =
          agrumino.readBatteryVoltage();
      PtrFlashMemory->Fields.data.vector[currentIndex].battLevel =
          agrumino.readBatteryLevel();
      PtrFlashMemory->Fields.data.vector[currentIndex].usb =
          agrumino.isAttachedToUSB();
      PtrFlashMemory->Fields.data.vector[currentIndex].charge =
          agrumino.isBatteryCharging();
      // PtrFlashMemory->Fields.index++; // Increment index

      Serial.println(
          "temperature:       " +
          String(PtrFlashMemory->Fields.data.vector[currentIndex].temp) + "°C");
      Serial.println(
          "soilMoisture:      " +
          String(PtrFlashMemory->Fields.data.vector[currentIndex].soil) + "%");
      Serial.println(
          "illuminance :      " +
          String(PtrFlashMemory->Fields.data.vector[currentIndex].lux) +
          " lux");
      Serial.println(
          "batteryVoltage :   " +
          String(PtrFlashMemory->Fields.data.vector[currentIndex].batt) + " V");
      Serial.println(
          "batteryLevel :     " +
          String(PtrFlashMemory->Fields.data.vector[currentIndex].battLevel) +
          "%");
      Serial.println(
          "isAttachedToUSB:   " +
          String(PtrFlashMemory->Fields.data.vector[currentIndex].usb));
      Serial.println(
          "isBatteryCharging: " +
          String(PtrFlashMemory->Fields.data.vector[currentIndex].charge));
      Serial.println("\nATTENTION: you need to do the commit if you want to "
                     "save your data from temporary RAM to FLASH");
      Serial.println();
      break;
    case 'r':
      // reading sensor data stored in flash at current index
      Serial.println("*****   I'm reading at index:  " + String(currentIndex));
      Serial.println(
          "temperature:       " +
          String(PtrFlashMemory->Fields.data.vector[currentIndex].temp) + "°C");
      Serial.println(
          "soilMoisture:      " +
          String(PtrFlashMemory->Fields.data.vector[currentIndex].soil) + "%");
      Serial.println(
          "illuminance :      " +
          String(PtrFlashMemory->Fields.data.vector[currentIndex].lux) +
          " lux");
      Serial.println(
          "batteryVoltage :   " +
          String(PtrFlashMemory->Fields.data.vector[currentIndex].batt) + " V");
      Serial.println(
          "batteryLevel :     " +
          String(PtrFlashMemory->Fields.data.vector[currentIndex].battLevel) +
          "%");
      Serial.println(
          "isAttachedToUSB:   " +
          String(PtrFlashMemory->Fields.data.vector[currentIndex].usb));
      Serial.println(
          "isBatteryCharging: " +
          String(PtrFlashMemory->Fields.data.vector[currentIndex].charge));
      break;
    case 'c':
      // Commit
      // Calculate checksum
      crc32b = calculateCRC32(PtrFlashMemory->Bytes, sizeof(Fields_t));
      crc8b = (uint8_t)crc32b & 0xff;
      Serial.println("Setting CRC32=" + String(crc8b));
      EEPROM.write(SECTOR_SIZE - 1, crc8b); // Put crc at the end of sector
      if (EEPROM.commit()) {
        Serial.println("EEPROM successfully committed");
      } else {
        Serial.println("ERROR! EEPROM commit failed");
      }
      break;
    case 'd':
      // Dump
      for (uint16_t i = 0; i < SECTOR_SIZE; i++) {
        Serial.println("EEPROM read addr=" + String(i) + " val=" +
                       String(EEPROM.read(i)));
        Serial.flush();
      }
      break;
    case 's':
      // DeepSleep
      agrumino.turnBoardOff();
      agrumino.deepSleepSec(30);
      break;
    case 'v':
      // Verify
      crc32b = calculateCRC32(PtrFlashMemory->Bytes, sizeof(Fields_t));
      crc8b = EEPROM.read(SECTOR_SIZE - 1); // Read crc at the end of sector
      Serial.println("Verifying data...");
      Serial.println("Calculated CRC32=" + String(crc32b & 0xff) + " readed=" +
                     String(crc8b));
      break;
    case '+':
      // Increment index
      /*currentIndex++;
        if(currentIndex>N_SAMPLES-1) {
        currentIndex=N_SAMPLES-1;
        }*/

      currentIndex++;
      if (currentIndex > N_SAMPLES - 1) { // Circular buffer behaviour
        currentIndex = 0;
        PtrFlashMemory->Fields.index = currentIndex;
      }
      Serial.println("Index=" + String(currentIndex));
      break;
    case '-':
      // Decrement index
      currentIndex--;
      if (currentIndex < 0) {
        currentIndex = 0;
      }
      Serial.println("Index=" + String(currentIndex));
      break;
    case 'p':
      Serial.println("\n\n");
      break;
    default:
      // Do nothing
      break;
    }
    Serial.flush();
  }
}

uint32_t calculateCRC32(const uint8_t *data, size_t length) {
  uint32_t crc = 0xffffffff;
  int i;
  Serial.println("length=" + String(length));
  for (i = 0; i < length; i++) {
    uint8_t c = data[i];
    for (uint32_t i = 0x80; i > 0; i >>= 1) {
      bool bit = crc & 0x80000000;
      if (c & i) {
        bit = !bit;
      }
      crc <<= 1;
      if (bit) {
        crc ^= 0x04c11db7;
      }
    }
  }
  return crc;
}
