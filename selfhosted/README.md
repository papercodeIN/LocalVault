# LocalVault Server (self-hosted, same UI)

Runs the exact same `LocalVault.html` UI over `http://127.0.0.1:18765`, auto-started on login. Zero dependencies — only Node 18+.

## Run now

```sh
cd selfhosted
node server.js
```

Open: **http://127.0.0.1:18765**

Health check: `http://127.0.0.1:18765/health` → `{"ok":true,...}`

Change port/host (host must stay loopback):

```sh
LOCALVAULT_PORT=8080 node server.js
```

## Setup (one script, all OS)

```sh
node setup.js
```

It detects your OS and asks:

- **1) Install** — auto-start server on login (Windows Startup / systemd user service / macOS LaunchAgent)
- **2) Uninstall** — remove auto-start

Non-interactive: `node setup.js install` or `node setup.js uninstall`.

Windows quick start: double-click `start-windows.bat` (runs hidden + opens browser). Stop with `stop-windows.bat`.

## Where passwords are stored

- **File mode** (`LocalVault.html` opened directly): vault stays in browser `localStorage`, as before.
- **Server mode** (this): the app additionally mirrors the **already-encrypted** vault blob (+ settings + recovery key) to `vault.store.json` on your local disk after every change, and loads it on boot (newer copy wins). The file holds only ciphertext — the server never sees your master password or entries. `vault.store.json` is git-ignored and never committed.

## First run — migrate your vault (once)

The browser treats `http://127.0.0.1:18765` as a new origin, so the vault stored under `file://` does not carry over automatically:

1. Open `LocalVault.html` (old) → Export JSON (or Settings → Download backup).
2. Open `http://127.0.0.1:18765` → create vault with the **same** master password → Settings → Restore from file.
3. Verify entries, then keep using the server address. From here the disk store keeps it safe even if browser storage is cleared.

## Security

- Binds `127.0.0.1` only — not reachable from the network. Never change host to `0.0.0.0` unless you know what you are doing.
- Encryption unchanged: PBKDF2-SHA256 → AES-GCM-256, all client-side.
- Same recovery-key flow as the file version (save the key shown at creation).

## Files

- `server.js` — static loopback server + encrypted disk store (no deps)
- `setup.js` — install/uninstall login autostart (auto-detects OS)
- `start-windows.bat` / `stop-windows.bat` — quick start/stop on Windows
- `vault.store.json` — encrypted vault on disk (created at runtime, git-ignored)
- UI lives in `../LocalVault.html` (single source, no copy to drift)
