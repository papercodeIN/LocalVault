/*
 * HTTP Server - serves WiFi setup portal in AP mode,
 * and LocalVault vault (gzipped with WebCrypto polyfill) in STA mode.
 * Also mirrors /api/store from server.js, and WiFi APIs.
 */
#include "http_server.h"
#include "config.h"
#include "vault_store.h"
#include "auth_store.h"
#include "wifi_manager.h"
#include "localvault_html.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#if defined(ESP8266)
  #include <ESP8266WiFi.h>
#elif defined(ESP32)
  #include <WiFi.h>
#elif defined(PICO_W)
  #include <WiFi.h>
#endif

static WebServer server(HTTP_PORT);

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
void handleApiAuthStatus();
void handleApiAuthLogin();
void handleApiAuthLogout();
void handleApiPinSetup();
void handleApiVaultGet();
void handleApiVaultPut();
void handleApiVaultDelete();
void handleNotFound();
void sendCorsHeaders();
bool parseJsonBody(JsonDocument& doc);
static String sessionCookie();
static bool requireAuth();
static void sendAuthRequired();
static void issueSessionCookie();

static const char HTML_WIFI_SETUP[] =
"<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'>"
"<meta name='viewport' content='width=device-width,initial-scale=1.0'>"
"<title>LocalVault Setup</title>"
"<style>*{box-sizing:border-box;margin:0;padding:0}"
"body{font-family:system-ui,-apple-system,sans-serif;background:#0f172a;min-height:100vh;display:flex;align-items:center;justify-content:center;padding:1rem}"
".c{background:#fff;border-radius:16px;padding:2rem;width:100%;max-width:420px;box-shadow:0 8px 32px rgba(0,0,0,.2)}"
"h1{color:#0f172a;margin-bottom:.25rem;font-size:1.35rem}"
".s{color:#64748b;margin-bottom:1.5rem;font-size:.9rem;line-height:1.4}"
".f{margin-bottom:1rem}.f label{display:block;margin-bottom:.35rem;font-weight:600;color:#334155;font-size:.85rem}"
".f input{width:100%;padding:.75rem .9rem;border:1.5px solid #e2e8f0;border-radius:10px;font-size:1rem}"
".f input:focus{outline:none;border-color:#4f46e5}"
".btn{width:100%;padding:.85rem;background:#4f46e5;color:#fff;border:none;border-radius:10px;font-size:1rem;font-weight:700;cursor:pointer;margin-top:.5rem}"
".btn:hover{background:#4338ca}.btn:disabled{opacity:.5;cursor:not-allowed}"
".i{margin-top:1rem;padding:.75rem .9rem;background:#eef2ff;border-radius:10px;font-size:.82rem;color:#3730a3;line-height:1.5}"
".msg{margin-top:1rem;padding:.75rem;border-radius:10px;font-size:.9rem;display:none}"
".msg.ok{background:#ecfdf5;color:#065f46;display:block}"
".msg.er{background:#fef2f2;color:#991b1b;display:block}"
".mode{display:flex;flex-direction:column;gap:.5rem}"
".mode label{display:flex;gap:.6rem;align-items:flex-start;padding:.65rem .75rem;border:1.5px solid #e2e8f0;border-radius:10px;cursor:pointer;font-size:.82rem;color:#475569;line-height:1.45}"
".mode label:has(input:checked){border-color:#4f46e5;background:#eef2ff;color:#3730a3}"
".mode input{margin-top:.2rem;accent-color:#4f46e5}"
".mode b{color:#0f172a}.mode label:has(input:checked) b{color:#3730a3}"
"</style></head><body><div class='c'>"
"<h1>🔐 LocalVault</h1><p class='s'>Connect your ESP to your home WiFi. After it restarts you can open it at <b>http://localvault.local</b> or at the IP shown below.</p>"
"<div class='i' style='border:1px solid #f59e0b;background:#fffbeb;color:#92400e'>⚠️ <b>Important:</b> this page uses plain HTTP on your LAN, so browsers block real WebCrypto (<i>AES-GCM</i>). This build ships a compatible fallback — vaults created here <b>only open on this ESP-served page</b> (not on localvault.app or file://). Keep LAN trusted.</div>"
"<form id='wf'>"
"<div class='f'><label>Vault mode</label><div class='mode'>"
"<label><input type='radio' name='vm' value='device' checked><span><b>Device vault (recommended)</b> — data stored on this ESP, protected by a short device PIN. Open it from any browser on your network: PIN once per browser, then it remembers you.</span></label>"
"<label><input type='radio' name='vm' value='zk'><span><b>Encrypted vault</b> — ESP stores only ciphertext. Master password required in every new browser (zero-knowledge).</span></label>"
"</div></div>"
"<div class='f'><label>WiFi network (SSID)</label><input id='ssid' name='ssid' placeholder='e.g. Capgemini_4G' required autocomplete='off'></div>"
"<div class='f'><label>Password</label><input id='pass' name='pass' type='password' placeholder='WiFi password'></div>"
"<button class='btn' id='btn' type='submit'>Connect to WiFi</button></form>"
"<div id='msg' class='msg'></div>"
"<div class='i'>💡 After tapping Connect, the device will restart and join your home network. Reconnect your phone/PC to your home WiFi, then open <b>http://localvault.local</b> or its IP.<br><b>Device vault:</b> first visit asks you to create a 4-8 digit device PIN — after that every browser opens your vault with just that PIN.<br><b>Encrypted vault:</b> create master password + download recovery key inside the vault.</div>"
"</div><script>"
"const f=document.getElementById('wf'),m=document.getElementById('msg'),b=document.getElementById('btn');"
"f.addEventListener('submit',async(e)=>{e.preventDefault();b.disabled=true;b.textContent='Connecting…';m.className='msg';m.textContent='';"
"try{const vm=document.querySelector('input[name=vm]:checked').value;"
"await fetch('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({vaultMode:vm})}).catch(()=>{});"
"const d=new URLSearchParams(new FormData(f)).toString();"
"const r=await fetch('/api/wifi/connect',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:d});"
"const j=await r.json();"
"if(j.ok){m.className='msg ok';m.textContent='✓ Saved. ESP is restarting and joining your WiFi. Reconnect your device to your home WiFi, then open http://localvault.local or http://'+location.hostname+' (new IP). This page will stop responding.';b.textContent='Done';}else{m.className='msg er';m.textContent='✗ '+(j.error||'Failed');b.disabled=false;b.textContent='Connect to WiFi';}"
"}catch(err){m.className='msg er';m.textContent='✗ Network error: '+err.message;b.disabled=false;b.textContent='Connect to WiFi';}"
"});</script></body></html>";

void httpServerInit() {
  server.enableCORS(true);
#if defined(ESP32)
  const char* cookieHeaders[] = {"Cookie"};
  server.collectHeaders(cookieHeaders, 1);
#else
  server.collectHeaders("Cookie");
#endif
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
  server.on("/api/auth", HTTP_GET, handleApiAuthStatus);
  server.on("/api/auth", HTTP_POST, handleApiAuthLogin);
  server.on("/api/auth/logout", HTTP_POST, handleApiAuthLogout);
  server.on("/api/pin/setup", HTTP_POST, handleApiPinSetup);
  server.on("/api/vault", HTTP_GET, handleApiVaultGet);
  server.on("/api/vault", HTTP_PUT, handleApiVaultPut);
  server.on("/api/vault", HTTP_DELETE, handleApiVaultDelete);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.printf("[HTTP] Server started on port %d\n", HTTP_PORT);
}

void httpServerLoop() { server.handleClient(); }

void sendCorsHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, PUT, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void handleRoot() {
  sendCorsHeaders();
  if (WiFi.status() == WL_CONNECTED) {
    server.sendHeader("Content-Encoding", "gzip");
    server.sendHeader("Content-Type", "text/html; charset=utf-8");
    server.sendHeader("Cache-Control", "no-store, must-revalidate");
    server.send_P(200, "text/html", (const char*)LOCALVAULT_HTML_GZ, LOCALVAULT_HTML_GZ_LEN);
  } else {
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "text/html", HTML_WIFI_SETUP);
  }
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
  if (err) { Serial.printf("[HTTP] JSON parse error: %s\n", err.c_str()); return false; }
  return true;
}

/* ---------- device-vault auth helpers ---------- */

static String sessionCookie() {
  if (!server.hasHeader("Cookie")) return "";
  String c = server.header("Cookie");
  int i = c.indexOf("lv_sess=");
  if (i < 0) return "";
  int s = i + 8;
  int e = c.indexOf(';', s);
  return (e < 0) ? c.substring(s) : c.substring(s, e);
}

static bool requireAuth() {
  String tok = sessionCookie();
  return tok.length() > 0 && authTokenValid(tok);
}

static void sendAuthRequired() {
  sendCorsHeaders();
  server.send(401, "application/json", "{\"ok\":false,\"error\":\"unauthorized\"}");
}

static void issueSessionCookie() {
  String tok = authIssueToken();
  server.sendHeader("Set-Cookie",
                    "lv_sess=" + tok + "; Path=/; Max-Age=31536000; SameSite=Strict; HttpOnly");
}

void handleApiAuthStatus() {
  sendCorsHeaders();
  JsonDocument doc;
  doc["ok"] = true;
  doc["authed"] = requireAuth();
  doc["pinSet"] = authPinSet();
  doc["retryAfter"] = authRetryAfterSec();
  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

void handleApiAuthLogin() {
  sendCorsHeaders();
  int ra = authRetryAfterSec();
  if (ra > 0) {
    server.send(429, "application/json",
                "{\"ok\":false,\"error\":\"rate_limited\",\"retryAfter\":" + String(ra) + "}");
    return;
  }
  JsonDocument doc;
  if (!parseJsonBody(doc)) { server.send(400, "application/json", "{\"ok\":false,\"error\":\"bad JSON\"}"); return; }
  String pin = doc["pin"] | "";
  if (!authPinSet()) { server.send(400, "application/json", "{\"ok\":false,\"error\":\"pin_not_set\"}"); return; }
  if (!authValidatePin(pin)) {
    server.send(401, "application/json",
                "{\"ok\":false,\"error\":\"wrong_pin\",\"retryAfter\":" + String(authRetryAfterSec()) + "}");
    return;
  }
  issueSessionCookie();
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleApiAuthLogout() {
  sendCorsHeaders();
  String tok = sessionCookie();
  if (tok.length()) authRevokeToken(tok);
  server.sendHeader("Set-Cookie", "lv_sess=; Path=/; Max-Age=0");
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleApiPinSetup() {
  sendCorsHeaders();
  JsonDocument doc;
  if (!parseJsonBody(doc)) { server.send(400, "application/json", "{\"ok\":false,\"error\":\"bad JSON\"}"); return; }
  String pin = doc["pin"] | "";
  if (pin.length() < 4 || pin.length() > 8) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"bad_pin\"}");
    return;
  }
  if (authPinSet()) {
    if (!requireAuth()) { server.send(409, "application/json", "{\"ok\":false,\"error\":\"pin_already_set\"}"); return; }
    String old = doc["old"] | "";
    if (old.length() && !authValidatePin(old)) {
      server.send(401, "application/json", "{\"ok\":false,\"error\":\"wrong_pin\"}");
      return;
    }
  }
  if (!authSetPin(pin)) { server.send(500, "application/json", "{\"ok\":false,\"error\":\"save failed\"}"); return; }
  issueSessionCookie();
  server.send(200, "application/json", "{\"ok\":true}");
}

/* ---------- device-vault plain JSON store ---------- */

void handleApiVaultGet() {
  sendCorsHeaders();
  if (!requireAuth()) { sendAuthRequired(); return; }
  File f;
  if (!plainVaultOpenRead(f)) {
    server.send(404, "application/json", "{\"ok\":false,\"error\":\"empty\"}");
    return;
  }
  server.sendHeader("Cache-Control", "no-store");
  server.streamFile(f, "application/json");
  f.close();
}

void handleApiVaultPut() {
  sendCorsHeaders();
  if (!requireAuth()) { sendAuthRequired(); return; }
  if (!server.hasArg("plain")) { server.send(400, "application/json", "{\"ok\":false,\"error\":\"empty body\"}"); return; }
  const String& body = server.arg("plain");
  if (body.length() > MAX_DEVICE_VAULT_BODY) {
    server.send(413, "application/json", "{\"ok\":false,\"error\":\"too large\",\"max\":" + String(MAX_DEVICE_VAULT_BODY) + "}");
    return;
  }
  if (body.length() < 2 || body[0] != '{') {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"not a JSON object\"}");
    return;
  }
  if (plainVaultWrite(body)) server.send(200, "application/json", "{\"ok\":true}");
  else server.send(500, "application/json", "{\"ok\":false,\"error\":\"write failed\"}");
}

void handleApiVaultDelete() {
  sendCorsHeaders();
  if (!requireAuth()) { sendAuthRequired(); return; }
  plainVaultErase();
  authRevokeAll();
  server.sendHeader("Set-Cookie", "lv_sess=; Path=/; Max-Age=0");
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleApiStoreGet() {
  sendCorsHeaders();
  JsonDocument doc;
  if (vaultStoreGet(doc)) {
    String out; serializeJson(doc, out); server.send(200, "application/json", out);
  } else {
    sendCorsHeaders(); server.send(404, "application/json", "{\"ok\":false,\"error\":\"empty\"}");
  }
}

void handleApiStorePut() {
  sendCorsHeaders();
  JsonDocument doc;
  if (!parseJsonBody(doc)) { server.send(400, "application/json", "{\"ok\":false,\"error\":\"bad JSON\"}"); return; }
  if (vaultStorePut(doc)) server.send(200, "application/json", "{\"ok\":true}");
  else server.send(400, "application/json", "{\"ok\":false,\"error\":\"bad shape or too large\"}");
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
  doc["vaultMode"] = configGet(CFG_KEY_VAULT_MODE, "device");
  doc["pinSet"] = authPinSet();
  String out; serializeJson(doc, out); server.send(200, "application/json", out);
}

void handleApiConfigPost() {
  sendCorsHeaders();
  JsonDocument doc;
  if (!parseJsonBody(doc)) { server.send(400, "application/json", "{\"ok\":false,\"error\":\"bad JSON\"}"); return; }
  bool changed = false;
  if (doc[CFG_KEY_DEVICE_NAME].is<const char*>()) { configSet(CFG_KEY_DEVICE_NAME, doc[CFG_KEY_DEVICE_NAME]); changed = true; }
  if (doc[CFG_KEY_USE_STATIC].is<bool>()) { configSetBool(CFG_KEY_USE_STATIC, doc[CFG_KEY_USE_STATIC].as<bool>()); changed = true; }
  if (doc[CFG_KEY_STATIC_IP].is<const char*>()) { configSet(CFG_KEY_STATIC_IP, doc[CFG_KEY_STATIC_IP]); changed = true; }
  if (doc[CFG_KEY_STATIC_GATEWAY].is<const char*>()) { configSet(CFG_KEY_STATIC_GATEWAY, doc[CFG_KEY_STATIC_GATEWAY]); changed = true; }
  if (doc[CFG_KEY_STATIC_NETMASK].is<const char*>()) { configSet(CFG_KEY_STATIC_NETMASK, doc[CFG_KEY_STATIC_NETMASK]); changed = true; }
  if (doc[CFG_KEY_AP_SSID].is<const char*>()) { configSet(CFG_KEY_AP_SSID, doc[CFG_KEY_AP_SSID]); changed = true; }
  if (doc[CFG_KEY_AP_PASS].is<const char*>()) { configSet(CFG_KEY_AP_PASS, doc[CFG_KEY_AP_PASS]); changed = true; }
  if (doc[CFG_KEY_VAULT_MODE].is<const char*>()) {
    String m = doc[CFG_KEY_VAULT_MODE].as<String>();
    if (m == "device" || m == "zk") { configSet(CFG_KEY_VAULT_MODE, m.c_str()); changed = true; }
  }
  if (changed) configSave();
  JsonDocument resp; resp["ok"] = true; resp["restartRequired"] = changed;
  String out; serializeJson(resp, out); server.send(200, "application/json", out);
}

void handleApiWifiScan() {
  sendCorsHeaders();
  Serial.println("[HTTP] WiFi scan requested");
  int n = WiFi.scanNetworks();
  JsonDocument doc; JsonArray networks = doc["networks"].to<JsonArray>();
  for (int i = 0; i < n; i++) {
    JsonObject net = networks.add<JsonObject>();
    net["ssid"] = WiFi.SSID(i); net["rssi"] = WiFi.RSSI(i);
#if defined(ESP8266)
    net["encryption"] = WiFi.encryptionType(i) == AUTH_OPEN ? "open" : "secured";
#elif defined(ESP32)
    net["encryption"] = WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "open" : "secured";
#elif defined(PICO_W)
    net["encryption"] = WiFi.encryptionType(i) == CYW43_AUTH_OPEN ? "open" : "secured";
#endif
  }
  String out; serializeJson(doc, out); server.send(200, "application/json", out); WiFi.scanDelete();
}

void handleApiWifiConnect() {
  sendCorsHeaders();
  String ssid, pass;
  if (server.hasArg("plain") && server.arg("plain").length() > 0 && server.arg("ssid").length() == 0) {
    JsonDocument doc;
    if (parseJsonBody(doc)) { ssid = doc["ssid"] | ""; pass = doc["pass"] | ""; }
  }
  if (ssid.length() == 0) { ssid = server.arg("ssid"); pass = server.arg("pass"); }
  if (ssid.length() == 0) { server.send(400, "application/json", "{\"ok\":false,\"error\":\"ssid required\"}"); return; }
  configSet(CFG_KEY_WIFI_SSID, ssid.c_str());
  configSet(CFG_KEY_WIFI_PASS, pass.c_str());
  configSave();
  Serial.printf("[HTTP] WiFi credentials saved for '%s', restarting to connect\n", ssid.c_str());
  JsonDocument resp; resp["ok"] = true; resp["message"] = "Saved. Device restarting...";
  String out; serializeJson(resp, out); server.send(200, "application/json", out);
  delay(800);
#if defined(ESP8266) || defined(ESP32)
  ESP.restart();
#elif defined(PICO_W)
  rp2040.restart();
#endif
}

void handleApiWifiForget() {
  sendCorsHeaders(); wifiEraseCredentials();
  JsonDocument resp; resp["ok"] = true; resp["message"] = "Credentials erased, entering AP mode";
  String out; serializeJson(resp, out); server.send(200, "application/json", out);
}

void handleApiRestart() {
  sendCorsHeaders();
  JsonDocument resp; resp["ok"] = true; String out; serializeJson(resp, out);
  server.send(200, "application/json", out); delay(100);
#if defined(ESP8266) || defined(ESP32)
  ESP.restart();
#elif defined(PICO_W)
  rp2040.restart();
#endif
}

void handleNotFound() { sendCorsHeaders(); server.send(404, "text/plain", "Not found"); }
