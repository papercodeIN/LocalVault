#!/usr/bin/env python3
"""
Copy built firmware binaries to web-flash directory for ESP Web Tools.
Run after: pio run -e esp32c3 -e d1_mini -e pico_w
"""
import os
import shutil
import sys

PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD_DIR = os.path.join(PROJECT_ROOT, '.pio', 'build')
WEB_FLASH_DIR = os.path.join(PROJECT_ROOT, 'web-flash')

FIRMWARE_MAP = {
    'esp32c3': ('firmware.bin', 'firmware_esp32c3.bin'),
    'd1_mini': ('firmware.bin', 'firmware_d1_mini.bin'),
    'pico_w': ('firmware.uf2', 'firmware_pico_w.uf2'),
}

def main():
    os.makedirs(WEB_FLASH_DIR, exist_ok=True)
    
    print(f"Build dir: {BUILD_DIR}")
    print(f"Web flash dir: {WEB_FLASH_DIR}")
    
    for env, (src_name, dst_name) in FIRMWARE_MAP.items():
        src = os.path.join(BUILD_DIR, env, src_name)
        dst = os.path.join(WEB_FLASH_DIR, dst_name)
        
        if os.path.exists(src):
            shutil.copy2(src, dst)
            size = os.path.getsize(dst)
            print(f"[OK] {env}: {src_name} -> {dst_name} ({size:,} bytes)")
        else:
            print(f"[WARN] {env}: {src} not found (build first)")
    
    # Also copy manifest
    manifest_src = os.path.join(PROJECT_ROOT, 'web-flash', 'manifest.json')
    if os.path.exists(manifest_src):
        print("[OK] manifest.json already in place")
    
    print("\nDone! Serve web-flash/ over HTTPS to use browser flashing.")
    print("Example: python -m http.server 8000 --directory web-flash")

if __name__ == '__main__':
    main()