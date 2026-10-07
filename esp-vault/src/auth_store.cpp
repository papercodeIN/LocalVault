/*
 * Device-vault auth implementation.
 * - PIN: 4-8 digits stored in config.json (plain, same trust level as wifi_pass)
 * - Rate limit: 5 consecutive wrong PINs -> 60 s lockout (RAM only)
 * - Tokens: 128-bit random hex, persisted in /tokens.json (cap 12, evict oldest)
 * - Plain vault: atomic pass-through JSON write to /vault.plain.json
 */
#include "auth_store.h"
#include "config.h"
#include <ArduinoJson.h>

#if defined(ESP8266)
  #include <user_interface.h>
  static uint32_t hwRand() { return os_random(); }
#elif defined(ESP32)
  #include <esp_random.h>
  static uint32_t hwRand() { return esp_random(); }
#else
  #include <pico/rand.h>
  static uint32_t hwRand() { return get_rand_32(); }
#endif

#define MAX_TOKENS 12
#define PIN_FAIL_LIMIT 5
#define PIN_LOCK_MS 60000UL

static String s_tokens[MAX_TOKENS];
static int s_tokenCount = 0;
static int s_pinFails = 0;
static uint32_t s_pinLockUntil = 0;

/* ---------------- tokens ---------------- */

static void tokensSave() {
  JsonDocument doc;
  JsonArray arr = doc["tokens"].to<JsonArray>();
  for (int i = 0; i < s_tokenCount; i++) arr.add(s_tokens[i]);
  File f = LittleFS.open(TOKENS_FILE, "w");
  if (!f) return;
  serializeJson(doc, f);
  f.close();
}

void authInit() {
  s_tokenCount = 0;
  // stale temp file from an interrupted write
  if (LittleFS.exists(String(VAULT_PLAIN_FILE) + ".tmp")) LittleFS.remove(String(VAULT_PLAIN_FILE) + ".tmp");
  File f = LittleFS.open(TOKENS_FILE, "r");
  if (!f) return;
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return;
  for (JsonVariant v : doc["tokens"].as<JsonArray>()) {
    if (s_tokenCount >= MAX_TOKENS) break;
    String t = v.as<String>();
    if (t.length() == 32) s_tokens[s_tokenCount++] = t;
  }
  Serial.printf("[Auth] Loaded %d session token(s)\n", s_tokenCount);
}

String authIssueToken() {
  char buf[33];
  snprintf(buf, sizeof(buf), "%08x%08x%08x%08x",
           hwRand(), hwRand(), hwRand(), hwRand());
  String tok(buf);
  if (s_tokenCount >= MAX_TOKENS) {  // evict oldest
    for (int i = 1; i < s_tokenCount; i++) s_tokens[i - 1] = s_tokens[i];
    s_tokenCount--;
  }
  s_tokens[s_tokenCount++] = tok;
  tokensSave();
  Serial.println("[Auth] Issued session token");
  return tok;
}

bool authTokenValid(const String& tok) {
  if (tok.length() != 32) return false;
  for (int i = 0; i < s_tokenCount; i++)
    if (s_tokens[i] == tok) return true;
  return false;
}

bool authRevokeToken(const String& tok) {
  bool changed = false;
  for (int i = 0; i < s_tokenCount; i++) {
    if (s_tokens[i] == tok) {
      for (int j = i + 1; j < s_tokenCount; j++) s_tokens[j - 1] = s_tokens[j];
      s_tokenCount--;
      changed = true;
      break;
    }
  }
  if (changed) tokensSave();
  return changed;
}

void authRevokeAll() {
  s_tokenCount = 0;
  tokensSave();
}

/* ---------------- PIN + rate limit ---------------- */

int authRetryAfterSec() {
  if ((int32_t)(millis() - s_pinLockUntil) >= 0) return 0;
  return (int)((s_pinLockUntil - millis()) / 1000) + 1;
}

bool authPinSet() {
  return configGet(CFG_KEY_PIN).length() > 0;
}

static bool pinShapeOk(const String& pin) {
  if (pin.length() < 4 || pin.length() > 8) return false;
  for (unsigned i = 0; i < pin.length(); i++)
    if (!isDigit(pin[i])) return false;
  return true;
}

bool authSetPin(const String& pin) {
  if (!pinShapeOk(pin)) return false;
  configSet(CFG_KEY_PIN, pin.c_str());
  return configSave();
}

bool authValidatePin(const String& pin) {
  if (authRetryAfterSec() > 0) return false;      // locked out
  if (!pinShapeOk(pin)) return false;
  if (pin == configGet(CFG_KEY_PIN)) {
    s_pinFails = 0;
    return true;
  }
  s_pinFails++;
  if (s_pinFails >= PIN_FAIL_LIMIT) {
    s_pinFails = 0;
    s_pinLockUntil = millis() + PIN_LOCK_MS;
    Serial.println("[Auth] PIN locked out for 60 s");
  }
  return false;
}

/* ---------------- plain vault file ---------------- */

bool plainVaultExists() {
  return LittleFS.exists(VAULT_PLAIN_FILE);
}

bool plainVaultOpenRead(File& out) {
  if (!LittleFS.exists(VAULT_PLAIN_FILE)) return false;
  out = LittleFS.open(VAULT_PLAIN_FILE, "r");
  return (bool)out;
}

bool plainVaultWrite(const String& body) {
  if (body.length() == 0 || body.length() > MAX_DEVICE_VAULT_BODY) return false;
  if (body[0] != '{') return false;
  String tmp = String(VAULT_PLAIN_FILE) + ".tmp";
  File f = LittleFS.open(tmp, "w");
  if (!f) return false;
  size_t written = f.print(body);
  f.close();
  if (written != body.length()) {
    LittleFS.remove(tmp);
    return false;
  }
  if (LittleFS.exists(VAULT_PLAIN_FILE)) LittleFS.remove(VAULT_PLAIN_FILE);
  if (!LittleFS.rename(tmp, VAULT_PLAIN_FILE)) {
    LittleFS.remove(tmp);
    return false;
  }
  Serial.printf("[Auth] Plain vault saved (%u bytes)\n", (unsigned)body.length());
  return true;
}

void plainVaultErase() {
  if (LittleFS.exists(VAULT_PLAIN_FILE)) LittleFS.remove(VAULT_PLAIN_FILE);
  if (LittleFS.exists(String(VAULT_PLAIN_FILE) + ".tmp")) LittleFS.remove(String(VAULT_PLAIN_FILE) + ".tmp");
  Serial.println("[Auth] Plain vault erased");
}
