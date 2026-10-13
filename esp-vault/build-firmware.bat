@echo off
setlocal EnableDelayedExpansion
title ESPVault One-Click Firmware Generator
cd /d "%~dp0"

echo ============================================================
echo  ESPVault One-Click Firmware Generator
echo  %DATE% %TIME%
echo ============================================================
echo.

REM --- 1. Check Python ---
where python >nul 2>nul
if errorlevel 1 (
  echo [ERROR] python not found in PATH.
  pause
  exit /b 1
)

REM --- 2. Regenerate embedded HTML (P1 UI + endpoint fixes) ---
echo [1/3] Regenerating embedded HTML...
python scripts\generate_html.py
if errorlevel 1 (
  echo [ERROR] generate_html.py failed.
  pause
  exit /b 1
)
echo.

REM --- 3. Build all 3 boards ---
echo [2/3] Building ESP32 + ESP32-C3 + D1 Mini (this takes ~1 min)...
python -m platformio run -e esp32 -e esp32c3 -e d1_mini
if errorlevel 1 (
  echo [ERROR] PlatformIO build failed.
  pause
  exit /b 1
)
echo.

REM --- 4. Copy to web-flash/ (production + dated archive) ---
echo [3/3] Copying firmware to web-flash\ ...
python scripts\copy_firmware.py
if errorlevel 1 (
  echo [ERROR] copy_firmware.py failed.
  pause
  exit /b 1
)
echo.

echo ============================================================
echo  DONE - web-flash\ ready:
dir /-C web-flash\firmware_*.bin web-flash\bootloader_*.bin web-flash\partitions_*.bin | findstr /V "dated Archived"
echo.
echo  Flash via browser: web-flash\index.html  (serve over HTTPS)
echo  Flash ESP32 via USB : python -m esptool --chip esp32 --port COM10 --baud 460800 write_flash -z --flash-mode dio --flash-freq 40m --flash-size 4MB 0x1000 web-flash\bootloader_esp32.bin 0x8000 web-flash\partitions_esp32.bin 0x10000 web-flash\firmware_esp32.bin
echo  Flash D1 Mini via USB: python -m esptool --port COM12 write_flash 0x0 web-flash\firmware_d1_mini.bin
echo ============================================================
pause
