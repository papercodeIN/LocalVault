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

#if defined(ESP8266)
  #include <ESP8266mDNS.h>
#elif defined(ESP32)
  #include <ESPmDNS.h>
#endif

static LvWiFiMode currentMode = LV_WIFI_MODE_STA;
static unsigned long lastReconnectAttempt = 0;
static bool apStarted = false;
static int failedReconnectCount = 0;
static const int MAX_RECONNECT_ATTEMPTS = 10; // 10 attempts * 30s = 5 minutes

String wifiHostname() {
  String h = configGet(CFG_KEY_DEVICE_NAME, "localvault");
  h.toLowerCase();
  h.replace(" ", "-");
  if (h.length() == 0) h = "localvault";
  return h;
}

String wifiGetSSID() {
  if (WiFi.status() == WL_CONNECTED) {
    String s = WiFi.SSID();
    if (s.length()) return s;
  }
  return configGet(CFG_KEY_WIFI_SSID);
}

void wifiStartMDNS() {
  if (WiFi.status() != WL_CONNECTED) return;
#if defined(ESP8266) || defined(ESP32)
  String host = wifiHostname();
  if (MDNS.begin(host.c_str())) {
    MDNS.addService("http", "tcp", HTTP_PORT);
    Serial.printf("[mDNS] http://%s.local (%s)\n", host.c_str(), WiFi.localIP().toString().c_str());
  } else {
    Serial.println("[mDNS] start failed");
  }
#else
  (void)0;
#endif
}

bool wifiTryConnect(const String& ssid, const String& pass) {
  if (ssid.length() == 0) return false;
  Serial.printf("[WiFi] Trying '%s'...\n", ssid.c_str());
#if !defined(PICO_W)
  WiFi.setHostname(wifiHostname().c_str());
#endif
  if (configGetBool(CFG_KEY_USE_STATIC)) {
    IPAddress ip, gw, nm;
    ip.fromString(configGet(CFG_KEY_STATIC_IP));
    gw.fromString(configGet(CFG_KEY_STATIC_GATEWAY));
    nm.fromString(configGet(CFG_KEY_STATIC_NETMASK));
    WiFi.config(ip, gw, nm);
  }
#if defined(PICO_W)
  if (!apStarted) {
    // Start AP for Pico W
    String apSsid = configGet(CFG_KEY_AP_SSID);
    String apPass = configGet(CFG_KEY_AP_PASS);
    WiFi.softAP(apSsid.c_str(), apPass.length() > 0 ? apPass.c_str() : nullptr);
    apStarted = true;
    currentMode = LV_WIFI_MODE_AP_STA;
  }
#else
  WiFi.mode(WIFI_AP_STA);
  if (!apStarted) {
    // keep AP up while we try so the browser stays connected to receive the reply
    String apSsid = configGet(CFG_KEY_AP_SSID);
    String apPass = configGet(CFG_KEY_AP_PASS);
    WiFi.softAP(apSsid.c_str(), apPass.length() > 0 ? apPass.c_str() : nullptr);
    apStarted = true;
    currentMode = LV_WIFI_MODE_AP_STA;
  } else {
    WiFi.mode(WIFI_AP_STA);
  }
#endif
  WiFi.begin(ssid.c_str(), pass.c_str());
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECT_TIMEOUT_MS) {
    delay(100);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[WiFi] Connected! IP: %s SSID: %s\n", WiFi.localIP().toString().c_str(), WiFi.SSID().c_str());
    wifiStartMDNS();
    if (currentMode == LV_WIFI_MODE_AP) currentMode = LV_WIFI_MODE_AP_STA;
    else if (!apStarted) currentMode = LV_WIFI_MODE_STA;
    else currentMode = LV_WIFI_MODE_AP_STA;
    return true;
  }
  Serial.println("[WiFi] Connect failed (check SSID/password or signal)");
  return false;
}

void wifiInit() {
  // Load config first
  configLoad();
  configApplyDefaults();
  
  // Set hostname (not supported on Pico W)
#if !defined(PICO_W)
  WiFi.setHostname(configGet(CFG_KEY_DEVICE_NAME).c_str());
#endif
  
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
#if defined(PICO_W)
    WiFi.begin(ssid.c_str(), pass.c_str());
#else
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());
#endif
    currentMode = LV_WIFI_MODE_STA;
    
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECT_TIMEOUT_MS) {
      delay(100);
    }
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
    currentMode = LV_WIFI_MODE_STA;
    failedReconnectCount = 0; // Reset counter on successful connection
    wifiStopAP();
    wifiStartMDNS();
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
      Serial.printf("[WiFi] Connection lost, attempting reconnect (%d/%d)...\n", 
                    failedReconnectCount + 1, MAX_RECONNECT_ATTEMPTS);
      wifiReconnect();
      lastReconnectAttempt = millis();
      
      // If still not connected, increment counter
      if (WiFi.status() != WL_CONNECTED) {
        failedReconnectCount++;
        
        // After MAX_RECONNECT_ATTEMPTS, fall back to AP mode
        if (failedReconnectCount >= MAX_RECONNECT_ATTEMPTS) {
          Serial.printf("[WiFi] Failed to reconnect after %d attempts, starting AP mode\n", 
                        MAX_RECONNECT_ATTEMPTS);
          wifiStartAP();
          failedReconnectCount = 0; // Reset counter
        }
      } else {
        // Successfully reconnected, reset counter
        failedReconnectCount = 0;
      }
    }
  }
  
  // In AP+STA mode, also check if STA connected and we can drop AP
  if (currentMode == LV_WIFI_MODE_AP_STA && WiFi.status() == WL_CONNECTED) {
    Serial.println("[WiFi] STA connected, stopping AP");
    wifiStopAP();
    currentMode = LV_WIFI_MODE_STA;
    failedReconnectCount = 0; // Reset counter on successful connection
  }
}

LvWiFiMode wifiGetMode() { return currentMode; }

bool wifiIsConnected() { return WiFi.status() == WL_CONNECTED; }

String wifiGetIP() {
  if (WiFi.status() == WL_CONNECTED) {
    return WiFi.localIP().toString();
  }
  return WiFi.softAPIP().toString();
}

void wifiStartAP() {
  if (apStarted) return;
  
  String apSsid = configGet(CFG_KEY_AP_SSID);
  String apPass = configGet(CFG_KEY_AP_PASS);
  
#if !defined(PICO_W)
  WiFi.mode(WIFI_AP_STA);
#endif
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
#if !defined(PICO_W)
  WiFi.mode(WIFI_STA);
#endif
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