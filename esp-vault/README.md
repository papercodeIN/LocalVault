# 🔐 ESPVault — Offline Password Manager on Microcontrollers

Run ESPVault entirely on an **ESP32**, **ESP32-C3**, or **ESP8266 (Wemos D1 Mini)**.  
No server, no cloud — the device *is* the vault. Plug it in, connect to WiFi, open the IP in any browser.

## ✨ What you get

| Feature | Details |
|---------|---------|
| 🌐 **Self-hosted on device** | ESP serves the full ESPVault HTML (gzipped, ~44 KB) |
| 🔒 **Same encryption** | AES-256-GCM, PBKDF2-SHA-256 — keys never leave the browser |
| 📶 **WiFi setup portal** | First boot → AP mode → connect → enter home WiFi → done |
| 📌 **mDNS discovery** | Access via `http://localvault.local` (ESP32/ESP8266 only) |
| 🔄 **Auto-reconnect** | Falls back to AP if WiFi drops, reconnects when back |
| 💾 **Two vault modes** | **Device vault** (PIN + plaintext) or **Encrypted vault** (master password + zero-knowledge) |
| 🔑 **Device vault mode** | 4–8 digit PIN once per browser (session cookie), plaintext storage, rate-limited (5 tries → 60 s lockout) |
| 🔐 **Encrypted vault mode** | Full master password (12+ chars), encrypted blob storage, zero-knowledge — same as browser LocalVault |
| 🌈 **WebCrypto polyfill** | Works over plain HTTP (no HTTPS needed on LAN) |
| 🖥️ **Three build methods** | PlatformIO, Arduino IDE, or **one-click browser flash** |

**Production Status (13-Oct-2026):**
- ✅ **ESP32** — Tested and working (broadcasts AP, serves web interface at 192.168.4.1)
- ✅ **ESP8266 (D1 Mini)** — Built and ready (previously tested working)
- ⚠️ **ESP32-C3** — Built successfully, hardware testing pending

**Critical Fix Applied:** Boot loop bug fixed by swapping initialization order in `src/main.cpp:75-76` — `wifiInit()` must run before `httpServerInit()` to prevent TCP/IP stack assertion failures.

---

## 🚀 Quick start (browser flash — easiest)

> **Requires HTTPS** (or `localhost`). Safari/iOS not supported (no Web Serial).

1. Open the hosted installer: **[https://your-domain.com/esp-vault/web-flash/](https://your-domain.com/esp-vault/web-flash/)**  
   (or serve locally: `python -m http.server 8000 --directory esp-vault/web-flash` then `https://localhost:8000` via `mkcert`)
2. Select your board: **ESP32**, **ESP32-C3**, or **D1 Mini (ESP8266)**
3. Click **Connect Device & Install** → pick your device in the browser dialog
4. Wait for flash → device restarts → creates AP **LocalVault-XXXXXX**
5. Connect to that AP → open **http://192.168.4.1**
6. Scan for your home WiFi, enter password, (optionally set static IP) → **Save**
7. Device restarts, joins your network → find its IP (router / AP page)
8. Open **http://<device-ip>** in any browser → **LocalVault loads!**

> **Vault mode** is chosen on the WiFi setup page (radio: *Device vault* vs *Zero-knowledge*).
> - **Device vault** (default) — no master password. Each new browser enters a **4–8 digit PIN** once, then a cookie keeps it unlocked. Vault is stored **plaintext** on the device (`/vault.plain.json`) and served in plain text to logged-in browsers.
> - **Zero-knowledge** — the classic LocalVault flow: encrypted blob in browser `localStorage`, master password per browser.

---

## 🛠️ Build & flash yourself

### Option 1: PlatformIO (recommended)

```bash
cd esp-vault

# Build all three
pio run

# Build & upload specific board
pio run -e esp32 -t upload
pio run -e esp32c3 -t upload
pio run -e d1_mini -t upload

# Monitor serial
pio device monitor -e esp32
```

**Requirements:** Python 3.8+, PlatformIO Core (`pip install platformio`)  
**Windows:** Enable long paths first (see [docs/PLATFORMIO.md](docs/PLATFORMIO.md))

[Full PlatformIO guide →](docs/PLATFORMIO.md)

---

### Option 2: Arduino IDE

1. Install board support:
   - **ESP32**: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json` → Boards Manager → `esp32` → Tools → Board → **ESP32 Dev Module** → Partition Scheme: **Default 4MB with spiffs (1.5MB SPIFFS)**
   - **ESP32-C3**: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json` → Boards Manager → `esp32` → Tools → Board → **ESP32C3 Dev Module** → Partition Scheme: **Default 4MB with spiffs (1.5MB SPIFFS)**
   - **ESP8266 D1 Mini**: `https://arduino.esp8266.com/stable/package_esp8266com_index.json` → Boards Manager → `esp8266` → Tools → Board → **LOLIN(WEMOS) D1 R2 & mini** → Flash Size: **4MB (FS:1MB)**
2. Install library: **ArduinoJson** (v7+)
3. Open `arduino-ide/LocalVault_ESP/LocalVault_ESP.ino`
4. Select board/port → **Upload**

[Full Arduino IDE guide →](docs/ARDUINO_IDE.md)

---

### Option 3: Host the browser flasher yourself

```bash
cd esp-vault
python -m http.server 8000 --directory web-flash
# Then use mkcert for HTTPS:
mkcert localhost
python -m http.server 8443 --directory web-flash --bind 0.0.0.0
# (requires a small HTTPS wrapper script)
```

The `web-flash/` folder contains:
- `index.html` — ESP Web Tools install button
- `manifest.json` — points to the three firmware files
- `firmware_esp32.bin`, `firmware_esp32c3.bin`, `firmware_d1_mini.bin`
- `bootloader_esp32.bin`, `partitions_esp32.bin` (for ESP32 multi-part flash)

### 🔧 ESP32 Bootloader Explained

**Why does ESP32 need 3 files while ESP32-C3/ESP8266 need only 1?**

The original ESP32 uses a **3-stage boot process**:

| Offset | File | Purpose |
|--------|------|---------|
| `0x1000` | `bootloader_esp32.bin` | Second-stage bootloader (initializes hardware, reads partition table) |
| `0x8000` | `partitions_esp32.bin` | Partition table (tells bootloader where app firmware is located) |
| `0x10000` | `firmware_esp32.bin` | Your actual ESPVault application |

**ESP32-C3 and ESP8266** use simpler boot architectures with everything bundled into one file at offset `0x0`.

**Flash requirements by board:**
- **ESP32**: ✅ 3 files required (bootloader + partitions + firmware)
- **ESP32-C3**: ✅ 1 file only (firmware)
- **ESP8266/D1 Mini**: ✅ 1 file only (firmware)

The ESP Web Tools in `index.html` automatically handles multi-part flashing based on `manifest.json`. PlatformIO generates the bootloader and partition table automatically during build.

**To rebuild firmware after changes:**
```bash
cd esp-vault
pio run -e esp32 -e esp32c3 -e d1_mini
python scripts/copy_firmware.py
```

---

## 📁 Project structure

```
esp-vault/
├── src/                    # Firmware source (C++)
│   ├── main.cpp            # Entry point
│   ├── config.h/.cpp       # LittleFS config (/config.json)
│   ├── wifi_manager.h/.cpp # STA + AP fallback, static IP
│   ├── vault_store.h/.cpp  # /vault.enc.json mirror (atomic writes)
│   ├── auth_store.h/.cpp   # device vault: PIN (config), session tokens, /vault.plain.json
│   ├── http_server.h/.cpp  # WebServer + API endpoints
│   └── localvault_html.h   # Auto-generated: gzipped HTML + polyfill + device shim
├── arduino-ide/
│   └── LocalVault_ESP/     # Single .ino for Arduino IDE
├── web-flash/              # Browser flasher (ESP Web Tools)
│   ├── index.html
│   ├── manifest.json
│   └── firmware_*.bin/.uf2
├── scripts/
│   ├── generate_html.py    # Embeds LocalVault.html + WebCrypto polyfill
│   └── copy_firmware.py    # Copies built bins to web-flash/
├── docs/
│   ├── ARDUINO_IDE.md
│   └── PLATFORMIO.md
└── platformio.ini          # Multi-env build config
```

---

## 🔌 HTTP API (mirrors Node `server.js` where applicable)

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/` | GET | LocalVault.html (gzipped, with WebCrypto polyfill + device shim) |
| `/health` | GET | `{ok, version, device, mode, ip, uptime, freeHeap, vaultExists}` |
| `/api/store` | GET | Returns `{blob, meta, rec, savedAt}` (zero-knowledge mode) |
| `/api/store` | PUT | Accepts `{blob, meta, rec}` → validates → atomic write |
| `/api/config` | GET | WiFi/static IP/AP settings + `{vaultMode, pinSet}` |
| `/api/config` | POST | Update config (device name, static IP, AP creds, `vaultMode`) |
| `/api/wifi/scan` | POST | Scan networks → `[{ssid, rssi, encryption}]` |
| `/api/wifi/connect` | POST | Save credentials & connect |
| `/api/wifi/forget` | POST | Erase WiFi, enter AP mode |
| `/api/restart` | POST | Restart device |
| `/api/auth` | GET | Device mode: `{pinSet, authed}` |
| `/api/auth` | POST | Device mode: login with PIN → `Set-Cookie: lv_sess=<token>` |
| `/api/auth/logout` | POST | Revoke current session token (clears cookie) |
| `/api/pin/setup` | POST | Device mode: create/change PIN (`{pin}`) |
| `/api/vault` | GET | Device mode: `{vault, meta}` plaintext (requires session) |
| `/api/vault` | PUT | Device mode: save `{vault, meta}` plaintext |
| `/api/vault` | DELETE | Device mode: erase vault |

---

## 💡 How it works

1. **First boot** — no WiFi saved → starts AP `LocalVault-XXXXXX` (open)
2. **User connects** → browser opens `http://192.168.4.1` → setup UI
3. **User enters home WiFi** → device saves to `/config.json` → restarts
4. **STA mode** — connects to home WiFi, serves vault on that IP
5. **If WiFi lost** → auto-reconnect every 30s; after timeout → falls back to AP
6. **Zero-knowledge vault** — encrypted blob in LittleFS `/vault.enc.json`, mirrors browser `localStorage`. Keys never leave the browser.
7. **Device vault** — PIN lives in config, session tokens in `/tokens.json` (max 12, oldest evicted), vault plaintext in `/vault.plain.json`. A slim JS shim injected into the served page bypasses the master-password screens and syncs straight to `/api/vault`.

---

## ⚠️ Limitations (honest)

- **Capacity** — RAM limits the vault file size: ~40–80 items on D1 Mini (24 KB limit), or ~300+ items on ESP32/ESP32-C3 (96 KB limit).
- **Plain HTTP** — vault works because we inject a WebCrypto polyfill, but traffic is visible on LAN. For sensitive use, put device on a trusted network or add TLS (mkcert + cert upload).
- **No RTC** — `savedAt` in `/api/store` uses `millis()` (uptime), not wall time.
- **Single-user** — no auth on device API; anyone on LAN can read/write vault ciphertext.
- **ESP8266 RAM** — tight (80 KB); disable debug, use `-Os` if needed.
- **Browser Web Serial** — requires Chrome/Edge/Firefox on desktop/Android; **no Safari/iOS**.

---

## 🔗 Related

- Main LocalVault: [`../LocalVault.html`](../LocalVault.html) — double-click to run in browser
- Node self-hosted: [`../selfhosted/`](../selfhosted/) — loopback server with disk mirror
- LocalVault docs: [`../README.md`](../README.md)

---

## 📄 License

MIT — see [`../../LICENSE`](../../LICENSE) (or root of repo).