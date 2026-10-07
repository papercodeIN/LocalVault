/*
 * Configuration management - stored in LittleFS /config.json
 */
#include "config.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
#elif defined(ESP32)
  #include <WiFi.h>
#elif defined(PICO_W)
  #include <WiFi.h>
#endif

static JsonDocument configDoc;

bool configLoad() {
  File f = LittleFS.open(CONFIG_FILE, "r");
  if (!f) {
    Serial.println("[Config] No config file, using defaults");
    return false;
  }
  DeserializationError err = deserializeJson(configDoc, f);
  f.close();
  if (err) {
    Serial.printf("[Config] Parse error: %s\n", err.c_str());
    return false;
  }
  Serial.println("[Config] Loaded");
  return true;
}

bool configSave() {
  File f = LittleFS.open(CONFIG_FILE, "w");
  if (!f) {
    Serial.println("[Config] Failed to open for write");
    return false;
  }
  serializeJson(configDoc, f);
  f.close();
  Serial.println("[Config] Saved");
  return true;
}

String configGet(const char* key, const char* def) {
  if (configDoc[key].is<const char*>()) return configDoc[key].as<String>();
  return def;
}

void configSet(const char* key, const char* value) {
  configDoc[key] = value;
}

bool configGetBool(const char* key, bool def) {
  if (configDoc[key].is<bool>()) return configDoc[key].as<bool>();
  return def;
}

void configSetBool(const char* key, bool value) {
  configDoc[key] = value;
}

// Apply defaults if not set
void configApplyDefaults() {
  bool changed = false;
  if (!configDoc[CFG_KEY_DEVICE_NAME].is<const char*>()) {
    configDoc[CFG_KEY_DEVICE_NAME] = DEVICE_NAME; changed = true;
  }
  if (!configDoc[CFG_KEY_AP_SSID].is<const char*>()) {
    String mac = WiFi.macAddress(); mac.replace(":", "");
    configDoc[CFG_KEY_AP_SSID] = String(AP_SSID_PREFIX) + mac.substring(mac.length()-6); changed = true;
  }
  if (!configDoc[CFG_KEY_AP_PASS].is<const char*>()) {
    configDoc[CFG_KEY_AP_PASS] = AP_PASSWORD; changed = true;
  }
  if (!configDoc[CFG_KEY_USE_STATIC].is<bool>()) {
    configDoc[CFG_KEY_USE_STATIC] = false; changed = true;
  }
  if (!configDoc[CFG_KEY_STATIC_IP].is<const char*>()) {
    configDoc[CFG_KEY_STATIC_IP] = DEFAULT_STATIC_IP; changed = true;
  }
  if (!configDoc[CFG_KEY_STATIC_GATEWAY].is<const char*>()) {
    configDoc[CFG_KEY_STATIC_GATEWAY] = DEFAULT_GATEWAY; changed = true;
  }
  if (!configDoc[CFG_KEY_STATIC_NETMASK].is<const char*>()) {
    configDoc[CFG_KEY_STATIC_NETMASK] = DEFAULT_NETMASK; changed = true;
  }
  if (!configDoc[CFG_KEY_VAULT_MODE].is<const char*>()) {
    configDoc[CFG_KEY_VAULT_MODE] = "device"; changed = true;
  }
  if (changed) configSave();
}