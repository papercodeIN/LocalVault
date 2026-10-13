# ESPVault Security Documentation

**Last Updated:** October 13, 2026

## Overview

ESPVault is a hardware password vault designed for **local-first, offline backup storage** of your critical passwords. This document explains the security architecture, threat model, vault modes, and best practices.

---

## Target Audience

ESPVault is designed for:

- **Post-breach paranoid users** who want offline backups after experiencing a password manager breach
- **Self-hosters and privacy activists** who don't trust cloud storage
- **Crypto holders** protecting high-value wallet seeds and private keys
- **Anyone wanting a $29-49 hardware backup** as insurance against password manager failures

---

## Threat Model

### What ESPVault Protects Against

✅ **Cloud service breaches** - Your passwords never touch the internet  
✅ **Password manager failures** - Hardware backup survives service shutdowns  
✅ **Device loss** - Encrypted backups can be restored on new hardware  
✅ **Physical device failure** - Export backups regularly to survive hardware death  
✅ **Local network snooping** (Encrypted Mode only) - Zero-knowledge encryption protects data in transit  

### What ESPVault Does NOT Protect Against

❌ **Physical device theft without PIN/password** - Device vault mode stores passwords in plaintext on the device  
❌ **Network attacks on HTTP traffic** (Device Mode) - HTTP traffic is not encrypted between browser and device  
❌ **Malware on your computer** - If your computer is compromised, attackers can steal passwords when you view them  
❌ **Brute-force attacks** (weak PINs/passwords) - Use strong passwords and PINs (8+ characters recommended)  
❌ **Advanced hardware attacks** - No secure element, encrypted storage, or anti-tamper protection  

---

## Two Vault Modes

ESPVault offers two security models. Choose based on your threat model:

### 1. Device Vault Mode (Default)

**How it works:**
- Passwords stored in **plaintext** on device flash storage
- Protected by 4-6 digit **PIN** (stored on device)
- PIN rate-limiting prevents brute-force attacks
- HTTP traffic between browser and device is **NOT encrypted**

**Best for:**
- Home network use only
- Trusted local network environments
- Users prioritizing convenience over maximum security
- Quick backup vault for low-sensitivity passwords

**Security considerations:**
- ⚠️ **Anyone with physical access to the device can extract passwords** if they have PIN
- ⚠️ **HTTP traffic can be intercepted** on local network (passwords visible in transit)
- ⚠️ PIN rate-limiting resets on device reboot
- ✅ Session tokens prevent repeated PIN entry
- ✅ Simple, fast, works on all ESP boards (ESP8266, ESP32, ESP32-C3)

**Capacity:**
- **ESP8266 (D1 Mini):** ~40-80 password items (24 KB limit)
- **ESP32/ESP32-C3:** ~300+ password items (96 KB limit)

---

### 2. Encrypted Vault Mode (Zero-Knowledge)

**How it works:**
- Passwords encrypted with **AES-256-GCM** using master password-derived key
- Master password **never stored** on device
- **Zero-knowledge architecture:** device only stores encrypted blob
- Client-side encryption/decryption in browser (Web Crypto API)
- HTTP traffic contains only encrypted data (useless without master password)

**Best for:**
- Maximum security requirements
- Users storing high-value credentials (crypto wallets, financial accounts)
- Untrusted network environments
- Users comfortable with master password management

**Security considerations:**
- ✅ **Physical device theft is safe** - encrypted blob is useless without master password
- ✅ **Network interception is safe** - only encrypted data transmitted
- ✅ **True zero-knowledge** - device never sees plaintext passwords
- ⚠️ **Master password forgotten = data permanently lost** (no recovery)
- ⚠️ Requires strong master password (12+ characters recommended)
- ⚠️ Slightly slower than device mode (encryption overhead in browser)

**Capacity:**
- Same as Device Mode (encryption adds ~10% overhead)

---

## HTTP vs HTTPS

### Current Status: HTTP Only

ESPVault currently uses **HTTP** for all web traffic. Here's why:

**Technical constraints:**
- HTTPS requires **~40 KB additional memory** for TLS/SSL stack
- ESP8266 (D1 Mini) only has **~80 KB free RAM** - HTTPS would consume 50% of available memory
- ESP32 has sufficient memory but requires self-signed certificates (browser warnings)

**Security impact:**
- **Device Mode:** HTTP traffic exposes passwords in transit on local network
- **Encrypted Mode:** HTTP traffic only contains encrypted blobs (safe even if intercepted)

### Recommendations

**For Device Mode users:**
- ✅ Only use on trusted home networks
- ✅ Never use on public WiFi or untrusted networks
- ✅ Consider switching to Encrypted Mode if network security is a concern

**For Encrypted Mode users:**
- ✅ HTTP is acceptable - encrypted data protects against network interception
- ✅ Master password never transmitted (used only for local encryption/decryption)

### Future HTTPS Support (Planned)

We're exploring HTTPS support for ESP32 boards:
- Self-signed certificates (one-time browser warning on first connect)
- Let's Encrypt via DNS challenge (requires internet connectivity)
- **Not feasible for ESP8266** due to memory constraints

---

## Backup and Recovery Strategy

### Why Backups Are Critical

⚠️ **Device failure = permanent data loss without backups**

ESPVault is a hardware device with finite lifespan:
- Flash memory has limited write cycles (~100,000 writes)
- Hardware can fail (power surge, physical damage, manufacturing defect)
- Lost/stolen devices cannot be recovered

### Backup Best Practices

✅ **Export backups regularly** (weekly recommended)
- Device Mode: `/api/backup/export` returns plaintext JSON (requires PIN)
- Encrypted Mode: `/api/backup/export` returns encrypted JSON (no auth required)

✅ **Store backups securely**
- Device Mode backups: **PLAINTEXT** - store in encrypted archive (7-Zip AES-256, VeraCrypt)
- Encrypted Mode backups: Already encrypted - can store anywhere (still recommend encrypted storage)

✅ **Test restore process**
- Regularly verify backups can be restored via `/api/backup/import`
- Keep backups on multiple storage locations (USB drive, encrypted cloud storage)

✅ **Backup before firmware updates**
- Export vault before flashing new firmware
- Firmware updates preserve vault data, but export is insurance

### Backup File Format

**Device Mode Backup:**
```json
{
  "type": "espvault-backup",
  "version": "1.0",
  "mode": "device",
  "exportedAt": 12345678,
  "device": "localvault",
  "vault": { /* plaintext password array */ },
  "meta": { /* metadata */ }
}
```

**Encrypted Mode Backup:**
```json
{
  "type": "espvault-backup",
  "version": "1.0",
  "mode": "encrypted",
  "exportedAt": 12345678,
  "device": "localvault",
  "blob": "encrypted-data...",
  "meta": "encrypted-metadata...",
  "rec": "recovery-data..."
}
```

---

## Storage Capacity and Monitoring

### Capacity Limits

ESPVault has finite storage capacity based on board type:

| Board | Free RAM | Max Vault Size | Est. Password Items |
|-------|----------|----------------|---------------------|
| ESP8266 (D1 Mini) | ~80 KB | 24 KB | 40-80 items |
| ESP32 | ~320 KB | 96 KB | 300+ items |
| ESP32-C3 | ~320 KB | 96 KB | 300+ items |

### Monitoring Capacity

Check `/health` endpoint for capacity metrics:

```json
{
  "vaultSize": 12345,
  "maxVaultSize": 98304,
  "vaultUsagePercent": 12,
  "estimatedItems": 20,
  "maxEstimatedItems": 163
}
```

⚠️ **Export backup when usage exceeds 80%** - prevents data loss from storage exhaustion

---

## WiFi Security

### WiFi Auto-Fallback

ESPVault automatically falls back to AP mode after **10 failed WiFi reconnection attempts** (5 minutes):
- Allows reconfiguration without physical access
- AP mode runs alongside STA mode (AP+STA)
- AP shuts down automatically when WiFi reconnects

### WiFi Best Practices

✅ **Use WPA2/WPA3** for home network  
✅ **Strong WiFi password** (16+ characters)  
✅ **Disable WPS** on router  
❌ **Never use public WiFi** with Device Mode  
❌ **Avoid open/unencrypted networks**  

### mDNS Support

- ESP32/ESP8266: `http://localvault.local` (mDNS enabled)
- ESP32-C3: IP address only (mDNS issues on some boards)
- Pico W: Not supported

---

## Authentication and Sessions

### Device Mode PIN

- **4-6 digit PIN** (configurable)
- Stored as SHA-256 hash in `/config.json`
- Rate-limiting: 3 attempts, then 60-second lockout
- **Rate limit resets on device reboot** (no persistent counter)

### Session Tokens

- Issued after successful PIN authentication
- Stored in browser cookies
- Valid until logout or device reboot
- Prevents repeated PIN entry during session

### Best Practices

✅ Use **6-digit PIN minimum** (1,000,000 combinations)  
✅ **Logout when done** to invalidate session  
❌ Don't reuse PIN from other devices  
❌ Don't write PIN on device label  

---

## Firmware Updates

### Update Safety

✅ Vault data persists across firmware updates (stored in LittleFS partition)  
✅ Export backup before updating (insurance against corruption)  
❌ Factory reset **erases all vault data** (always backup first)  

### Update Process

1. Export vault backup via `/api/backup/export`
2. Flash new firmware via web flasher or `platformio run -t upload`
3. Device reboots with new firmware
4. Verify vault data intact via `/health`
5. Test vault access
6. Store backup securely

---

## Security Recommendations

### General Best Practices

✅ **Export backups weekly** - device failure is not if, but when  
✅ **Use Encrypted Mode for sensitive data** - crypto wallets, financial accounts  
✅ **Use Device Mode for convenience** - home network, low-sensitivity passwords  
✅ **Strong master password** (12+ characters, mixed case, numbers, symbols)  
✅ **Monitor capacity** - export when >80% full  
✅ **Secure backup storage** - encrypted USB drive, VeraCrypt container  

### What NOT to Do

❌ **Don't store only copy of passwords in Device Mode** - plaintext vulnerable to theft  
❌ **Don't forget master password** (Encrypted Mode) - no recovery possible  
❌ **Don't skip backups** - hardware failure = permanent data loss  
❌ **Don't use on untrusted networks** (Device Mode) - HTTP exposes passwords  
❌ **Don't ignore capacity warnings** - storage exhaustion corrupts vault  

---

## Compliance and Limitations

### Not a Replacement for Enterprise Solutions

ESPVault is designed for **personal use backup storage**, not enterprise password management:

❌ No team sharing features  
❌ No audit logs or compliance reporting  
❌ No integration with SSO/SAML/LDAP  
❌ No hardware security module (HSM)  
❌ No FIPS 140-2 certification  
❌ No secure element or trusted execution environment  

### Use Cases

✅ Personal backup vault for Bitwarden/1Password/LastPass  
✅ Offline storage for crypto wallet seeds  
✅ Emergency password access when cloud services down  
✅ Privacy-focused users avoiding cloud storage  
✅ Self-hosters wanting hardware backup insurance  

---

## Incident Response

### Device Lost or Stolen

**Device Mode:**
1. Assume all passwords compromised
2. Change all passwords stored in device vault immediately
3. Review account activity for unauthorized access
4. Enable 2FA on all accounts if not already active

**Encrypted Mode:**
1. Device theft is safe if master password is strong (12+ characters)
2. Monitor accounts for suspicious activity
3. Consider changing critical passwords as precaution
4. Master password remains secret (never stored on device)

### Backup File Compromised

**Device Mode:**
- Backup file contains **plaintext passwords** - treat as full breach
- Change all passwords immediately
- Review how backup was compromised (weak encryption, lost USB drive)

**Encrypted Mode:**
- Backup file is encrypted - attacker needs master password
- If master password is strong (12+ characters), data remains safe
- Consider changing passwords if master password may be weak

### WiFi Network Compromised

**Device Mode:**
- HTTP traffic may have been intercepted
- Change all passwords accessed during compromised period

**Encrypted Mode:**
- Only encrypted blobs transmitted - data remains safe
- Master password never transmitted over network

---

## Technical Details

### Encryption (Encrypted Mode)

- **Algorithm:** AES-256-GCM (Galois/Counter Mode)
- **Key Derivation:** PBKDF2-SHA256 (100,000 iterations)
- **Salt:** Random 16-byte salt per vault
- **IV:** Random 12-byte IV per encryption operation
- **Authentication:** GCM provides authenticated encryption (prevents tampering)

### Storage

- **File System:** LittleFS (wear-leveling, power-loss protection)
- **Atomic Writes:** Temp file + rename (no partial writes)
- **Vault Files:**
  - Device Mode: `/vault.plain.json` (plaintext)
  - Encrypted Mode: `/vault.store.json` (encrypted blob)
  - Config: `/config.json` (device settings, PIN hash)

### Memory Safety

- **ESP8266:** ~80 KB free RAM, ~24 KB vault limit (30% of RAM)
- **ESP32:** ~320 KB free RAM, ~96 KB vault limit (30% of RAM)
- Limits prevent out-of-memory crashes during JSON parsing

---

## Frequently Asked Questions

### Q: Is HTTP secure enough?

**A:** Depends on vault mode:
- **Device Mode:** No - HTTP exposes passwords. Use trusted networks only.
- **Encrypted Mode:** Yes - only encrypted data transmitted. Master password never sent.

### Q: Can I use ESPVault as my primary password manager?

**A:** No. ESPVault is designed as a **backup vault**, not a primary password manager. Use Bitwarden, 1Password, or similar for daily use. ESPVault is your insurance policy when those services fail.

### Q: What happens if I forget my master password?

**A:** **Data is permanently lost.** Zero-knowledge encryption means no recovery mechanism. Write down master password and store in secure location (safe, bank vault).

### Q: How often should I export backups?

**A:** **Weekly recommended**, or immediately after adding critical passwords. Device failure is rare but catastrophic without backups.

### Q: Can I upgrade from Device Mode to Encrypted Mode?

**A:** Not directly. You must:
1. Export Device Mode backup
2. Switch to Encrypted Mode in config
3. Manually re-import passwords (backup format incompatible between modes)

### Q: Is my data safe if someone steals my ESPVault device?

**A:**
- **Device Mode:** No - passwords stored in plaintext (PIN provides minimal protection)
- **Encrypted Mode:** Yes - encrypted blob useless without master password

### Q: Why no HTTPS on ESP8266?

**A:** Memory constraints. HTTPS requires ~40 KB for TLS/SSL stack. ESP8266 only has ~80 KB free RAM. Adding HTTPS would consume 50% of available memory and destabilize the device.

---

## Conclusion

ESPVault provides **local-first, offline password backup** with two security models:

- **Device Mode:** Convenient, fast, plaintext storage (trusted networks only)
- **Encrypted Mode:** Maximum security, zero-knowledge encryption (safe anywhere)

**Key Takeaways:**

✅ Export backups regularly (device failure is inevitable)  
✅ Use Encrypted Mode for sensitive data  
✅ Monitor capacity (export when >80% full)  
✅ Understand threat model (not a replacement for enterprise solutions)  
✅ Secure backup storage (encrypted USB, VeraCrypt container)  

**ESPVault is your $29-49 insurance policy against password manager failures.**

---

## Contact and Support

- **GitHub:** https://github.com/papercodeIN/LocalVault
- **Issues:** Report security issues via GitHub Issues
- **Documentation:** See README.md for setup and usage instructions

---

**Disclaimer:** ESPVault is provided as-is without warranty. Users are responsible for backup strategy, master password management, and understanding security tradeoffs. Always maintain multiple backups of critical passwords.
