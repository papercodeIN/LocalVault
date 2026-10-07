# PlatformIO Setup Guide

## Prerequisites

- **Python 3.8+** (PlatformIO requires it)
- **VS Code** with **PlatformIO IDE** extension (recommended) or PlatformIO Core CLI

```bash
# Install PlatformIO Core (if not using VS Code extension)
pip install platformio

# Or on Windows:
pip install platformio --user
# Then add %APPDATA%\Python\Python3x\Scripts to PATH
```

---

## 1. Open Project

```bash
cd esp-vault
# In VS Code: File → Open Folder → select esp-vault
# Or CLI:
pio project init --ide vscode  # generates .vscode/ for IntelliSense
```

---

## 2. Install Platform Dependencies

**Windows: Enable long paths first** (required for Pico core):
```cmd
git config --system core.longpaths true
# Then enable "Enable NTFS/Win32 long paths" in gpedit.msc → reboot
```

PlatformIO will auto-install:
- `espressif32` platform (for ESP32-C3)
- `espressif8266` platform (for D1 Mini)
- `maxgerhardt/platform-raspberrypi` (for Pico W)

First build downloads toolchains (~200 MB total).

---

## 3. Build

```bash
# Build all environments
pio run

# Build specific board
pio run -e esp32c3
pio run -e d1_mini
pio run -e pico_w
```

---

## 4. Upload (Serial)

```bash
# Upload to connected board
pio run -e esp32c3 -t upload
pio run -e d1_mini -t upload
pio run -e pico_w -t upload

# Auto-detect port
pio run -e esp32c3 -t upload --upload-port /dev/ttyUSB0
```

---

## 5. Filesystem (LittleFS)

The HTML is embedded in flash (PROGMEM), so **no filesystem upload needed** for the web UI.

But if you want to store vault data on LittleFS (it does by default), the partition is created on first boot.

To upload a pre-built filesystem image:
```bash
pio run -e esp32c3 -t buildfs    # Creates .pio/build/esp32c3/littlefs.bin
pio run -e esp32c3 -t uploadfs   # Uploads it
```

---

## 6. Monitor Serial

```bash
pio device monitor -e esp32c3
pio device monitor -e d1_mini -b 115200
```

---

## 7. Custom Board (if yours isn't listed)

Add to `platformio.ini` under the appropriate env:

```ini
; Example: Custom ESP32-C3 board with 8 MB flash
[env:my_esp32c3]
extends = env:esp32c3
board = esp32c3-devkitm-1
board_build.flash_size = 8MB
board_build.partitions = default_8MB.csv
board_build.filesystem_size = 3m
```

---

## 8. Build Flags Explained

| Flag | Purpose |
|------|---------|
| `-DLOCALVAULT_VERSION=1.0.0` | Firmware version in /health |
| `-O2 -flto` | Optimize for size + speed |
| `board_build.filesystem_size` | LittleFS partition size |
| `board_build.core = earlephilhower` | Select Arduino-Pico core (Pico W) |

---

## 9. Troubleshooting

| Error | Fix |
|-------|-----|
| `Filename too long` (Windows) | Enable long paths + reboot (see step 2) |
| `board 'rpipicow' not found` | Update platform: `pio pkg update -g -p https://github.com/maxgerhardt/platform-raspberrypi.git` |
| `LittleFS mount failed` | First boot auto-formats. If persists: `pio run -t erase` |
| `Out of memory` (ESP8266) | Reduce `board_build.filesystem_size = 0.5m` |
| WebCrypto errors | Polyfill is auto-injected. Use HTTPS for production. |

---

## 10. Regenerate HTML Header

If you modify `../../LocalVault.html`:

```bash
cd esp-vault/scripts
python generate_html.py
pio run  # rebuilds with new header
```