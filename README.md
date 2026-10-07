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

No software is perfect. Here's what LocalVault can't do, in plain words.

**Good to know, whichever version you use**

- **It won't sync for you.** A change you make in one browser won't appear in another on its own. You have to move it yourself (see the notes below for each version).
- **There's no "forgot password" email.** There's no company behind this to send you a reset link. Your recovery key is the only reset — so keep it somewhere safe and private. Anyone who finds it can open your vault.
- **Backups are your job.** Every so often, download the backup file and put it somewhere safe, like a USB stick.
- **Backups only happen while you're using it.** The app quietly saves a snapshot once a week, but only when you open it and it's been a week. If you never open it, no snapshot is made. It also keeps only the last 4.
- **It won't lock itself.** Your vault stays open until you click **Lock** or close the tab. On a shared computer, remember to lock it.
- **Most exports are readable by anyone.** CSV, Excel and plain JSON exports show your passwords in clear text. Only the *encrypted* backup file is safe to leave lying around.

**📄 If you just double-click the file (`LocalVault.html`)**

- **Only that one browser has your vault.** Chrome, Edge and Firefox each keep their own separate copy. To move your vault between them, you export it from one and import it into the other.
- **Clearing your browser data will delete it.** If you clear cookies or site data, your vault and its snapshots are gone — there's no other copy anywhere.
- **There's only a little room.** Browsers only allow a few megabytes of storage. Fine for passwords, but don't expect to store documents.

**🖥️ If you run the self-hosted version (`selfhosted/`)**

- **It only works while the program is running.** If LocalVault isn't running, the page won't open at all. Run `node setup.js` once to start it automatically when you log in.
- **Anyone using this computer can reach it.** There's no password on the server itself. Your passwords stay encrypted, so they can't be read — but a program on your machine could overwrite your saved file. That's why it runs only on your own computer and must never be opened up to your network.
- **Don't use two browsers at the same time.** Chrome, Edge and Firefox can all open the same vault — they pick up the latest saved copy when they start. But if two of them are open at once, whichever saves last overwrites the other's changes. Use one at a time, and reopen the tab to get the newest version.
- **It won't warn you if it loses connection.** If the program stops while you're working, your changes quietly stay in the browser and are saved later. You won't see an error.
- **Weekly snapshots stay in the browser.** The vault itself is copied to a file on your disk, but the weekly snapshots are not. Clear your browser data and the vault comes back — the snapshots don't.

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
