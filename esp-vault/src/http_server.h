#pragma once
/*
 * HTTP Server - serves LocalVault.html (gzipped) and handles API
 * Endpoints:
 *   GET  /           -> LocalVault.html (with WebCrypto polyfill)
 *   GET  /health     -> {ok: true, ...}
 *   GET  /api/store  -> vault store (for browser pull)
 *   PUT  /api/store  -> vault store (for browser push)
 *   GET  /api/config -> current config (for setup UI)
 *   POST /api/config -> update config (WiFi, static IP, etc.)
 *   POST /api/wifi/scan -> scan networks
 *   POST /api/wifi/connect -> connect to WiFi
 *   POST /api/wifi/forget -> forget WiFi, enter AP mode
 *   POST /api/restart -> restart device
 */
#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#if defined(ESP8266)
  #include <ESP8266WebServer.h>
  #define WebServer ESP8266WebServer
#elif defined(ESP32)
  #include <WebServer.h>
#elif defined(PICO_W)
  #include <WebServer.h>
#endif

void httpServerInit();
void httpServerLoop();

#endif // HTTP_SERVER_H