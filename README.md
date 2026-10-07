# 🔐 LocalVault — Offline Password Manager

> Your passwords. Your device. Nobody else's server.

No accounts. No cloud. No tracking. No fees. Just one app that keeps your secrets encrypted on hardware you own.

---

## 🚀 Quick start (60 seconds)

**Option A — double-click & go (simplest)**

1. Double-click `LocalVault.html` — opens in any browser, works fully offline.
2. Create your vault → pick a strong master password (min 12 chars).
3. 💾 **Save the recovery key** shown once — it's your only way back in if you forget the password.

**Option B — self-hosted server (recommended for daily use)**

```sh
cd selfhosted
node setup.js      # 1 = auto-start on login, 2 = remove it
node server.js     # or double-click start-windows.bat on Windows
```

Then open 👉 **http://127.0.0.1:18765**

> First run needs a one-time move: export from the file version → restore into the server address (new browser origin = fresh storage). Details in [`selfhosted/README.md`](selfhosted/README.md).

---

## ✨ What you get

| Feature | Details |
|---|---|
| 🔑 Logins, Cards, Identities, Notes, SSH keys | Bitwarden-style **+ Add** menu, per-type forms |
| 🎲 Generator | Passwords + passphrases, offline |
| 💓 Health report | Weak / reused / old passwords, optional breach check |
| 📁 Folders, ⭐ favourites, 🔍 instant search | Bitwarden-style sidebar |
| 🗑️ Bin with 30-day auto-expire | Recoverable deletes, bulk actions |
| 📥 Import / 📤 Export | CSV, JSON, Excel + encrypted backups, duplicate detection |
| 🔑 Recovery key | Forgotten master password? Reset it, keep your data |
| 🛡️ Re-prompt | Per-item "ask master password before showing" |
| 🌙 Dark mode, ⌨️ shortcuts (`Ctrl+K`, `Ctrl+N`, `Esc`) | |
| 📅 Weekly auto-backup | Encrypted snapshots (keeps newest 4), restore any of them |

---

## 💾 Where is my data, exactly?

| Mode | Vault lives in… | Cleared by… |
|---|---|---|
| 📄 File (`LocalVault.html`) | That browser's `localStorage` | Clearing browser cookies/cache |
| 🖥️ Server (`selfhosted/`) | Browser `localStorage` **+ encrypted mirror** `vault.store.json` on your disk | Browser clear only removes the copy — disk mirror reloads on next boot |

Everything stored is **ciphertext** (AES-256-GCM, keys from PBKDF2-SHA-256). The server never sees plaintext — it just hands the file to your browser.

---

## 🔑 Forgot your master password?

1. Lock screen → **Use recovery key** → paste the key you saved at creation.
2. Set a new password. Done — entries preserved, nothing lost.
3. No key *and* no password? The vault is unrecoverable by design — delete and start over (or restore an encrypted backup file).

> ⚠️ Changing your master password or security level **invalidates the old recovery key** — generate a fresh one in Settings → Recovery key.

---

## ⚠️ Honest limitations

**True for both modes**

- **One browser profile = one vault.** Chrome ≠ Edge ≠ Firefox. Use export/restore (or the server address, which pulls from disk) to move between them — one at a time, it's not live sync.
- **No password reset emails.** There is no server to email you. The recovery key *is* the reset — so anyone holding it can reset your password too.
- **Offline means you own backups.** Download the encrypted backup monthly to a USB stick.
- **Weekly backup = at first unlock after 7 days** (an hourly check catches long-open sessions). A sleeping PC can't snapshot itself, and only the newest 4 snapshots are kept.
- **No idle auto-lock.** The vault stays unlocked until you click **Lock** or close the tab.
- **Only the encrypted backup file is safe to park on disk** — CSV, Excel and plain JSON exports show passwords in the clear.

**📄 Direct HTML (`LocalVault.html`)**

- **Zero redundancy.** Everything lives in that browser's `localStorage` — clearing cookies/site data wipes the vault *and* all 4 snapshots. There is no second copy anywhere.
- **Small ceiling.** Storage is bounded by browser `localStorage` quota (a few MB), not the server's 25 MB cap.
- **No server, no sync.** Double-click works anywhere — and also means the only way to move a vault is manual export/restore.

**🖥️ Self-hosted (`selfhosted/`)**

- **The server must be running** or the page won't load at all. `node setup.js` installs login autostart; otherwise start it by hand.
- **No authentication on the API.** Any process on your machine can read or overwrite the ciphertext — which is why it binds `127.0.0.1` and must never be port-forwarded.
- **Last write wins, no merge.** Each save pushes the whole vault; editing in two browsers at once means the later save silently overwrites the earlier one. Reopen the tab to pull from disk.
- **Sync failures are silent.** If the server dies mid-session, changes just stay browser-local with no warning — the disk copy catches up on the next successful save.
- **Weekly snapshots are not mirrored.** Only the vault, settings and recovery key reach `vault.store.json`; clear browser storage and the vault reloads from disk, but the 4 snapshots are gone.

---

## ❓ FAQ

<details>
<summary><b>Can I use it on my phone?</b></summary>

Yes — open the file or server address in Chrome/Safari and bookmark it. Same rules: that phone browser holds its own vault copy.
</details>

<details>
<summary><b>Can I run it on a Raspberry Pi?</b></summary>

Yes — pure Node, runs great on ARM. Install Node 20+ via NodeSource (Pi OS apt is usually too old), then `node setup.js`. It serves loopback-only, so use the Pi's own browser — or ask about LAN access before exposing it further.
</details>

<details>
<summary><b>Is it safe to put the server on my home network?</b></summary>

The vault stays encrypted either way (decryption happens in your browser), but the page itself would be reachable by anyone on the network. Loopback-only by default is the safe choice — don't port-forward it to the internet.
</details>

<details>
<summary><b>Bitwarden has X — do you?</b></summary>

Logins, cards, identities, notes, SSH keys, folders, TOTP, generator, health, import/export, recovery key, re-prompt, auto-backup. No cloud sync, no browser-extension autofill, no sharing/organizations — that's the offline tradeoff.
</details>

---

## 📂 Files

- `LocalVault.html` — the entire app. Double-click to run.
- `selfhosted/` — loopback server + encrypted disk store + one-script setup ([guide](selfhosted/README.md)).
- `README.md` — you are here. 🙂
