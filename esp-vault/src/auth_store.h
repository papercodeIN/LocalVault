#pragma once
/*
 * Device-vault auth: device PIN + session tokens + plain JSON vault file.
 * Mode "zk" (zero-knowledge) never touches these — it uses vault_store.
 */
#include <Arduino.h>
#include <LittleFS.h>

// Session tokens (persisted in /tokens.json)
void authInit();
String authIssueToken();
bool authTokenValid(const String& tok);
bool authRevokeToken(const String& tok);
void authRevokeAll();

// Device PIN (stored in /config.json, like wifi_pass)
bool authPinSet();
bool authSetPin(const String& pin);
bool authValidatePin(const String& pin);  // respects + updates rate limit
int authRetryAfterSec();                  // 0 = not limited

// Plain vault file (/vault.plain.json) — pass-through JSON, atomic write
bool plainVaultExists();
bool plainVaultOpenRead(File& out);       // opens file for streaming, false if missing
bool plainVaultWrite(const String& body);
void plainVaultErase();
