/*
 * LocalVault ESP - Firmware for ESP32-C3 / ESP8266 / Raspberry Pi Pico W
 * Serves LocalVault.html with WebCrypto polyfill for non-secure contexts
 * Provides /api/store mirroring server.js, WiFi config portal, static IP
 */
#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#include "config.h"
#include "wifi_manager.h"
#include "vault_store.h"
#include "auth_store.h"
#include "http_server.h"

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n\n=== LocalVault ESP ===");
  Serial.printf("Device: %s\n", DEVICE_NAME);
  Serial.printf("Version: %s\n", FIRMWARE_VERSION);
  
#if defined(ESP32)
  Serial.printf("Chip: %s\n", ESP.getChipModel());
  Serial.printf("Flash: %d MB\n", ESP.getFlashChipSize() / (1024*1024));
  Serial.printf("Free Heap: %d\n", ESP.getFreeHeap());
#elif defined(ESP8266)
  Serial.printf("Chip: ESP8266\n");
  Serial.printf("Flash: %d MB\n", ESP.getFlashChipSize() / (1024*1024));
  Serial.printf("Free Heap: %d\n", ESP.getFreeHeap());
#elif defined(PICO_W)
  Serial.printf("Chip: RP2040\n");
  Serial.printf("Flash: %d MB\n", 2);  // Pico W has 2MB flash
  Serial.printf("Free Heap: %d\n", rp2040.getFreeHeap());
#endif
  
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
  
  // List files for debugging
#if defined(ESP32)
  File root = LittleFS.open("/");
  File file = root.openNextFile();
  while (file) {
    Serial.printf("[FS] %s (%d bytes)\n", file.name(), file.size());
    file = root.openNextFile();
  }
#elif defined(ESP8266)
  Dir dir = LittleFS.openDir("/");
  while (dir.next()) {
    Serial.printf("[FS] %s (%d bytes)\n", dir.fileName().c_str(), dir.fileSize());
  }
#elif defined(PICO_W)
  File root = LittleFS.open("/", "r");
  if (root) {
    File file = root.openNextFile();
    while (file) {
      Serial.printf("[FS] %s (%d bytes)\n", file.name(), file.size());
      file = root.openNextFile();
    }
  }
#endif
  
  // Initialize subsystems
  vaultStoreInit();
  authInit();
  httpServerInit();
  wifiInit();  // This also loads config and applies defaults
  
  // LED setup
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);  // Active low - off
  
  Serial.println("[Main] Setup complete");
}

void loop() {
  httpServerLoop();
  wifiLoop();
  
  // Heartbeat LED (slow blink when AP mode, fast when connecting, solid when connected)
  static unsigned long lastBlink = 0;
  static bool ledState = false;
  unsigned long now = millis();
  
  if (wifiGetMode() == LV_WIFI_MODE_AP) {
    if (now - lastBlink > 1000) {
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState ? LOW : HIGH);
      lastBlink = now;
    }
  } else if (wifiGetMode() == LV_WIFI_MODE_AP_STA) {
    if (now - lastBlink > 200) {
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState ? LOW : HIGH);
      lastBlink = now;
    }
  } else {
    // STA connected - solid on
    digitalWrite(LED_PIN, LOW);
  }
  
  // Yield for WiFi stack
  delay(1);
}