/*
 * LocalVault ESP - Arduino IDE version
 * 
 * Hardware supported:
 *   - ESP32-C3 (XIAO ESP32C3, ESP32-C3-DevKitM-1, etc.)
 *   - ESP8266 (Wemos D1 Mini, NodeMCU, etc.)
 *   - Raspberry Pi Pico W
 * 
 * Installation:
 *   1. Install board support (see README.md)
 *   2. Install libraries: ArduinoJson (v7+)
 *   3. Select your board in Tools > Board
 *   4. Set flash size / partition scheme if needed
 *   5. Upload
 * 
 * First boot: creates AP "LocalVault-XXXXXX" - connect and go to http://192.168.4.1
 * Configure your home WiFi, optionally set static IP.
 * Then access vault at the device's IP address.
 */

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

// Include all implementation files directly for Arduino IDE
#include "config.h"
#include "config.cpp"
#include "wifi_manager.h"
#include "wifi_manager.cpp"
#include "vault_store.h"
#include "vault_store.cpp"
#include "http_server.h"
#include "http_server.cpp"
#include "localvault_html.h"

// For Pico W: need to include WiFi, WebServer, LittleFS from core
#if defined(PICO_W) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
  #include <WiFi.h>
  #include <WebServer.h>
  #include <LittleFS.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  #include <LittleFS.h>
  #define WebServer ESP8266WebServer
#elif defined(ESP32)
  #include <WiFi.h>
  #include <WebServer.h>
  #include <LittleFS.h>
#endif

// LED pin override for Pico W
#if defined(PICO_W) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
  #ifndef LED_PIN
    #define LED_PIN 25
  #endif
#endif

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n\n=== LocalVault ESP (Arduino IDE) ===");
  Serial.printf("Device: %s\n", DEVICE_NAME);
  Serial.printf("Version: %s\n", FIRMWARE_VERSION);
  Serial.printf("Chip: %s\n", ESP.getChipModel());
  Serial.printf("Flash: %d MB\n", ESP.getFlashChipSize() / (1024*1024));
  Serial.printf("Free Heap: %d\n", ESP.getFreeHeap());
  
  // Mount LittleFS
  if (!LittleFS.begin()) {
    Serial.println("[FS] LittleFS mount failed, formatting...");
    LittleFS.format();
    if (!LittleFS.begin()) {
      Serial.println("[FS] Format failed, halting");
      while (true) delay(1000);
    }
  }
  Serial.println("[FS] LittleFS mounted");
  
  // Initialize subsystems
  vaultStoreInit();
  httpServerInit();
  wifiInit();
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  
  Serial.println("[Main] Setup complete");
}

void loop() {
  httpServerLoop();
  wifiLoop();
  
  static unsigned long lastBlink = 0;
  static bool ledState = false;
  unsigned long now = millis();
  
  if (wifiGetMode() == WIFI_MODE_AP) {
    if (now - lastBlink > 1000) {
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState ? LOW : HIGH);
      lastBlink = now;
    }
  } else if (wifiGetMode() == WIFI_MODE_AP_STA) {
    if (now - lastBlink > 200) {
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState ? LOW : HIGH);
      lastBlink = now;
    }
  } else {
    digitalWrite(LED_PIN, LOW);
  }
  
  delay(1);
}