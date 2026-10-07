# Arduino IDE Setup Guide

## Supported Boards

| Board | Core | Flash | Notes |
|-------|------|-------|-------|
| ESP32-C3 DevKitM-1, XIAO ESP32C3 | ESP32 Arduino Core v3+ | 4 MB | Select "ESP32C3 Dev Module" |
| Wemos D1 Mini, NodeMCU | ESP8266 Arduino Core v3+ | 4 MB | Select "LOLIN(WEMOS) D1 R2 & mini" |
| Raspberry Pi Pico W | Arduino-Pico (Earle Philhower) | 2 MB | Select "Raspberry Pi Pico W" |

---

## 1. Install Board Support

### ESP32 (includes ESP32-C3)
1. File → Preferences → Additional Boards Manager URLs:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
2. Tools → Board → Boards Manager → Search "esp32" → Install **esp32 by Espressif Systems** (v3.0+)
3. Tools → Board → ESP32 Arduino → **ESP32C3 Dev Module**
4. Tools → Partition Scheme → **Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)**
   - Or create custom: 1.5 MB for LittleFS
5. Tools → Flash Size → **4MB (32Mb)**
6. Tools → Upload Speed → **921600**

### ESP8266 (Wemos D1 Mini, NodeMCU)
1. File → Preferences → Additional Boards Manager URLs (add to existing):
   ```
   https://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```
2. Tools → Board → Boards Manager → Search "esp8266" → Install **esp8266 by ESP8266 Community** (v3.1+)
3. Tools → Board → ESP8266 → **LOLIN(WEMOS) D1 R2 & mini** (or your board)
4. Tools → Flash Size → **4MB (FS:1MB OTA:~1019KB)**
5. Tools → Upload Speed → **921600**

### Raspberry Pi Pico W
1. File → Preferences → Additional Boards Manager URLs (add to existing):
   ```
   https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
   ```
2. Tools → Board → Boards Manager → Search "pico" → Install **Raspberry Pi Pico/RP2040 by Earle F. Philhower** (v4.0+)
3. Tools → Board → Raspberry Pi RP2040 → **Raspberry Pi Pico W**
4. Tools → Flash Size → **2 MB (1 MB FS)**
5. Tools → Optimize → **Fast (-O2)** or **Small (-Os)**
6. Tools → Upload Method → **UF2** (hold BOOTSEL while plugging in) or **Serial**

> **Windows Users:** Enable long paths before installing Pico core (see [arduino-pico docs](https://arduino-pico.readthedocs.io/en/latest/platformio.html#important-steps-for-windows-users-before-installing)):
> ```
> git config --system core.longpaths true
> ```
> Then enable "Enable NTFS/Win32 long paths" in `gpedit.msc` and reboot.

---

## 2. Install Libraries

**Tools → Manage Libraries → Search and install:**
- **ArduinoJson** by Benoit Blanchon → version **7.0.4** or later

---

## 3. Open and Upload

1. Open `esp-vault/arduino-ide/LocalVault_ESP/LocalVault_ESP.ino`
2. Select your board and port
3. Click **Upload**

> **Pico W UF2 upload:** Hold BOOTSEL button, plug in USB, release BOOTSEL. A drive "RPI-RP2" appears. Drag the `.uf2` file from the build folder onto it.

---

## 4. First Boot

1. Device creates WiFi AP: **LocalVault-XXXXXX** (last 6 chars of MAC)
2. Connect your phone/laptop to this AP (no password by default)
3. Open browser → **http://192.168.4.1**
4. You'll see the setup page:
   - Scan for your home WiFi
   - Enter password
   - (Optional) Set static IP
   - Save → device restarts and joins your network
5. Find its IP (check router, or use the AP again to see current IP)
6. Open **http://<device-ip>** — LocalVault loads!

---

## 5. Reconfigure WiFi

If WiFi fails or you want to change networks:
1. Connect to the AP again (device falls back to AP if STA fails)
2. Go to **http://192.168.4.1** → "Forget WiFi" → enter new credentials
3. Or use the web UI at the device's IP: Settings → WiFi → "Reconfigure"

---

## Partition / Filesystem Notes

| Board | Flash | Sketch Max | LittleFS | How to set |
|-------|-------|------------|----------|------------|
| ESP32-C3 | 4 MB | ~2.5 MB | 1.5 MB | Partition Scheme menu |
| ESP8266 | 4 MB | ~3 MB | 1 MB | Flash Size menu (FS:1MB) |
| Pico W | 2 MB | ~1 MB | 1 MB | Flash Size menu |

The gzipped HTML + polyfill is ~42 KB. With 1 MB+ LittleFS you have plenty of room for vault data.

---

## Troubleshooting

| Issue | Fix |
|-------|-----|
| "LittleFS mount failed" | First boot formats automatically. If persistent, hold BOOT/FLASH button during boot to force format. |
| "Out of memory" on ESP8266 | Use `-Os` optimize, reduce LittleFS to 512 KB. |
| Pico W not found in Boards Manager | Check URL exactly matches; restart Arduino IDE. |
| WebCrypto errors in browser | The polyfill is auto-injected. If still fails, serve via HTTPS (mkcert) or use `file://` + device IP. |
| Can't connect to AP | Ensure you're not on a VPN; try phone hotspot. |
| Upload fails | Check USB cable (data, not charge-only); try lower upload speed. |

---

## Building the HTML Header (if you modify LocalVault.html)

The `src/localvault_html.h` is pre-generated. To regenerate:

```bash
cd esp-vault/scripts
python generate_html.py
```

This reads `../../LocalVault.html`, injects the WebCrypto polyfill, gzips it, and writes the C array header.