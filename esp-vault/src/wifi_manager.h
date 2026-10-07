#pragma once
/*
 * WiFi Manager - handles STA connection, AP fallback, reconnection, static IP
 */
#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "config.h"

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
#elif defined(ESP32)
  #include <WiFi.h>
#elif defined(PICO_W)
  #include <WiFi.h>
#endif

enum LvWiFiMode {
  LV_WIFI_MODE_STA,      // Connected to home WiFi
  LV_WIFI_MODE_AP,       // AP mode (config portal)
  LV_WIFI_MODE_AP_STA    // Both (AP for config while trying STA)
};

void wifiInit();
void wifiLoop();
LvWiFiMode wifiGetMode();
bool wifiIsConnected();
String wifiGetIP();
void wifiStartAP();
void wifiStopAP();
void wifiReconnect();
void wifiEraseCredentials();

#endif // WIFI_MANAGER_H