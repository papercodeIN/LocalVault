# 🖥️ LocalVault Server — self-hosted, same UI

> The exact same app, auto-started on login, with an encrypted copy on your disk.

Zero dependencies — just **Node 18+**. The server only serves the file and stores ciphertext; all encryption happens in your browser.

---

## 🏁 Start in 3 steps

On Windows, double-click `start-windows.bat` to start the server, then open **http://127.0.0.1:18765** in each browser you want to use. To stop it, run `stop-windows.bat`. Keep the server running while using LocalVault.

On macOS or Linux (or from a terminal):

```sh
cd selfhosted
node server.js
```

Open 👉 **http://127.0.0.1:18765** · Health: 👉 **http://127.0.0.1:18765/health**

```sh
node setup.js            # menu: 1 = install auto-start, 2 = remove it
node setup.js install    # non-interactive install
node setup.js uninstall  # non-interactive remove
```

| OS | Install does… |
|---|---|
| 🪟 Windows | Hidden launcher in your Startup folder (no admin). `start-windows.bat` runs it now, `stop-windows.bat` stops it. |
| 🐧 Linux | `systemd` user service, enabled immediately. |
| 🍎 macOS | LaunchAgent, loaded immediately. |

Change port (host must stay loopback):

```sh
LOCALVAULT_PORT=8080 node server.js
```

---

## 🌐 Choose how to run LocalVault

```text
Open LocalVault.html directly
  └─ Browser A → its own local vault
  └─ Browser B → a separate local vault (no sharing)

Run selfhosted/server.js, then open http://127.0.0.1:18765
  └─ Browser A ─┐
  └─ Browser B ─┴→ same encrypted server vault on this computer
                    Use one browser at a time; open tabs do not live-sync.
```

Direct-file mode keeps data separately in each browser. Self-hosted mode shares one vault through the running local server; each browser must use the server address, not open the HTML file directly. Move an existing file-mode vault once using the encrypted backup steps below.

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

## 🛡️ Security rules

1. **Binds `127.0.0.1` only** — unreachable from the network. Keep it that way.
2. **Never port-forward to the internet.** LAN sharing is a conscious choice — ask first.
3. Same recovery-key flow as file mode — save the key shown at creation.

---

## ⚠️ Honest limitations

Server-only notes — the basics (no reset email, no auto-lock, backups are on you) are in the [main README](../README.md).

- **Works only while running.** No program, no page. Run `node setup.js` once to auto-start on login.
- **No login on it.** Keep it on this PC only, never open it to the network.
- **Browsers share the same server vault, not live synchronization.** Open the server URL (`http://127.0.0.1:18765`) in each browser on the same computer. They load and save the same encrypted disk store, but use one browser at a time: the last save wins, and already-open tabs do not refresh. The server must be running.
- **Snapshots stay in the browser.** The vault is saved to disk, the weekly snapshots are not.

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
