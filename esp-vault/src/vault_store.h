#pragma once
/*
 * Vault Store - mirrors /api/store API from server.js
 * GET  /api/store -> returns {blob, meta, rec, savedAt}
 * PUT  /api/store -> accepts {blob, meta, rec}, validates, atomically writes vault.store.json
 */
#ifndef VAULT_STORE_H
#define VAULT_STORE_H

#include <ArduinoJson.h>

bool vaultStoreInit();
bool vaultStoreGet(JsonDocument& out);
bool vaultStorePut(const JsonDocument& in);
bool vaultStoreExists();
void vaultStoreErase();
void vaultStoreReset();   // <-- added

#endif // VAULT_STORE_H