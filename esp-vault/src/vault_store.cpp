/*
 * Vault Store - mirrors server.js /api/store exactly
 * Stores {blob, meta, rec, savedAt} as JSON in LittleFS
 * Atomic write via temp file + rename
 * Validates shape and 25 MB cap
 */
#include "vault_store.h"
#include "config.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

bool vaultStoreInit() {
  // LittleFS is mounted in main.cpp before this is called
  return LittleFS.exists(VAULT_FILE);
}

bool vaultStoreExists() {
  return LittleFS.exists(VAULT_FILE);
}

bool vaultStoreGet(JsonDocument& out) {
  File f = LittleFS.open(VAULT_FILE, "r");
  if (!f) return false;
  
  DeserializationError err = deserializeJson(out, f);
  f.close();
  
  if (err) {
    Serial.printf("[VaultStore] Parse error: %s\n", err.c_str());
    return false;
  }
  return true;
}

bool vaultStorePut(const JsonDocument& in) {
  // Validate shape - matches server.js:80-90
  if (!in["blob"].is<const char*>() && !in["blob"].isNull()) return false;
  if (!in["meta"].is<const char*>() && !in["meta"].isNull()) return false;
  if (!in["rec"].is<const char*>() && !in["rec"].isNull()) return false;
  
  // Check size cap (25 MB)
  if (in["blob"].is<const char*>() && strlen(in["blob"]) > MAX_API_BODY) return false;
  if (in["meta"].is<const char*>() && strlen(in["meta"]) > MAX_API_BODY) return false;
  if (in["rec"].is<const char*>() && strlen(in["rec"]) > MAX_API_BODY) return false;
  
  // Validate blob structure if present
  if (in["blob"].is<const char*>()) {
    JsonDocument blobDoc;
    DeserializationError err = deserializeJson(blobDoc, in["blob"]);
    if (err || !blobDoc["salt"] || !blobDoc["data"]) return false;
  }
  
  // Build output document
  JsonDocument out;
  out["blob"] = in["blob"] | nullptr;
  out["meta"] = in["meta"] | nullptr;
  out["rec"]  = in["rec"] | nullptr;
  out["savedAt"] = millis();  // using uptime as timestamp (no RTC)
  
  // Atomic write: temp file + rename
  String tmpFile = String(VAULT_FILE) + ".tmp";
  File f = LittleFS.open(tmpFile, "w");
  if (!f) return false;
  
  serializeJson(out, f);
  f.close();
  
  // Rename (atomic on LittleFS)
  if (LittleFS.exists(VAULT_FILE)) LittleFS.remove(VAULT_FILE);
  if (!LittleFS.rename(tmpFile, VAULT_FILE)) {
    LittleFS.remove(tmpFile);
    return false;
  }
  
  Serial.println("[VaultStore] Saved");
  return true;
}

void vaultStoreErase() {
  if (LittleFS.exists(VAULT_FILE)) LittleFS.remove(VAULT_FILE);
  if (LittleFS.exists(String(VAULT_FILE) + ".tmp")) LittleFS.remove(String(VAULT_FILE) + ".tmp");
  Serial.println("[VaultStore] Erased");
}

/* -------------------------------------------------------------
   NEW: convenience wrapper – call from Wi‑Fi manager after STA mode
   ------------------------------------------------------------- */
void vaultStoreReset() {
  vaultStoreErase();          // delete any possibly corrupted vault
  Serial.println("[VaultStore] Vault reset (erased)");
}