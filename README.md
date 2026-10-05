# LocalVault — Offline Password Manager (Single File)

One HTML file. No install, no server, 100% offline. Your vault lives only in your browser.

## Quick Start
1. Double-click `LocalVault.html` (Chrome / Edge / Firefox / Safari).
2. Create vault → choose a master password (min 12 chars) → remember it.
3. Unlock with that password each time you reopen.

## How to Use
- **Add:** Top-right `+ Add` → choose Login / Card / Identity / Note.
- **Search:** Type in search box (also filters by folder, fav, bin).
- **Edit:** Click a row → detail panel → pencil icon.
- **Move to Bin:** Row menu `⋮` → Move to Bin. Or red trash icon on row.
- **Delete forever:** Open `Bin` in sidebar → trash icon → confirm.
- **Bulk:** Checkboxes on rows → `Move to Bin` bar appears.
- **Generator:** Sidebar → Generator (password / passphrase).
- **Health:** Sidebar → Health report (weak / reused / breached - optional online check).
- **Folders:** Sidebar → `+ New` under Folders.
- **Import/Export:** Sidebar → Import (CSV/JSON) / Export → JSON file / Copy JSON / CSV / Excel.

## Backup & Restore
- **Download backup:** Settings → Download backup (encrypted, needs master password to restore).
- **Restore:** Settings → Restore from file → pick backup JSON → enter master password.
- **Export (decrypted):** Sidebar Export → JSON/CSV/Excel or `Copy JSON` (copies to clipboard). Keep decrypted exports safe — delete after use.
- **Automatic:** Settings → Automatic weekly backup (on by default) keeps last 4 encrypted snapshots locally. Banner reminds you to download.

## Limitations — Read This
- **No sync:** Vault is tied to `origin + browser profile` (localStorage). 2 devices/browsers = 2 separate vaults. Merge via export/import.
- **No password reset:** Forgot master password = vault lost forever. No recovery.
- **Clearing browser data** (cookies/cache) erases vault. Keep an encrypted backup.
- **Incognito/private** wipes on close.
- **`file://` vs hosted `https://`** are different origins — vault does not carry over.
- **No cloud:** File never contacts internet. You are responsible for backups.

## Security
- Browser WebCrypto: PBKDF2-SHA256 → AES-GCM 256. Encryption iterations selectable as Security level (Standard / High / Maximum) in Settings.
- All crypto happens locally. Nothing is sent anywhere (except optional breach check — sends only 5-char hash prefix).

## Files
- `LocalVault.html` — the app (everything).
- `README.md` — this file.

## Tips
- Bookmark the file.
- Write master password on paper, store safely.
- Download encrypted backup monthly to USB/drive.
