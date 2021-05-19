/*
  AgruminoDweetHttpOTA.ino - Sample project for Agrumino board using the
  Agrumino library.
  !!!WARNING!!! You need to program the board with option Tools->Erase
  Flash->All Flash Contents

  Created by giuseppe.broccia@lifely.cc on October 2017.
  Modified May 2021 by:
  Giuseppe Broccia giuseppe.broccia@lifely.cc
  Gabriele Foddis gabriele.foddis@lifely.cc


  This sketch reads every 1h all values from the Arduino board and update them
  to the Dweet.io service every 4h. It integrates FLASH management to collect 
  all data before transmit them.  
  Moreover integrates the update of the firmware via OTA using a remote HTTP server. 
  Each time the executable (.bin) loaded on the server is updated, 
  the board downloads it automatically.

  @see Agrumino.h for the documentation of the lib
*/
#include "Agrumino.h"         // Our super cool lib ;)
#include <ArduinoJson.h>      // https://github.com/bblanchon/ArduinoJson
#include <DNSServer.h>        // Installed from ESP8266 board
#include <ESP8266WebServer.h> // Installed from ESP8266 board
#include <ESP8266WiFi.h>      // https://github.com/esp8266/Arduino
#include <WiFiManager.h>      // https://github.com/tzapu/WiFiManager

#include <ESP8266WiFiMulti.h>
#include <ESP8266httpUpdate.h>

#define VERSION_SERVER_URL                                                     \
  "http://SERVERNAMEHERE/agruminoupdates/vers.txt" // this remote file stores
                                                   // the current version of the
                                                   // binary in the server
#define BIN_SERVER_URL                                                         \
  "http://SERVERNAMEHERE/agruminoupdates/" // this is the url of the remote
                                           // folder containing the binary and
                                           // the version files
ESP8266WiFiMulti WiFiMulti;
WiFiClient client;           // Used to send/receive data to a WiFi AP
String currentVersion = "1"; // this is the version of the sketch. If you update
                             // this sketch, please update this value and the
                             // file vers.txt defined in VERSION_SERVER_URL
bool errorReadingStringServer =
    false; // this flag becomes true if there are some problem reading the
           // vers.txt file defined in VERSION_SERVER_URL

// Time to sleep in second between the readings/data sending
#define SLEEP_TIME_SEC 3600 // [Seconds]

// The size of the flash sector we want to use to store samples (do not modify).
#define SECTOR_SIZE 4096u

// Cloud Web Server data, in our sample we use Dweet.io.
const char *WEB_SERVER_HOST = "dweet.io";
const String WEB_SERVER_API_SEND_DATA =
    "/dweet/quietly/for/"; // The Dweet name is created in the loop() method.

// Our super cool lib
Agrumino agrumino;

// Flash memory stuff
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

  // WiFiManager Logic taken from
  // https://github.com/kentaylor/WiFiManager/blob/master/examples/ConfigOnSwitch/ConfigOnSwitch.ino

  // With batteryCheck true will return true only if the Agrumino is attached to
  // USB with a valid power
  boolean resetWifi = checkIfResetWiFiSettings(true);
  boolean hasWifiCredentials = WiFi.SSID().length() > 0;

  if (resetWifi || !hasWifiCredentials) {
    // Show Configuration portal

    // Blink and keep ON led as a feedback :)
    blinkLed(100, 5);
    agrumino.turnLedOn();

    WiFiManager wifiManager;

    // Customize the web configuration web page
    wifiManager.setCustomHeadElement("<h1>Agrumino</h1>");

    // Sets timeout in seconds until configuration portal gets turned off.
    // If not specified device will remain in configuration mode until
    // switched off via webserver or device is restarted.
    // wifiManager.setConfigPortalTimeout(600);

    // Starts an access point and goes into a blocking loop awaiting
    // configuration
    String ssidAP = "Agrumino-AP-" + getChipId();
    boolean gotConnection = wifiManager.startConfigPortal(ssidAP.c_str());

    Serial.print("\nGot connection from Config Portal: ");
    Serial.println(gotConnection);

    agrumino.turnLedOff();

    // ESP.reset() doesn't work on first reboot
    agrumino.deepSleepSec(1);
    return;

  } else {
    // Try to connect to saved network
    // Force to station mode because if device was switched off while in access
    // point mode it will start up next time in access point mode.
    agrumino.turnLedOn();
    WiFi.mode(WIFI_STA);
    WiFi.waitForConnectResult();
  }

  boolean connected = (WiFi.status() == WL_CONNECTED);

  if (connected) {
    // If you get here you have connected to the WiFi :D
    Serial.print("\nConnected to WiFi as ");
    Serial.print(WiFi.localIP());
    Serial.println(" ...yeey :)\n");
  } else {
    // No WiFi connection. Skip a cycle and retry later
    Serial.print("\nNot connected!\n");
    // ESP.reset() doesn't work on first reboot
    agrumino.deepSleepSec(SLEEP_TIME_SEC);
  }

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
  Serial.println("CRC32 calculated=" + String(crc32b & 0xff) +
                 " readed=" + String(crc8b));

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

  // Here we check the version of the remote binary file. If it has changed, the
  // sketch will be updated automatically.
  if ((WiFiMulti.run() == WL_CONNECTED)) {
    bool equalVersions = false;
    String remoteVersion = remoteBinVersion();

    if (errorReadingStringServer) {
      Serial.println("Oops error in reading version from server....");
    } else {
      if (remoteVersion == currentVersion)
        equalVersions = true;
      else
        equalVersions = false;

      Serial.println("Remote version: " + remoteVersion);
      Serial.println("Current version: " + currentVersion);

      if (equalVersions) {
        if (!errorReadingStringServer) {
          Serial.println("No firmware update is necessary.");
        }
      } else {
        Serial.println(
            "New firmware is available, downloading and updating system...");
        // The line below is optional. It can be used to blink the LED on the
        // board during flashing The LED will be on during download of one
        // buffer of data from the network. The LED will be off during writing
        // that buffer to flash On a good connection the LED should flash
        // regularly. On a bad connection the LED will be on much longer than it
        // will be off. Other pins than LED_BUILTIN may be used. The second
        // value is used to put the LED on. If the LED is on with HIGH, that
        // value should be passed ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);

        // Add optional callback notifiers
        ESPhttpUpdate.onStart(update_started);
        ESPhttpUpdate.onEnd(update_finished);
        ESPhttpUpdate.onProgress(update_progress);
        ESPhttpUpdate.onError(update_error);

        // we reboot the board only after the deepsleep
        ESPhttpUpdate.rebootOnUpdate(false);
        t_httpUpdate_return ret = ESPhttpUpdate.update(
            client, String(BIN_SERVER_URL) + "AgruminoDweetHttpOTA.ino.bin");
        // Or:
        // t_httpUpdate_return ret = ESPhttpUpdate.update(client,
        // "servername.com", 80,
        // "agruminoupdates/AgruminoDweetHttpOTA.ino.bin");

        switch (ret) {
        case HTTP_UPDATE_FAILED:
          Serial.printf("HTTP_UPDATE_FAILED with Error (%d): %s\n",
                        ESPhttpUpdate.getLastError(),
                        ESPhttpUpdate.getLastErrorString().c_str());
          break;

        case HTTP_UPDATE_NO_UPDATES: // Should never go here with Simple updater
                                     // see ota_updates doc in the core
          Serial.println("HTTP_UPDATE_NO_UPDATES");
          break;

        case HTTP_UPDATE_OK:
          Serial.println("HTTP_UPDATE_OK");
          break;
        }
      }
    }

    delay(2000);
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
  // bytes). Byte-level access to ESP's flash is not possible with this flash
  // chip.
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
    String dweetThingName = "Agrumino-" + getChipId();

    Serial.println("Now I'm sending " + String(N_SAMPLES) +
                   " json(s) to Dweet");
    for (uint8_t i = 0; i < N_SAMPLES; i++) {
      // Send data to our web service
      sendData(dweetThingName, PtrFlashMemory->Fields.data.vector[i].temp,
               PtrFlashMemory->Fields.data.vector[i].soil,
               PtrFlashMemory->Fields.data.vector[i].lux,
               PtrFlashMemory->Fields.data.vector[i].batt,
               PtrFlashMemory->Fields.data.vector[i].battLevel,
               PtrFlashMemory->Fields.data.vector[i].usb,
               PtrFlashMemory->Fields.data.vector[i].charge);
      delay(5000);
    }
  }

  // Blink when the business is done for giving an Ack to the user
  blinkLed(200, 2);

  // Board off before delay/sleep to save battery :)
  agrumino.turnBoardOff();

  // delaySec(SLEEP_TIME_SEC); // The ESP8266 stays powered, executes the loop
  // repeatedly
  agrumino.deepSleepSec(
      SLEEP_TIME_SEC); // ESP8266 enter in deepSleep and after the selected time
                       // starts back from setup() and then loop()
}

//////////////////
// HTTP methods //
//////////////////

void sendData(String dweetName, float temp, int soil, float lux, float batt,
              unsigned int battLevel, boolean usb, boolean charge) {

  String bodyJsonString =
      getSendDataBodyJsonString(temp, soil, lux, batt, battLevel, usb, charge);

  // Use WiFiClient class to create TCP connections, we try until the connection
  // is estabilished
  while (!client.connect(WEB_SERVER_HOST, 80)) {
    Serial.println("connection failed\n");
    delay(1000);
  }
  Serial.println("connected to " + String(WEB_SERVER_HOST) + " ...yeey :)\n");

  Serial.println("###################################");
  Serial.println("### Your Dweet.io thing name is ###");
  Serial.println("###   --> " + dweetName + " <--  ###");
  Serial.println("###################################\n");

  // Print the HTTP POST API data for debug
  Serial.println("Requesting POST: " + String(WEB_SERVER_HOST) +
                 WEB_SERVER_API_SEND_DATA + dweetName);
  Serial.println("Requesting POST: " + bodyJsonString);

  // This will send the request to the server
  client.println("POST " + WEB_SERVER_API_SEND_DATA + dweetName + " HTTP/1.1");
  client.println("Host: " + String(WEB_SERVER_HOST) + ":80");
  client.println("Content-Type: application/json");
  client.println("Content-Length: " + String(bodyJsonString.length()));
  client.println();
  client.println(bodyJsonString);

  delay(10);

  int timeout = 300; // 100 ms per loop so 30 sec.
  while (!client.available()) {
    // Waiting for server response
    delay(100);
    Serial.print(".");
    timeout--;
    if (timeout <= 0) {
      Serial.print("Err. client.available() timeout reached!");
      return;
    }
  }

  String response = "";

  while (client.available() > 0) {
    char c = client.read();
    response = response + c;
  }

  // Remove bad chars from response
  response.trim();
  response.replace("/n", "");
  response.replace("/r", "");

  Serial.println("\nAPI Update successful! Response: \n");
  Serial.println(response);
}

// Returns the Json body that will be sent to the send data HTTP POST API
String getSendDataBodyJsonString(float temp, int soil, float lux, float batt,
                                 unsigned int battLevel, boolean usb,
                                 boolean charge) {
  DynamicJsonDocument doc(200); // Allocate json buffer
  String input = "{}";
  deserializeJson(doc, input); // resets the document
  JsonObject jsonPost = doc.as<JsonObject>();

  jsonPost["temp"] = String(temp);
  jsonPost["soil"] = String(soil);
  jsonPost["lux"] = String(lux);
  jsonPost["battVolt"] = String(batt);
  jsonPost["battLevel"] = String(battLevel);
  jsonPost["battCharging"] = String(charge);
  jsonPost["usbConnected"] = String(usb);

  String jsonPostString;
  serializeJson(doc, jsonPostString); // create a minified JSON document

  return jsonPostString;
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

const String getChipId() {
  // Returns the ESP Chip ID, Typical 7 digits
  return String(ESP.getChipId());
}

// If the Agrumino S1 button is pressed for 5 secs then reset the wifi saved
// settings. If "checkBattery" is true the method return true only if the USB is
// connected.
boolean checkIfResetWiFiSettings(boolean checkBattery) {
  int delayMs = 100;
  int remainingsLoops = (5 * 1000) / delayMs;
  Serial.print("\nCheck if reset WiFi settings: ");
  while (remainingsLoops > 0 && agrumino.isButtonPressed() &&
         (agrumino.isAttachedToUSB() ||
          !checkBattery) // The Agrumino must be attached to USB
  ) {
    // Blink the led every sec as confirmation
    if (remainingsLoops % 10 == 0) {
      agrumino.turnLedOn();
    }
    delay(delayMs);
    agrumino.turnLedOff();
    remainingsLoops--;
    Serial.print(".");
  }
  agrumino.turnLedOff();
  boolean success = (remainingsLoops == 0);
  Serial.println(success ? " YES!" : " NO");
  Serial.println();
  return success;
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

// This function reads the remote file vers.txt defined in VERSION_SERVER_URL
// and containing the current version of the binary file
String remoteBinVersion() {
  HTTPClient http;
  String payload = "";

  if (http.begin(client, VERSION_SERVER_URL)) {
    int httpCode = http.GET(); // Make the request

    if (httpCode > 0) { // Check for the returning code
      switch (httpCode) {
      case 200:
        Serial.println("Page found: code 200");
        payload = http.getString();
        payload.replace("\n", "");
        Serial.println("Successful HTTP request for bin version");
        errorReadingStringServer = false;
        break;
      case 404:
        Serial.println("Page not found: code 404");
        payload = "0";
        errorReadingStringServer = true;
        break;
      default:
        Serial.println("Other error: code " + String(httpCode));
        payload = "0";
        errorReadingStringServer = true;
        break;
      }
    } else {
      errorReadingStringServer =
          true; // flag for error in http request for binary version
      Serial.println("Error on HTTP request for bin version");
    }
  }

  http.end(); // Free the resources
  return payload;
}

void update_started() {
  Serial.println("CALLBACK:  HTTP update process started");
}

void update_finished() {
  Serial.println("CALLBACK:  HTTP update process finished");
}

void update_progress(int cur, int total) {
  Serial.printf("CALLBACK:  HTTP update process at %d of %d bytes...\n", cur,
                total);
}

void update_error(int err) {
  Serial.printf("CALLBACK:  HTTP update fatal error code %d\n", err);
}
