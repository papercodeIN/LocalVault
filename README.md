# LocalVault

An offline password manager that encrypts your vault on your device. No account, cloud sync, or subscription.

## Quick start

> ## 🔴 **OPTION A — OPEN THE FILE (SIMPLEST)**

1. Open `LocalVault.html` in your browser.
2. Create a vault with a strong master password.
3. Save the recovery key somewhere safe. It is shown once and is needed to reset a forgotten password.

The vault is saved in that browser. Other browsers have separate vaults.

> ## 🔵 **OPTION B — USE THE LOCAL SERVER**

On Windows, open `selfhosted/start-windows.bat`. On macOS or Linux, run:

```sh
cd selfhosted
node server.js
```

Then open `http://127.0.0.1:18765`. Browsers using this address share the vault, with an encrypted copy stored on this computer. The server must be running. Use one browser at a time; open tabs do not sync changes live.

For automatic startup, run `node setup.js` from the `selfhosted` folder. To move an existing vault to the server, follow the [self-hosted guide](selfhosted/README.md).

## Your data and backups

- **File option:** Vault data is stored in the browser. Clearing that browser's saved data may remove it.
- **Server option:** The browser and the server's local disk store an encrypted copy of the vault. Weekly snapshots remain in each browser and are not included in the server's disk copy.
- **Keep a separate backup:** In Settings, use **Download backup** to save an encrypted backup file somewhere safe. The server copy is useful for recovery, but is not a separate backup.
- **Exports:** CSV, Excel, and plain JSON exports contain readable passwords. Keep them private and delete them when no longer needed.

Encryption and decryption happen in the browser; the self-hosted server stores encrypted vault data. The server listens on this computer only by default. Do not expose it to the internet.

## Features

- Logins, cards, identities, notes, and SSH keys
- Password and passphrase generator
- Folders, search, favourites, and recoverable bin
- Import and export, with duplicate detection
- Recovery key and per-item master-password re-prompt
- Password health report and optional breach check
- Dark mode and keyboard shortcuts (`Ctrl+K`, `Ctrl+N`, `Esc`)
- Weekly encrypted browser snapshots (keeps the newest four)

## If you forget your password

Use **Use recovery key** on the lock screen and set a new password. Changing your master password or security level invalidates the recovery key, so generate and save a new one in Settings.

Without the master password or recovery key, the vault cannot be unlocked. Restore an encrypted backup if you have one.

## More information

- [Self-hosted setup and usage](selfhosted/README.md)
- `LocalVault.html` — the standalone app
- `selfhosted/` — the optional local server and setup scripts
