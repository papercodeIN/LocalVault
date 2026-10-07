# 🖥️ LocalVault Server — self-hosted, same UI

> The exact same app, auto-started on login, with an encrypted copy on your disk.

Zero dependencies — just **Node 18+**. The server only serves the file and stores ciphertext; all encryption happens in your browser.

---

## 🏁 Start in 3 steps

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

## 🔄 First run — one-time move (2 min)

Your browser treats the server address as a *new home*, so the old vault doesn't follow automatically:

1. Open `LocalVault.html` → Export JSON (or Settings → Download backup).
2. Open `http://127.0.0.1:18765` → create vault with the **same** master password → Settings → Restore from file.
3. ✅ Verify entries — from now on, live here. Every change is mirrored encrypted to disk.

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

Applies to server mode only — the general ones (one browser profile per origin, no reset emails, weekly backup needs an unlock every 7 days, no idle auto-lock, plaintext CSV/Excel exports) are in the [main README](../README.md).

- **Node has to be up.** No process → no page. Install autostart with `node setup.js`, or keep `start-windows.bat` handy.
- **No auth on `/api/store`.** It validates shape, not identity — any local process can read or overwrite the ciphertext. Loopback-only binding *is* the security boundary; don't move the host or port-forward it.
- **Last write wins, no merge.** A save PUTs the entire vault (`server.js:97`). Two unlocked browsers will silently overwrite each other — use one at a time and reopen the tab to pull the disk copy.
- **Sync errors are swallowed.** A stopped server means saves stay in `localStorage` with no on-screen warning; the disk mirror lags until a save succeeds again.
- **Only `blob`, `meta` and `rec` are mirrored.** The 4 weekly snapshots (`localvault.auto.v1`) stay browser-side — clear site data and they're gone even though the vault reloads from `vault.store.json`.
- **Moving between `file://` and this server is manual.** New origin = fresh storage — see **First run** above.
- **25 MB body cap**, and `vault.store.json` is git-ignored — back it up yourself.

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
