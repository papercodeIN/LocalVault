# 🔐 LocalVault ESP — Offline Password Manager on Microcontrollers

Run LocalVault entirely on an **ESP32-C3**, **ESP8266 (Wemos D1 Mini)**, or **Raspberry Pi Pico W**.  
No server, no cloud — the device *is* the vault. Plug it in, connect to WiFi, open the IP in any browser.

## ✨ What you get

| Feature | Details |
|---------|---------|
| 🌐 **Self-hosted on device** | ESP serves the full LocalVault HTML (gzipped, ~42 KB) |
| 🔒 **Same encryption** | AES-256-GCM, PBKDF2-SHA-256 — keys never leave the browser |
| 📶 **WiFi setup portal** | First boot → AP mode → connect → enter home WiFi → done |
| 📌 **Static IP support** | Set a fixed IP via web UI |
| 🔄 **Auto-reconnect** | Falls back to AP if WiFi drops, reconnects when back |
| 💾 **Encrypted vault on flash** | Mirrors `/api/store` API — same format as Node server |
| 🌈 **WebCrypto polyfill** | Works over plain HTTP (no HTTPS needed on LAN) |
| 🖥️ **Three build methods** | PlatformIO, Arduino IDE, or **one-click browser flash** |

---

## 🚀 Quick start (browser flash — easiest)

> **Requires HTTPS** (or `localhost`). Safari/iOS not supported (no Web Serial).

1. Open the hosted installer: **[https://your-domain.com/esp-vault/web-flash/](https://your-domain.com/esp-vault/web-flash/)**  
   (or serve locally: `python -m http.server 8000 --directory esp-vault/web-flash` then `https://localhost:8000` via `mkcert`)
2. Select your board: **ESP32-C3**, **D1 Mini**, or **Pico W**
3. Click **Connect Device & Install** → pick your device in the browser dialog
4. Wait for flash → device restarts → creates AP **LocalVault-XXXXXX**
5. Connect to that AP → open **http://192.168.4.1**
6. Scan for your home WiFi, enter password, (optionally set static IP) → **Save**
7. Device restarts, joins your network → find its IP (router / AP page)
8. Open **http://<device-ip>** in any browser → **LocalVault loads!**

---

## 🛠️ Build & flash yourself

### Option 1: PlatformIO (recommended)

```bash
cd esp-vault

# Build all three
pio run

# Build & upload specific board
pio run -e esp32c3 -t upload
pio run -e d1_mini -t upload
pio run -e pico_w -t upload

# Monitor serial
pio device monitor -e esp32c3
```

**Requirements:** Python 3.8+, PlatformIO Core (`pip install platformio`)  
**Windows:** Enable long paths first (see [docs/PLATFORMIO.md](docs/PLATFORMIO.md))

[Full PlatformIO guide →](docs/PLATFORMIO.md)

---

### Option 2: Arduino IDE

1. Install board support:
   - **ESP32-C3**: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json` → Boards Manager → `esp32` → Tools → Board → **ESP32C3 Dev Module** → Partition Scheme: **Default 4MB with spiffs (1.5MB SPIFFS)**
   - **ESP8266 D1 Mini**: `https://arduino.esp8266.com/stable/package_esp8266com_index.json` → Boards Manager → `esp8266` → Tools → Board → **LOLIN(WEMOS) D1 R2 & mini** → Flash Size: **4MB (FS:1MB)**
   - **Pico W**: `https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json` → Boards Manager → `Raspberry Pi Pico/RP2040` → Tools → Board → **Raspberry Pi Pico W** → Flash Size: **2 MB (1 MB FS)**
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
- `firmware_esp32c3.bin`, `firmware_d1_mini.bin`, `firmware_pico_w.uf2`

**To rebuild firmware after changes:**
```bash
cd esp-vault
pio run -e esp32c3 -e d1_mini -e pico_w
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
│   ├── vault_store.h/.cpp  # /api/store mirror (atomic writes)
│   ├── http_server.h/.cpp  # WebServer + API endpoints
│   └── localvault_html.h   # Auto-generated: gzipped HTML + polyfill
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

## 🔌 HTTP API (same as Node `server.js`)

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/` | GET | LocalVault.html (gzipped, with WebCrypto polyfill) |
| `/health` | GET | `{ok, version, device, mode, ip, uptime, freeHeap, vaultExists}` |
| `/api/store` | GET | Returns `{blob, meta, rec, savedAt}` |
| `/api/store` | PUT | Accepts `{blob, meta, rec}` → validates → atomic write |
| `/api/config` | GET | Current WiFi/static IP/AP settings |
| `/api/config` | POST | Update config (device name, static IP, AP creds) |
| `/api/wifi/scan` | POST | Scan networks → `[{ssid, rssi, encryption}]` |
| `/api/wifi/connect` | POST | Save credentials & connect |
| `/api/wifi/forget` | POST | Erase WiFi, enter AP mode |
| `/api/restart` | POST | Restart device |

---

## 💡 How it works

1. **First boot** — no WiFi saved → starts AP `LocalVault-XXXXXX` (open)
2. **User connects** → browser opens `http://192.168.4.1` → setup UI
3. **User enters home WiFi** → device saves to `/config.json` → restarts
4. **STA mode** — connects to home WiFi, serves vault on that IP
5. **If WiFi lost** → auto-reconnect every 30s; after timeout → falls back to AP
6. **Vault data** — stored encrypted in LittleFS `/vault.store.json` (mirrors browser `localStorage`)
7. **Browser** — loads HTML from device, uses `/api/store` to sync vault (same as Node server)

---

## ⚠️ Limitations (honest)

- **Plain HTTP** — vault works because we inject a WebCrypto polyfill, but traffic is visible on LAN. For sensitive use, put device on a trusted network or add TLS (mkcert + cert upload).
- **No RTC** — `savedAt` in `/api/store` uses `millis()` (uptime), not wall time.
- **Single-user** — no auth on device API; anyone on LAN can read/write vault ciphertext.
- **Pico W flash** — 2 MB flash limits sketch to ~1 MB; 1 MB LittleFS for vault.
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