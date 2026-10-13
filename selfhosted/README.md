# 🖥️ LocalVault Server — self-hosted, same UI

> The exact same app, auto-started on login, with an encrypted copy on your disk.

Zero dependencies — just **Node 18+**. The server only serves the file and stores ciphertext; all encryption happens in your browser.

---

## 🏁 Start here

### Windows

1. Install Node.js 18 or newer if it is not already installed.
2. Double-click `start-windows.bat` inside `selfhosted/`. It starts the server hidden and opens LocalVault in your default browser.
3. In other browsers on the same computer, open **http://127.0.0.1:18765**. Keep the server running; use one browser at a time.
4. To stop it, double-click `stop-windows.bat`.

### macOS / Linux

```sh
cd selfhosted
node server.js
```

Then open **http://127.0.0.1:18765** in a browser. Health check: **http://127.0.0.1:18765/health**. Keep the terminal/server running while using LocalVault.

### Optional: start automatically at login

```sh
node setup.js            # interactive install/uninstall menu
node setup.js install    # install auto-start
node setup.js uninstall  # remove auto-start
```

To change the port, set `LOCALVAULT_PORT` before starting the server (for example, `LOCALVAULT_PORT=8080 node server.js`). The host remains bound to `127.0.0.1`.

| OS | Auto-start behavior |
|---|---|
| Windows | Hidden launcher in your Startup folder (no admin). `start-windows.bat` runs it now; `stop-windows.bat` stops it. |
| Linux | `systemd` user service, enabled immediately. |
| macOS | LaunchAgent, loaded immediately. |

---

## 🌐 Choose how to run LocalVault

```text
Open LocalVault.html directly
  ├─ Browser A → its own local vault
  └─ Browser B → a separate local vault (no sharing)

Start self-hosted server → open http://127.0.0.1:18765
  ├─ Browser A ─┐
  └─ Browser B ─┴→ one encrypted vault mirrored to this computer's disk
                    Sequential use only; no live sync between open tabs.
```

| Mode | Advantages | Trade-offs |
|---|---|---|
| Open `LocalVault.html` | Simplest; works offline; no Node/server to run | Each browser has a separate vault; browser data clearing can erase that browser's copy; sharing requires manual export/import. |
| Self-hosted | Browsers on this computer use the same vault; encrypted disk mirror survives browser storage clearing; optional start-on-login | Requires Node and a running server; browsers do not live-sync, so use one at a time; server is loopback-only and not reachable from other devices by default. |

For shared use, every browser must open the server address—not the HTML file. Transfer an existing file-mode vault once using the encrypted-backup steps below. The server is local storage, not cloud sync or multi-user access.
---

## 🔄 First run — one-time move (2 min)

Your browser treats the server address as a *new home*, so the old vault doesn't follow automatically:

1. Open `LocalVault.html`, unlock your vault, then go to Settings → **Download backup**. This creates an encrypted backup file.
2. Open `http://127.0.0.1:18765` → create a vault with the **same** master password → Settings → **Restore from file**, then select the backup. Confirm that you want to replace the new, empty vault.
3. Verify your entries. From now on, the vault is mirrored encrypted to `vault.store.json` while the server runs.

---

## 💾 What lands on disk?

`vault.store.json` — your vault blob + settings + recovery key, **all still encrypted** (same AES-GCM ciphertext the browser holds). The server process can read the file but can never read your passwords.

- Loaded on boot, newer copy wins — so a fresh browser pulls your vault straight from disk.
- Wipe/delete also wipes the disk copy (no zombie restores).
- Git-ignored, never committed. Back it up like anything precious.

---

## 🔌 API (loopback only)

| Endpoint | Method | Does… |
|---|---|---|
| `/` | GET | The app |
| `/health` | GET | `{"ok":true,…}` liveness |
| `/api/store` | GET / PUT | Read / write the encrypted store (validates shape, 25 MB cap, atomic write) |

---

## 🛡️ Important security and data notes

- The server binds to `127.0.0.1` only; it is for this computer and is not reachable from other devices by default. Do not port-forward it or expose it to the internet.
- The server stores encrypted data and cannot read vault contents, but anyone with access to your computer can copy or delete the store. Protect the computer and keep separate backups.
- Save the recovery key shown at vault creation. There is no password reset; changing the master password or security level invalidates the previous recovery key. See the [main README](../README.md) for other app-wide limitations.
- `vault.store.json` is the server's persistent encrypted mirror, not a backup strategy. Keep an encrypted backup somewhere separate from this computer.
- Weekly snapshots are stored in browser storage, not in `vault.store.json`; they do not follow you to another browser.
- Exports such as CSV or plain JSON contain readable passwords. Store them securely and delete them when no longer needed; use encrypted backups for safekeeping.
- When moving data from direct-file mode, export an encrypted backup first and verify the restored entries before relying on the server copy.

---

## ⚠️ Self-hosted limitations

- The server must be running to open the app through its URL. Use the optional login auto-start if desired.
- There is no separate server login. Anyone using the computer can access the app page; the vault still requires its master password to unlock.
- Browsers on this computer load/save the same server vault, but there is no live synchronization. Use one browser at a time: the newest save wins, and already-open tabs do not refresh automatically.
- The server is local-only by default. It is not cloud sync, LAN sharing, or multi-user access.
- Browser snapshots remain in that browser and are not mirrored to the server disk.
- App-wide limitations—including no password reset, no auto-lock, and the need to manage backups—are covered in the [main README](../README.md).

---

## ❓ FAQ

<details>
<summary><b>Browser storage got cleared — did I lose everything?</b></summary>

No. On next boot the app reloads the encrypted vault from `vault.store.json`. Unlock with your master password and carry on.
</details>

<details>
<summary><b>Can two browsers share one server?</b></summary>

Yes, sequentially — the second browser pulls from disk on first boot. Don't edit in two browsers at once: newest save wins, no merge.
</details>

<details>
<summary><b>Raspberry Pi?</b></summary>

Perfect fit (pure Node, ARM-friendly). Install Node 20+ via NodeSource — Pi OS apt is usually too old — then `node setup.js`. Use the Pi's own browser, or set up LAN access deliberately.
</details>

---

## 📂 Files

- `server.js` — static loopback server + encrypted disk store (no deps)
- `setup.js` — install / uninstall login autostart (auto-detects OS)
- `start-windows.bat` / `stop-windows.bat` — quick start / stop on Windows
- `vault.store.json` — created at runtime, git-ignored, ciphertext only
- UI lives in `../LocalVault.html` — single source, no copy to drift
