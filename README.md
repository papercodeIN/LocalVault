# LocalVault — Offline Password Manager

Two ways to run the same app: open the single HTML file directly, or run the self-hosted server (auto-starts on login, keeps an encrypted copy on disk). No accounts, no cloud, 100% offline either way.

## Quick Start
1. Double-click `LocalVault.html` (Chrome / Edge / Firefox / Safari).
2. Create vault → choose a master password (min 12 chars) → remember it.
3. **Save the recovery key** that is shown after creation (copy, print, or download .txt).
4. Unlock with that password each time you reopen.

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

## 🔑 Recovery Key (Forgot Master Password?)
When you create the vault, a **recovery key** is shown once. It is your only way to reset a forgotten master password **without losing your data**.

- **Save it offline** — print it, copy to a USB stick, or download the .txt. Store it somewhere safe but not tied to the browser.
- **To use it:** On the lock screen click **“Use recovery key”** → enter the key → set a new master password. Your existing entries are preserved and re-encrypted.
- **Anyone with the key can unlock your vault** — treat it like a password.
- **Changing your master password or security level invalidates the old key.** A new one must be generated (Settings → Recovery key → Generate).
- **If you lost the key AND forget the password:** the vault cannot be recovered. You must delete and start over (or restore from an encrypted backup file).

## 🖥️ Self-hosted Server (recommended for daily use)

Same UI, served at `http://127.0.0.1:18765` with auto-start on login. Additionally saves the **already-encrypted** vault to `vault.store.json` on your disk after every change — so clearing browser data no longer loses the vault. Needs only Node 18+, zero dependencies.

```sh
cd selfhosted
node setup.js     # 1) Install auto-start  2) Uninstall
node server.js    # run now
```

First run needs a one-time migrate (new browser origin): export from the file version → restore into the server address. Full steps in [`selfhosted/README.md`](selfhosted/README.md).

## Limitations — Read This
- **No sync:** Vault is tied to `origin + browser profile` (localStorage). 2 devices/browsers = 2 separate vaults. Merge via export/import.
- **No recovery without the key:** Recovery only works if you saved the recovery key. No email / phone reset exists (nothing to contact — it's fully offline).
- **File mode:** Clearing browser data (cookies/cache) erases the vault AND the stored recovery key. Keep an encrypted backup + your recovery key. (Server mode additionally keeps an encrypted copy on disk.)
- **Incognito/private** wipes on close.
- **`file://` vs `http://127.0.0.1:18765`** are different origins — vault does not carry over (migrate once via export/restore).
- **No cloud:** Nothing ever contacts the internet. You are responsible for backups.

## Security
- Browser WebCrypto: PBKDF2-SHA256 → AES-GCM 256. Encryption iterations selectable as Security level (Standard / High / Maximum) in Settings.
- All crypto happens locally. Nothing is sent anywhere (except optional breach check — sends only 5-char hash prefix).

## Files
- `LocalVault.html` — the app (everything).
- `selfhosted/` — loopback server + encrypted disk store + one-script setup. See [`selfhosted/README.md`](selfhosted/README.md).
- `README.md` — this file.

## Tips
- Bookmark the file.
- Write master password on paper, store safely.
- Download encrypted backup monthly to USB/drive.
