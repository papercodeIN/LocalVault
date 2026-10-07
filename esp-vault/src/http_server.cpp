/*
 * HTTP Server implementation
 */
#include "http_server.h"
#include "config.h"
#include "vault_store.h"
#include "wifi_manager.h"
#include "localvault_html.h"
#include <ArduinoJson.h>
#include <LittleFS.h>

static WebServer server(HTTP_PORT);

// Forward declarations
void handleRoot();
void handleHealth();
void handleApiStoreGet();
void handleApiStorePut();
void handleApiConfigGet();
void handleApiConfigPost();
void handleApiWifiScan();
void handleApiWifiConnect();
void handleApiWifiForget();
void handleApiRestart();
void handleNotFound();
void sendCorsHeaders();
bool parseJsonBody(JsonDocument& doc);

void httpServerInit() {
  // CORS for browser access
  server.enableCORS(true);
  
  // Routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/health", HTTP_GET, handleHealth);
  server.on("/api/store", HTTP_GET, handleApiStoreGet);
  server.on("/api/store", HTTP_PUT, handleApiStorePut);
  server.on("/api/config", HTTP_GET, handleApiConfigGet);
  server.on("/api/config", HTTP_POST, handleApiConfigPost);
  server.on("/api/wifi/scan", HTTP_POST, handleApiWifiScan);
  server.on("/api/wifi/connect", HTTP_POST, handleApiWifiConnect);
  server.on("/api/wifi/forget", HTTP_POST, handleApiWifiForget);
  server.on("/api/restart", HTTP_POST, handleApiRestart);
  server.onNotFound(handleNotFound);
  
  server.begin();
  Serial.printf("[HTTP] Server started on port %d\n", HTTP_PORT);
}

void httpServerLoop() {
  server.handleClient();
}

void sendCorsHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, PUT, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void handleRoot() {
  // Serve gzipped HTML with proper headers
  sendCorsHeaders();
  server.sendHeader("Content-Encoding", "gzip");
  server.sendHeader("Content-Type", "text/html; charset=utf-8");
  server.sendHeader("Cache-Control", "no-store, must-revalidate");
  server.send_P(200, "text/html", (const char*)LOCALVAULT_HTML_GZ, LOCALVAULT_HTML_GZ_LEN);
}

void handleHealth() {
  sendCorsHeaders();
  JsonDocument doc;
  doc["ok"] = true;
  doc["app"] = "LocalVault";
  doc["version"] = FIRMWARE_VERSION;
  doc["device"] = configGet(CFG_KEY_DEVICE_NAME);
  doc["mode"] = (int)wifiGetMode();
  doc["ip"] = wifiGetIP();
  doc["uptime"] = millis();
#if defined(ESP8266) || defined(ESP32)
  doc["freeHeap"] = ESP.getFreeHeap();
#elif defined(PICO_W)
  doc["freeHeap"] = rp2040.getFreeHeap();
#endif
  doc["vaultExists"] = vaultStoreExists();
  
  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

bool parseJsonBody(JsonDocument& doc) {
  if (!server.hasArg("plain")) return false;
  String body = server.arg("plain");
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    Serial.printf("[HTTP] JSON parse error: %s\n", err.c_str());
    return false;
  }
  return true;
}

void handleApiStoreGet() {
  sendCorsHeaders();
  JsonDocument doc;
  if (vaultStoreGet(doc)) {
    String out;
    serializeJson(doc, out);
    server.send(200, "application/json", out);
  } else {
    sendCorsHeaders();
    server.send(404, "application/json", "{\"ok\":false,\"error\":\"empty\"}");
  }
}

void handleApiStorePut() {
  sendCorsHeaders();
  JsonDocument doc;
  if (!parseJsonBody(doc)) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"bad JSON\"}");
    return;
  }
  
  if (vaultStorePut(doc)) {
    server.send(200, "application/json", "{\"ok\":true}");
  } else {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"bad shape or too large\"}");
  }
}

void handleApiConfigGet() {
  sendCorsHeaders();
  JsonDocument doc;
  doc["deviceName"] = configGet(CFG_KEY_DEVICE_NAME);
  doc["wifiSsid"] = configGet(CFG_KEY_WIFI_SSID);
  doc["useStatic"] = configGetBool(CFG_KEY_USE_STATIC);
  doc["staticIp"] = configGet(CFG_KEY_STATIC_IP);
  doc["staticGateway"] = configGet(CFG_KEY_STATIC_GATEWAY);
  doc["staticNetmask"] = configGet(CFG_KEY_STATIC_NETMASK);
  doc["apSsid"] = configGet(CFG_KEY_AP_SSID);
  doc["apPass"] = configGet(CFG_KEY_AP_PASS);
  doc["mode"] = (int)wifiGetMode();
  doc["ip"] = wifiGetIP();
  doc["connected"] = wifiIsConnected();
  
  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

void handleApiConfigPost() {
  sendCorsHeaders();
  JsonDocument doc;
  if (!parseJsonBody(doc)) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"bad JSON\"}");
    return;
  }
  
  bool changed = false;
  if (doc[CFG_KEY_DEVICE_NAME].is<const char*>()) {
    configSet(CFG_KEY_DEVICE_NAME, doc[CFG_KEY_DEVICE_NAME]);
    changed = true;
  }
  if (doc[CFG_KEY_USE_STATIC].is<bool>()) {
    configSetBool(CFG_KEY_USE_STATIC, doc[CFG_KEY_USE_STATIC].as<bool>());
    changed = true;
  }
  if (doc[CFG_KEY_STATIC_IP].is<const char*>()) {
    configSet(CFG_KEY_STATIC_IP, doc[CFG_KEY_STATIC_IP]);
    changed = true;
  }
  if (doc[CFG_KEY_STATIC_GATEWAY].is<const char*>()) {
    configSet(CFG_KEY_STATIC_GATEWAY, doc[CFG_KEY_STATIC_GATEWAY]);
    changed = true;
  }
  if (doc[CFG_KEY_STATIC_NETMASK].is<const char*>()) {
    configSet(CFG_KEY_STATIC_NETMASK, doc[CFG_KEY_STATIC_NETMASK]);
    changed = true;
  }
  if (doc[CFG_KEY_AP_SSID].is<const char*>()) {
    configSet(CFG_KEY_AP_SSID, doc[CFG_KEY_AP_SSID]);
    changed = true;
  }
  if (doc[CFG_KEY_AP_PASS].is<const char*>()) {
    configSet(CFG_KEY_AP_PASS, doc[CFG_KEY_AP_PASS]);
    changed = true;
  }
  
  if (changed) configSave();
  
  JsonDocument resp;
  resp["ok"] = true;
  resp["restartRequired"] = changed;
  String out;
  serializeJson(resp, out);
  server.send(200, "application/json", out);
}

void handleApiWifiScan() {
  sendCorsHeaders();
  Serial.println("[HTTP] WiFi scan requested");
  int n = WiFi.scanNetworks();
  JsonDocument doc;
  JsonArray networks = doc["networks"].to<JsonArray>();
  for (int i = 0; i < n; i++) {
    JsonObject net = networks.add<JsonObject>();
    net["ssid"] = WiFi.SSID(i);
    net["rssi"] = WiFi.RSSI(i);
#if defined(ESP8266)
    net["encryption"] = WiFi.encryptionType(i) == AUTH_OPEN ? "open" : "secured";
#elif defined(ESP32)
    net["encryption"] = WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "open" : "secured";
#elif defined(PICO_W)
    net["encryption"] = WiFi.encryptionType(i) == CYW43_AUTH_OPEN ? "open" : "secured";
#endif
  }
  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
  WiFi.scanDelete();
}

void handleApiWifiConnect() {
  sendCorsHeaders();
  JsonDocument doc;
  if (!parseJsonBody(doc)) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"bad JSON\"}");
    return;
  }
  
  String ssid = doc["ssid"] | "";
  String pass = doc["pass"] | "";
  
  if (ssid.length() == 0) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"ssid required\"}");
    return;
  }
  
  // Save credentials and attempt connection
  configSet(CFG_KEY_WIFI_SSID, ssid.c_str());
  configSet(CFG_KEY_WIFI_PASS, pass.c_str());
  configSave();
  
  // Try to connect (will be handled by wifiLoop)
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  
  JsonDocument resp;
  resp["ok"] = true;
  resp["message"] = "Connecting...";
  String out;
  serializeJson(resp, out);
  server.send(200, "application/json", out);
}

void handleApiWifiForget() {
  sendCorsHeaders();
  wifiEraseCredentials();
  
  JsonDocument resp;
  resp["ok"] = true;
  resp["message"] = "Credentials erased, entering AP mode";
  String out;
  serializeJson(resp, out);
  server.send(200, "application/json", out);
}

void handleApiRestart() {
  sendCorsHeaders();
  JsonDocument resp;
  resp["ok"] = true;
  String out;
  serializeJson(resp, out);
  server.send(200, "application/json", out);
  delay(100);
#if defined(ESP8266) || defined(ESP32)
  ESP.restart();
#elif defined(PICO_W)
  rp2040.restart();
#endif
}

void handleNotFound() {
  sendCorsHeaders();
  server.send(404, "text/plain", "Not found");
}