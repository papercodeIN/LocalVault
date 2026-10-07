#pragma once
/*
 * esp-vault configuration constants
 */
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <String.h>

// Filesystem paths
#define CONFIG_FILE "/config.json"
#define VAULT_FILE  "/vault.store.json"

// HTTP server
#define HTTP_PORT 80
#define MAX_API_BODY 25 * 1024 * 1024  // 25 MB, matches server.js

// WiFi
#define WIFI_CONNECT_TIMEOUT_MS 15000
#define WIFI_RECONNECT_INTERVAL_MS 30000
#define AP_SSID_PREFIX "LocalVault-"
#define AP_PASSWORD ""  // open AP for initial setup

// Default static IP (user can change via web UI)
#define DEFAULT_STATIC_IP "192.168.4.1"
#define DEFAULT_GATEWAY   "192.168.4.1"
#define DEFAULT_NETMASK   "255.255.255.0"

// Device info
#define DEVICE_NAME "LocalVault"
#define FIRMWARE_VERSION "1.0.0"

// LED indicators (active low on most boards)
#ifndef LED_PIN
  #ifdef ESP32C3
    #define LED_PIN 8
  #elif defined(ESP8266)
    #define LED_PIN 2
  #elif defined(PICO_W)
    #define LED_PIN 25  // Pico W onboard LED
  #else
    #define LED_PIN 2
  #endif
#endif

// Config keys (stored in LittleFS /config.json)
#define CFG_KEY_WIFI_SSID       "wifi_ssid"
#define CFG_KEY_WIFI_PASS       "wifi_pass"
#define CFG_KEY_STATIC_IP       "static_ip"
#define CFG_KEY_STATIC_GATEWAY  "static_gateway"
#define CFG_KEY_STATIC_NETMASK  "static_netmask"
#define CFG_KEY_USE_STATIC      "use_static"
#define CFG_KEY_DEVICE_NAME     "device_name"
#define CFG_KEY_AP_SSID         "ap_ssid"
#define CFG_KEY_AP_PASS         "ap_pass"

// Function declarations
bool configLoad();
bool configSave();
String configGet(const char* key, const char* def = "");
void configSet(const char* key, const char* value);
bool configGetBool(const char* key, bool def = false);
void configSetBool(const char* key, bool value);
void configApplyDefaults();

#endif // CONFIG_H