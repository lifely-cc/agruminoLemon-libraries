/*
  AgruminoThingSpeakHttpPost.ino - Sample project for Agrumino board using the
  Agrumino library.
  !!!WARNING!!! You need to program the board with option Tools->Erase
  Flash->All Flash Contents

  Created by Paolo Paolucci on May 2018.
  Modified May 2021 by:
  Giuseppe Broccia giuseppe.broccia@lifely.cc
  Gabriele Foddis gabriele.foddis@lifely.cc

  This sketch Sketch save sensor data of Agrumino board on FLASH
  and after 4 cycles transmits data to Thing Speak
  with 4 different HTTP POST.

  @see Agrumino.h for the documentation of the lib
*/

#include "Agrumino.h"         // Our super cool lib ;)
#include <ArduinoJson.h>      // https://github.com/bblanchon/ArduinoJson
#include <DNSServer.h>        // Installed from ESP8266 board
#include <ESP8266WebServer.h> // Installed from ESP8266 board
#include <ESP8266WiFi.h>      // https://github.com/esp8266/Arduino
#include <WiFiManager.h>      // https://github.com/tzapu/WiFiManager

// Time to sleep in second between the readings/data sending
#define SLEEP_TIME_SEC 10

// The size of the flash sector we want to use to store samples (do not modify).
#define SECTOR_SIZE 4096u

// ThingSpeak information.
// To update more fields, increase NUM_FIELDS number and add a field label below.
#define NUM_FIELDS 5 
#define TEMPERATURA 2
#define UMIDITA 1
#define LUMINOSITA 3
#define TENSIONE 4
#define LIVELLOBATT 5
#define THING_SPEAK_ADDRESS "api.thingspeak.com"
String writeAPIKey = "XXXXXXXXXXXXXXXX"; // Change this to your channel Write API key.
#define TIMEOUT 5000    // Timeout for server response.

// Our super cool lib
Agrumino agrumino;

// Used to create TCP connections and make Http calls
WiFiClient client;

flashMemory_t *PtrFlashMemory = NULL;

uint32_t crc32b = 0;
uint16_t currentIndex = 0;
uint8_t crc8b = 0;

void setup() {
  Serial.begin(115200);

  // Setup our super cool lib
  agrumino.setup();

  // Turn on the board to allow the usage of the Led
  agrumino.turnBoardOn();

  // WiFiManager
  // Local intialization. Once its business is done, there is no need to keep it
  // around
  WiFiManager wifiManager;

  // If the S1 button is pressed for 5 seconds then reset the wifi saved
  // settings.
  if (checkIfResetWiFiSettings()) {
    wifiManager.resetSettings();
    Serial.println("Reset WiFi Settings Done!");
    // Blink led for confirmation :)
    blinkLed(100, 10);
  }

  // Set timeout until configuration portal gets turned off
  // useful to make it all retry or go to sleep in seconds
  wifiManager.setTimeout(180); // 3 minutes

  // Customize the web configuration web page
  wifiManager.setCustomHeadElement("<h1>Agrumino</h1>");

  // Fetches ssid and pass and tries to connect
  // If it does not connect it starts an access point with the specified name
  // here and goes into a blocking loop awaiting configuration
  String ssidAP = "Agrumino-AP-" + getChipId();
  if (!wifiManager.autoConnect(ssidAP.c_str())) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    // Reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  // If you get here you have connected to the WiFi :D
  Serial.println("\nConnected to WiFi ...yeey :)\n");

  // Initialize EEPROM and pointer
  Serial.println("Initializing EEPROM...");
  EEPROM.begin(SECTOR_SIZE);

  PtrFlashMemory =
      (flashMemory_t *)EEPROM.getDataPtr(); // Assigning pointer address to
                                            // flash memory block dumped in RAM
  Serial.println("Memory assignement successful!");

  // Calculate checksum and validate memory area
  // TODO: use 32bit crc
  crc32b = calculateCRC32(PtrFlashMemory->Bytes, sizeof(Fields_t));
  crc8b = EEPROM.read(SECTOR_SIZE - 1); // Read crc at the end of sector
  Serial.println("CRC32 calculated=" + String(crc32b & 0xff) + " readed=" +
                 String(crc8b));

  if (((uint8_t)crc32b & 0xff) == crc8b)
    Serial.println("CRC32 pass!");
  else {
    Serial.println("CRC32 fail! Cleaning memory...");
    cleanMemory();
  }
}

void loop() {
  Serial.println("#########################\n");

  agrumino.turnBoardOn();
  agrumino.turnLedOn();

  // Variable currentIndex is the last memorized struct of sensor data
  currentIndex = PtrFlashMemory->Fields.index;
  if (currentIndex > N_SAMPLES - 1) { // Circular buffer behaviour
    currentIndex = 0;
    PtrFlashMemory->Fields.index = currentIndex;
  }

  Serial.println("*****   CURRENT INDEX IS:  " + String(currentIndex));

  // Copy sensors data to struct
  PtrFlashMemory->Fields.data.vector[currentIndex].temp = agrumino.readTempC();
  PtrFlashMemory->Fields.data.vector[currentIndex].soil = agrumino.readSoil();
  PtrFlashMemory->Fields.data.vector[currentIndex].lux = agrumino.readLux();
  PtrFlashMemory->Fields.data.vector[currentIndex].batt =
      agrumino.readBatteryVoltage();
  PtrFlashMemory->Fields.data.vector[currentIndex].battLevel =
      agrumino.readBatteryLevel();
  PtrFlashMemory->Fields.data.vector[currentIndex].usb =
      agrumino.isAttachedToUSB();
  PtrFlashMemory->Fields.data.vector[currentIndex].charge =
      agrumino.isBatteryCharging();

  PtrFlashMemory->Fields.index++; // Increment index

  Serial.println("temperature:       " +
                 String(PtrFlashMemory->Fields.data.vector[currentIndex].temp) +
                 "°C");
  Serial.println("soilMoisture:      " +
                 String(PtrFlashMemory->Fields.data.vector[currentIndex].soil) +
                 "%");
  Serial.println("illuminance :      " +
                 String(PtrFlashMemory->Fields.data.vector[currentIndex].lux) +
                 " lux");
  Serial.println("batteryVoltage :   " +
                 String(PtrFlashMemory->Fields.data.vector[currentIndex].batt) +
                 " V");
  Serial.println(
      "batteryLevel :     " +
      String(PtrFlashMemory->Fields.data.vector[currentIndex].battLevel) + "%");
  Serial.println("isAttachedToUSB:   " +
                 String(PtrFlashMemory->Fields.data.vector[currentIndex].usb));
  Serial.println(
      "isBatteryCharging: " +
      String(PtrFlashMemory->Fields.data.vector[currentIndex].charge));
  Serial.println();

  // Calculate checksum
  crc32b = calculateCRC32(PtrFlashMemory->Bytes, sizeof(Fields_t));
  crc8b = (uint8_t)crc32b & 0xff;
  Serial.println("New CRC32=" + String(crc8b));
  EEPROM.write(SECTOR_SIZE - 1, crc8b); // Put crc at the end of sector

  // With EEPROM.commit() we write all our data from RAM
  // to flash in one block. Actually the entire sector is written (#SECTOR_SIZE
  // bytes).
  // Byte-level access to ESP's flash is not possible with this flash chip.
  if (EEPROM.commit()) {
    Serial.println("EEPROM successfully committed");
  } else {
    Serial.println("ERROR! EEPROM commit failed");
  }

  // We have the queue full, we need to consume data and send to cloud.
  // Variable currentIndex will be resetted @next loop.
  if (currentIndex == N_SAMPLES - 1) {
    // Change this if you whant to change your thing name
    // We use the chip Id to avoid name clashing
    String thingName = "Agrumino-" + getChipId();

    // You can fill fieldData with up to 8 values to write to successive fields
    // in your channel.
    String fieldData[NUM_FIELDS + 1];

    Serial.println("Now I'm sending " + String(N_SAMPLES) +
                   " json(s) to Dweet");
    for (uint8_t i = 0; i < N_SAMPLES; i++) {

      // You can write to multiple fields by storing data in the fieldData[]
      // array, and changing numFields.
      // Write the moisture data to field 1.
      fieldData[UMIDITA] = String(PtrFlashMemory->Fields.data.vector[i].soil);
      fieldData[TEMPERATURA] =
          String(PtrFlashMemory->Fields.data.vector[i].temp);
      fieldData[LUMINOSITA] =
          String(PtrFlashMemory->Fields.data.vector[currentIndex].lux);
      fieldData[TENSIONE] =
          String(PtrFlashMemory->Fields.data.vector[currentIndex].batt);
      fieldData[LIVELLOBATT] =
          String(PtrFlashMemory->Fields.data.vector[currentIndex].battLevel);

      HTTPPost(NUM_FIELDS, fieldData);
      delay(15000); // delay min between two post for free user on ThingSpeak
    }
  }
  // Blink when the business is done for giving an Ack to the user
  blinkLed(200, 1);

  // Board off before delay/sleep to save battery :)
  agrumino.turnBoardOff();

  // delaySec(SLEEP_TIME_SEC); // The ESP8266 stays powered, executes the loop
  // repeatedly
  deepSleepSec(SLEEP_TIME_SEC); // ESP8266 enter in deepSleep and after the
                                // selected time starts back from setup() and
                                // then loop()
}

//////////////////
// HTTP methods //
//////////////////

int HTTPPost(int numFields, String fieldData[]) {
  if (client.connect(THING_SPEAK_ADDRESS, 80)) {
    // Build the Posting data string.
    // If you have multiple fields, make sure the sting does not exceed 1440
    // characters.
    String postData = "api_key=" + writeAPIKey;
    for (int fieldNumber = 1; fieldNumber < numFields + 1; fieldNumber++) {
      String fieldName = "field" + String(fieldNumber);
      postData += "&" + fieldName + "=" + fieldData[fieldNumber];
    }
    // POST data via HTTP
    Serial.println("Connecting to ThingSpeak for update...");
    Serial.println();
    client.println("POST /update HTTP/1.1");
    client.println("Host: api.thingspeak.com");
    client.println("Connection: close");
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.println("Content-Length: " + String(postData.length()));
    client.println();
    client.println(postData);
    Serial.println(postData);
    String answer = getResponse();
    Serial.println(answer);
  } else {
    Serial.println("Connection Failed");
  }
}

// Wait for a response from the server to be available,
// and then collect the response and build it into a string.
String getResponse() {
  String response;
  long startTime = millis();
  delay(200);
  while (client.available() < 1 && ((millis() - startTime) < TIMEOUT)) {
    delay(5);
  }
  if (client.available() > 0) { // Get response from server
    char charIn;
    do {
      charIn = client.read(); // Read a char from the buffer.
      response += charIn;     // Append the char to the string response.
    } while (client.available() > 0);
  }
  client.stop();
  return response;
}

/////////////////////
// Utility methods //
/////////////////////

void blinkLed(int duration, int blinks) {
  for (int i = 0; i < blinks; i++) {
    agrumino.turnLedOn();
    delay(duration);
    agrumino.turnLedOff();
    if (i < blinks) {
      delay(duration); // Avoid delay in the latest loop ;)
    }
  }
}

void delaySec(int sec) { delay(sec * 1000); }

void deepSleepSec(int sec) {
  Serial.print("\nGoing to deepSleep for ");
  Serial.print(sec);
  Serial.println(" seconds... (ー。ー) zzz\n");
  ESP.deepSleep(sec * 1000000); // microseconds
}

const String getChipId() {
  // Returns the ESP Chip ID, Typical 7 digits
  return String(ESP.getChipId());
}

// If the Agrumino S1 button is pressed for 5 seconds then reset the wifi saved
// settings.
boolean checkIfResetWiFiSettings() {

  int remainingsLoops = (5 * 1000) / 100;

  Serial.print("Check if reset WiFi settings: ");

  while (agrumino.isButtonPressed() && remainingsLoops > 0) {
    delay(100);
    remainingsLoops--;
    Serial.print(".");
  }

  if (remainingsLoops == 0) {
    // Reset Wifi Settings
    Serial.println(" YES!");
    return true;
  } else {
    Serial.println(" NO");
    return false;
  }
}

// Function to calculate CRC32
// TODO: verify support for hardware CRC32 in bsp, eventually
// copy implementation from official Espressif's SDK if present.
uint32_t calculateCRC32(const uint8_t *data, size_t length) {
  uint32_t crc = 0xffffffff;
  int i;
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

// This function initilizes content of
// the data struct in RAM
void cleanMemory() {
  Serial.println("RAM memory data struct initialization");
  PtrFlashMemory->Fields.index = 0;
  for (uint8_t i = 0; i < N_SAMPLES; i++) {
    PtrFlashMemory->Fields.data.vector[i].temp = 0.0f;
    PtrFlashMemory->Fields.data.vector[i].soil = 0u;
    PtrFlashMemory->Fields.data.vector[i].lux = 0.0f;
    PtrFlashMemory->Fields.data.vector[i].batt = 0.0f;
    PtrFlashMemory->Fields.data.vector[i].battLevel = 0u;
    PtrFlashMemory->Fields.data.vector[i].usb = false;
    PtrFlashMemory->Fields.data.vector[i].charge = false;
  }
}
