'use strict';
/* LocalVault setup — one script for every OS.
   Detects the OS and installs/removes login autostart for server.js.
   Usage: node setup.js [install|uninstall]   (no arg = menu) */
const fs = require('fs');
const path = require('path');
const os = require('os');
const readline = require('readline');
const { spawnSync } = require('child_process');

const DIR = __dirname;
const SERVER = path.join(DIR, 'server.js');
const NODE = process.execPath;
const OS = process.platform; // win32 | linux | darwin

function winVbsPath() {
  return path.join(
    process.env.APPDATA || path.join(os.homedir(), 'AppData', 'Roaming'),
    'Microsoft', 'Windows', 'Start Menu', 'Programs', 'Startup', 'LocalVaultServer.vbs'
  );
}
const LINUX_UNIT = path.join(os.homedir(), '.config', 'systemd', 'user', 'localvault.service');
const MAC_PLIST = path.join(os.homedir(), 'Library', 'LaunchAgents', 'com.localvault.server.plist');

function status() {
  if (OS === 'win32') return fs.existsSync(winVbsPath());
  if (OS === 'linux') return fs.existsSync(LINUX_UNIT);
  if (OS === 'darwin') return fs.existsSync(MAC_PLIST);
  return false;
}

function install() {
  if (OS === 'win32') {
    const vbs = `Set WshShell = CreateObject("WScript.Shell")\r\nWshShell.Run "node \\"${SERVER}\\"", 0, False\r\n`;
    fs.mkdirSync(path.dirname(winVbsPath()), { recursive: true });
    fs.writeFileSync(winVbsPath(), vbs);
    console.log('Installed: server starts hidden on Windows login.');
  } else if (OS === 'linux') {
    const unit = `[Unit]\nDescription=LocalVault loopback password manager\nAfter=network.target\n\n`
      + `[Service]\nExecStart=${NODE} ${SERVER}\nRestart=on-failure\n`
      + `Environment=LOCALVAULT_HOST=127.0.0.1\nEnvironment=LOCALVAULT_PORT=18765\n\n`
      + `[Install]\nWantedBy=default.target\n`;
    fs.mkdirSync(path.dirname(LINUX_UNIT), { recursive: true });
    fs.writeFileSync(LINUX_UNIT, unit);
    const r1 = spawnSync('systemctl', ['--user', 'daemon-reload'], { stdio: 'inherit' });
    const r2 = spawnSync('systemctl', ['--user', 'enable', '--now', 'localvault.service'], { stdio: 'inherit' });
    if (r1.status !== 0 || r2.status !== 0) console.log('Unit file written; enable manually: systemctl --user enable --now localvault.service');
    else console.log('Installed: systemd user service enabled.');
  } else if (OS === 'darwin') {
    const plist = `<?xml version="1.0" encoding="UTF-8"?>\n`
      + `<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">\n`
      + `<plist version="1.0">\n<dict>\n`
      + `  <key>Label</key><string>com.localvault.server</string>\n`
      + `  <key>ProgramArguments</key>\n  <array>\n    <string>${NODE}</string>\n    <string>${SERVER}</string>\n  </array>\n`
      + `  <key>EnvironmentVariables</key>\n  <dict>\n    <key>LOCALVAULT_HOST</key><string>127.0.0.1</string>\n`
      + `    <key>LOCALVAULT_PORT</key><string>18765</string>\n  </dict>\n`
      + `  <key>RunAtLoad</key><true/>\n  <key>KeepAlive</key><true/>\n</dict>\n</plist>\n`;
    fs.mkdirSync(path.dirname(MAC_PLIST), { recursive: true });
    fs.writeFileSync(MAC_PLIST, plist);
    spawnSync('launchctl', ['unload', MAC_PLIST], { stdio: 'ignore' });
    const r = spawnSync('launchctl', ['load', MAC_PLIST], { stdio: 'inherit' });
    console.log(r.status === 0 ? 'Installed: LaunchAgent loaded.' : 'Plist written; load manually: launchctl load ' + MAC_PLIST);
  } else {
    console.log('Unsupported OS: ' + OS);
    process.exit(1);
  }
  console.log('Open: http://127.0.0.1:18765');
}

function uninstall() {
  if (OS === 'win32') {
    try { fs.unlinkSync(winVbsPath()); } catch { /* already gone */ }
    console.log('Removed. Stop a running server with stop-windows.bat or Task Manager.');
  } else if (OS === 'linux') {
    spawnSync('systemctl', ['--user', 'disable', '--now', 'localvault.service'], { stdio: 'ignore' });
    try { fs.unlinkSync(LINUX_UNIT); } catch { /* already gone */ }
    spawnSync('systemctl', ['--user', 'daemon-reload'], { stdio: 'ignore' });
    console.log('Removed.');
  } else if (OS === 'darwin') {
    spawnSync('launchctl', ['unload', MAC_PLIST], { stdio: 'ignore' });
    try { fs.unlinkSync(MAC_PLIST); } catch { /* already gone */ }
    console.log('Removed.');
  } else {
    console.log('Unsupported OS: ' + OS);
    process.exit(1);
  }
}

async function menu() {
  console.log(`OS detected: ${OS}  |  autostart: ${status() ? 'INSTALLED' : 'not installed'}`);
  console.log('  1) Install (auto-start server on login)');
  console.log('  2) Uninstall (remove auto-start)');
  const rl = readline.createInterface({ input: process.stdin, output: process.stdout });
  const ans = await new Promise((res) => rl.question('Choose 1 or 2: ', res));
  rl.close();
  const c = ans.trim();
  if (c === '1' || /^install/i.test(c)) install();
  else if (c === '2' || /^uninstall/i.test(c)) uninstall();
  else { console.log('Cancelled.'); process.exit(1); }
}

(async () => {
  const arg = (process.argv[2] || '').toLowerCase();
  if (arg === 'install' || arg === '1') install();
  else if (arg === 'uninstall' || arg === '2') uninstall();
  else if (!arg) await menu();
  else { console.log('Usage: node setup.js [install|uninstall]'); process.exit(1); }
})();
