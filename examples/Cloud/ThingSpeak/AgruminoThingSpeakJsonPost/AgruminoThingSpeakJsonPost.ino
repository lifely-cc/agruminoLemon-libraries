/*
  AgruminoThingSpeakJsonPost.ino - Sample project for Agrumino board using the
  Agrumino library.
  !!!WARNING!!! You need to program the board with option Tools->Erase
  Flash->All Flash Contents

  Created by Paolo Paolucci on May 2018.
  Modified May 2021 by:
  Giuseppe Broccia giuseppe.broccia@lifely.cc
  Gabriele Foddis gabriele.foddis@lifely.cc

  This sketch Sketch save sensor data of Agrumino board on FLASH
  and after 4 cycles transmits data to Thing Speak
  with one JSON POST.

  @see Agrumino.h for the documentation of the lib
*/

#include "Agrumino.h"         // Our super cool lib ;)
#include <ArduinoJson.h>      // https://github.com/bblanchon/ArduinoJson
#include <DNSServer.h>        // Installed from ESP8266 board
#include <ESP8266WebServer.h> // Installed from ESP8266 board
#include <ESP8266WiFi.h>      // https://github.com/esp8266/Arduino
#include <NTPClient.h>        // https://github.com/taranais/NTPClient
#include <WiFiManager.h>      // https://github.com/tzapu/WiFiManager
#include <WiFiUdp.h>

// Time to sleep in second between the readings/data sending
#define SLEEP_TIME_SEC 10 // seconds --> 1 hour

// The size of the flash sector we want to use to store samples (do not modify).
#define SECTOR_SIZE 4096u

// ThingSpeak information.
#define THING_SPEAK_ADDRESS "api.thingspeak.com"
String ThingSpeakName = "XXXXXXX"; // Change this with your Channel ID
String writeAPIKey = "YYYYYYYYYYYYYYYY"; // Change this to your channel Write API key.
#define TIMEOUT 5000    // Timeout for server response.

// Our super cool lib
Agrumino agrumino;

// Used for sending Json POST requests
DynamicJsonDocument doc(200 * N_SAMPLES);

// Used to create TCP connections and make Http calls
WiFiClient client;

flashMemory_t *PtrFlashMemory = NULL;

uint32_t crc32b = 0;
uint16_t currentIndex = 0;
uint8_t crc8b = 0;

// To check time in seconds from a NTPClient
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

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
  // here
  // and goes into a blocking loop awaiting configuration
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

  // Starts the underlying UDP client with the default local port
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(7200); // GMT +2 = 7200 Italy

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

  // By default an update from the NTP Server is only made every 60 seconds.
  // If not updated it will force the update from the NTP Server.
  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }

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
    String bodyJsonString = "";
    Serial.println("Now I'm sending json to ThingSpeak");
    unsigned long secondsNTP = timeClient.getEpochTime();
    // put data in the same order you configured your ThingSpeak dashboard
    // (temp-soil-lux-batt-battLevel-usb-charge)
    for (uint8_t i = 0; i < N_SAMPLES; i++) {
      unsigned long seconds =
          secondsNTP - (SLEEP_TIME_SEC * ((N_SAMPLES - 1) - i));
      String tmp = getSendDataBodyJsonString(
          PtrFlashMemory->Fields.data.vector[i].temp,
          PtrFlashMemory->Fields.data.vector[i].soil,
          PtrFlashMemory->Fields.data.vector[i].lux,
          PtrFlashMemory->Fields.data.vector[i].batt,
          PtrFlashMemory->Fields.data.vector[i].battLevel,
          PtrFlashMemory->Fields.data.vector[i].usb,
          PtrFlashMemory->Fields.data.vector[i].charge, seconds);
      bodyJsonString = bodyJsonString + tmp + String(",");
    }
    JSONPost(bodyJsonString);
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

//////////////////
// HTTP methods //
//////////////////

int JSONPost(String bodyJsonString) {

  bodyJsonString.remove(bodyJsonString.length() - 1);
  // Serial.println("Json after remove is:" +  bodyJsonString); //DEBUG

  // JSON format for Thing Speak (see
  // https://www.mathworks.com/help/thingspeak/bulkwritejsondata.html)
  bodyJsonString = String("{\"write_api_key\": \"") + String(writeAPIKey) +
                   String("\",") + String("\"updates\": [") + bodyJsonString +
                   ("]}");

  // Use WiFiClient class to create TCP connections, we try until the connection
  // is estabilished
  while (!client.connect(THING_SPEAK_ADDRESS, 80)) {
    Serial.println("connection failed\n");
    delay(1000);
  }
  Serial.println("connected to " + String(THING_SPEAK_ADDRESS) +
                 " ...yeey :)\n");

  Serial.println("###################################");
  Serial.println("### Your ThingSpeak Channel ID is ###");
  Serial.println("###   --> " + ThingSpeakName + " <--  ###");
  Serial.println("###################################\n");

  // Print the JSON POST API data for debug
  Serial.println("Requesting POST: " + String(THING_SPEAK_ADDRESS) +
                 String(" Channel ID: ") + ThingSpeakName);
  Serial.println("Requesting POST: " + bodyJsonString);

  // client.println( "POST /update HTTP/1.1" );
  client.println("POST /channels/" + String(ThingSpeakName) +
                 "/bulk_update.json HTTP/1.1");
  client.println("Host: api.thingspeak.com");
  client.println("Connection: close");
  client.println("Content-Type: application/json");
  client.println("Content-Length: " + String(bodyJsonString.length()));
  client.println();
  client.println(bodyJsonString);

  String answer = getResponse();
  Serial.println(answer);
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

// Returns the Json body that will be sent to the send data HTTP POST API
String getSendDataBodyJsonString(float temp, int soil, float lux, float batt,
                                 unsigned int battLevel, boolean usb,
                                 boolean charge, unsigned long sec) {
  String datetime = getFormattedDateThingSpeak(sec);

  String input = "{}";
  deserializeJson(doc, input); // resets the document
  JsonObject jsonPost = doc.as<JsonObject>();

  jsonPost["created_at"] = datetime;
  jsonPost["field1"] = String(temp);
  jsonPost["field2"] = String(soil);
  jsonPost["field3"] = String(lux);
  jsonPost["field4"] = String(batt);
  jsonPost["field5"] = String(battLevel);
  jsonPost["field6"] = String(charge);
  jsonPost["field7"] = String(usb);

  String jsonPostString;
  serializeJson(doc, jsonPostString); // create a minified JSON document

  return jsonPostString;
}

// function to evaluate the Thing Speak formatted string for data and time
String getFormattedDateThingSpeak(unsigned long secs) {
  // unsigned long rawTime = (secs ? secs : timeClient->getEpochTime()) /
  // 86400L;  // in days
  unsigned long rawTime = secs / 86400L; // in days
  unsigned long days = 0, year = 1970;
  uint8_t month;
  static const uint8_t monthDays[] = {31, 28, 31, 30, 31, 30,
                                      31, 31, 30, 31, 30, 31};

  while ((days += (LEAP_YEAR(year) ? 366 : 365)) <= rawTime)
    year++;
  rawTime -= days - (LEAP_YEAR(year) ? 366 : 365); // now it is days in this year, starting at 0
  days = 0;
  for (month = 0; month < 12; month++) {
    uint8_t monthLength;
    if (month == 1) { // february
      monthLength = LEAP_YEAR(year) ? 29 : 28;
    } else {
      monthLength = monthDays[month];
    }
    if (rawTime < monthLength)
      break;
    rawTime -= monthLength;
  }
  String monthStr =
      ++month < 10 ? "0" + String(month) : String(month); // jan is month 1
  String dayStr =
      ++rawTime < 10 ? "0" + String(rawTime) : String(rawTime); // day of month
  return String(year) + "-" + monthStr + "-" + dayStr + " " +
         timeClient.getFormattedTime(secs ? secs : 0) + " +0200";
}
