#!/usr/bin/env python3
"""
Copy built firmware binaries to web-flash directory for ESP Web Tools.
Creates both production (non-dated) and archived (dated) versions.
Run after: pio run -e esp32 -e esp32c3 -e d1_mini -e pico_w
"""
import os
import shutil
import sys
from datetime import datetime

PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD_DIR = os.path.join(PROJECT_ROOT, '.pio', 'build')
WEB_FLASH_DIR = os.path.join(PROJECT_ROOT, 'web-flash')

# Generate date suffix (DD-MMM-YYYY)
DATE_SUFFIX = datetime.now().strftime("%d-%b-%Y")

FIRMWARE_MAP = {
    'esp32': [
        ('firmware.bin', 'firmware_esp32.bin'),
        ('bootloader.bin', 'bootloader_esp32.bin'),
        ('partitions.bin', 'partitions_esp32.bin'),
    ],
    'esp32c3': [('firmware.bin', 'firmware_esp32c3.bin')],
    'd1_mini': [('firmware.bin', 'firmware_d1_mini.bin')],
    'pico_w': [('firmware.uf2', 'firmware_pico_w.uf2')],
}

def main():
    os.makedirs(WEB_FLASH_DIR, exist_ok=True)
    
    print(f"Build dir: {BUILD_DIR}")
    print(f"Web flash dir: {WEB_FLASH_DIR}")
    print(f"Date suffix: {DATE_SUFFIX}\n")
    
    for env, file_list in FIRMWARE_MAP.items():
        for src_name, dst_name in file_list:
            src = os.path.join(BUILD_DIR, env, src_name)
            
            # Production filename (non-dated)
            dst_prod = os.path.join(WEB_FLASH_DIR, dst_name)
            
            # Archived filename (dated)
            base, ext = os.path.splitext(dst_name)
            dst_dated = os.path.join(WEB_FLASH_DIR, f"{base}_{DATE_SUFFIX}{ext}")
            
            if os.path.exists(src):
                # Copy production version
                shutil.copy2(src, dst_prod)
                size = os.path.getsize(dst_prod)
                print(f"[OK] {env}: {src_name} -> {dst_name} ({size:,} bytes)")
                
                # Copy dated archive version
                shutil.copy2(src, dst_dated)
                print(f"     --> {os.path.basename(dst_dated)} (archived)")
            else:
                print(f"[WARN] {env}: {src} not found (build first)")
    
    # Check manifest
    manifest_src = os.path.join(PROJECT_ROOT, 'web-flash', 'manifest.json')
    if os.path.exists(manifest_src):
        print("\n[OK] manifest.json already in place")
    
    print("\nDone! Firmware files ready:")
    print("  - Production: firmware_*.bin/uf2 (for manifest.json)")
    print(f"  - Archived: firmware_*_{DATE_SUFFIX}.bin/uf2 (for backup)")
    print("\nServe web-flash/ over HTTPS to use browser flashing.")
    print("Example: python -m http.server 8000 --directory web-flash")

if __name__ == '__main__':
    main()