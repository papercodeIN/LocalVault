/*
 * WiFi Manager implementation
 * Handles STA connection with fallback to AP mode, static IP config, reconnection
 */
#include "wifi_manager.h"
#include "config.h"
#include "vault_store.h"

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
#elif defined(ESP32)
  #include <WiFi.h>
#elif defined(PICO_W)
  #include <WiFi.h>
  #include <rp2040.h>
#endif

#include <LittleFS.h>

static LvWiFiMode currentMode = LV_WIFI_MODE_STA;
static unsigned long lastReconnectAttempt = 0;
static bool apStarted = false;

void wifiInit() {
  // Load config first
  configLoad();
  configApplyDefaults();
  
  // Set hostname
  WiFi.setHostname(configGet(CFG_KEY_DEVICE_NAME).c_str());
  
  // Configure static IP if enabled
  if (configGetBool(CFG_KEY_USE_STATIC)) {
    IPAddress ip, gw, nm;
    ip.fromString(configGet(CFG_KEY_STATIC_IP));
    gw.fromString(configGet(CFG_KEY_STATIC_GATEWAY));
    nm.fromString(configGet(CFG_KEY_STATIC_NETMASK));
    WiFi.config(ip, gw, nm);
  }
  
  // Try to connect to saved WiFi
  String ssid = configGet(CFG_KEY_WIFI_SSID);
  String pass = configGet(CFG_KEY_WIFI_PASS);
  
  if (ssid.length() > 0) {
    Serial.printf("[WiFi] Connecting to '%s'...\n", ssid.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());
    currentMode = LV_WIFI_MODE_STA;
    
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECT_TIMEOUT_MS) {
      delay(100);
    }
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
    currentMode = LV_WIFI_MODE_STA;
    wifiStopAP();

    // Only reset vault if it exists but is corrupted (cannot be parsed)
    if (vaultStoreExists()) {
      JsonDocument probe;
      if (!vaultStoreGet(probe)) {
        Serial.println("[VaultStore] Corrupted vault detected, resetting");
        vaultStoreReset();
      }
    }
  } else {
    Serial.println("[WiFi] STA failed, starting AP mode");
    wifiStartAP();
  }
  
  // Start periodic reconnection check
  lastReconnectAttempt = millis();
}

void wifiLoop() {
  // Periodic reconnection attempt in STA mode
  if (currentMode == LV_WIFI_MODE_STA && WiFi.status() != WL_CONNECTED) {
    if (millis() - lastReconnectAttempt > WIFI_RECONNECT_INTERVAL_MS) {
      Serial.println("[WiFi] Connection lost, attempting reconnect...");
      wifiReconnect();
      lastReconnectAttempt = millis();
    }
  }
  
  // In AP+STA mode, also check if STA connected and we can drop AP
  if (currentMode == LV_WIFI_MODE_AP_STA && WiFi.status() == WL_CONNECTED) {
    Serial.println("[WiFi] STA connected, stopping AP");
    wifiStopAP();
    currentMode = LV_WIFI_MODE_STA;
  }
}

LvWiFiMode wifiGetMode() { return currentMode; }

bool wifiIsConnected() { return WiFi.status() == WL_CONNECTED; }

String wifiGetIP() {
  if (currentMode == LV_WIFI_MODE_AP || currentMode == LV_WIFI_MODE_AP_STA) {
    return WiFi.softAPIP().toString();
  }
  return WiFi.localIP().toString();
}

void wifiStartAP() {
  if (apStarted) return;
  
  String apSsid = configGet(CFG_KEY_AP_SSID);
  String apPass = configGet(CFG_KEY_AP_PASS);
  
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(apSsid.c_str(), apPass.length() > 0 ? apPass.c_str() : nullptr);
  
  Serial.printf("[WiFi] AP started: %s (%s)\n", apSsid.c_str(), WiFi.softAPIP().toString().c_str());
  apStarted = true;
  
  if (currentMode == LV_WIFI_MODE_STA) {
    currentMode = LV_WIFI_MODE_AP_STA;
  } else {
    currentMode = LV_WIFI_MODE_AP;
  }
}

void wifiStopAP() {
  if (!apStarted) return;
  WiFi.softAPdisconnect(true);
  apStarted = false;
  Serial.println("[WiFi] AP stopped");
}

void wifiReconnect() {
  String ssid = configGet(CFG_KEY_WIFI_SSID);
  String pass = configGet(CFG_KEY_WIFI_PASS);
  
  if (ssid.length() == 0) {
    Serial.println("[WiFi] No credentials saved");
    return;
  }
  
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECT_TIMEOUT_MS) {
    delay(100);
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[WiFi] Reconnected! IP: %s\n", WiFi.localIP().toString().c_str());
    wifiStopAP();
    currentMode = LV_WIFI_MODE_STA;
  } else {
    Serial.println("[WiFi] Reconnect failed, staying in AP mode");
    wifiStartAP();
  }
}

void wifiEraseCredentials() {
  configSet(CFG_KEY_WIFI_SSID, "");
  configSet(CFG_KEY_WIFI_PASS, "");
  configSave();
  Serial.println("[WiFi] Credentials erased");
  // Restart to enter AP mode
#if defined(ESP8266) || defined(ESP32)
  ESP.restart();
#elif defined(PICO_W)
  rp2040.restart();
#endif
}