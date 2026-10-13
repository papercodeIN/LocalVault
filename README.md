# 🔐 LocalVault — Offline Password Manager

> Your passwords. Your device. Nobody else's server.

No accounts. No cloud. No tracking. No fees. Just one app that keeps your secrets encrypted on hardware you own.

---

## 🚀 Quick start (60 seconds)

**Option A — double-click & go (simplest)**

1. Double-click `LocalVault.html` — opens in any browser, works fully offline.
2. Create your vault → pick a strong master password (min 12 chars).
3. 💾 **Save the recovery key** shown once — it's your only way back in if you forget the password.

**Option B — run a local server**

On Windows, double-click `selfhosted/start-windows.bat`. On macOS or Linux, open a terminal and run:

```sh
cd selfhosted
node server.js
```

Then open **http://127.0.0.1:18765**. The server keeps an encrypted copy of your vault on this computer. You can open this address in different browsers on the same computer to use the same vault. Use one browser at a time; changes do not appear in other open tabs automatically.

To start the server automatically when you sign in, run `node setup.js` from the `selfhosted` folder and follow the menu. To move an existing vault into the server, use the encrypted backup and restore steps in the [self-hosted guide](selfhosted/README.md).

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

| How you open LocalVault | Where your vault is kept |
|---|---|
| Open `LocalVault.html` | Separately in each browser. Browsers do not share this vault. |
| Use the local server | In the browser and as an encrypted copy on this computer. Browsers using the server address share this vault. |

The server only saves encrypted vault data; it cannot read your passwords. The encrypted copy can restore your vault if browser data is cleared, but it is not a replacement for keeping a separate backup. Weekly snapshots stay in browser storage and are not included in the server's disk copy.

---

## 🔑 Forgot your master password?

1. Lock screen → **Use recovery key** → paste the key you saved at creation.
2. Set a new password. Done — entries preserved, nothing lost.
3. No key *and* no password? The vault is unrecoverable by design — delete and start over (or restore an encrypted backup file).

> ⚠️ Changing your master password or security level **invalidates the old recovery key** — generate a fresh one in Settings → Recovery key.

---

## ⚠️ Honest limitations

- **No password reset.** There is no reset email — your recovery key is the only way back in. Anyone holding it can open your vault.
- **No auto-lock.** It stays open until you click **Lock** or close the tab.
- **Backups are on you.** A snapshot is saved once a week, but only when you open the app (keeps the last 4). Copy the encrypted backup to a USB stick now and then.
- **Exports show passwords.** CSV, Excel and plain JSON are readable by anyone. Only the encrypted backup is safe to keep around.

**📄 Open the HTML file:** Each browser has its own vault. If you clear that browser's saved data, its vault may be lost. Use an encrypted backup to move it to another browser.

**🖥️ Use the local server:** Keep the server running while you use it. It is for this computer only, not for sharing over the internet or with other computers. Browsers opened at the server address use the same vault, but changes do not show up in other open tabs right away; use one browser at a time. Weekly snapshots stay in browser storage and are not saved in the server's disk copy. Keep a separate encrypted backup too.

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
