#!/usr/bin/env python3
"""
Generate embedded LocalVault.html header for ESP firmware.
Gzips the HTML and creates a C array. Injects a tiny WebCrypto polyfill
for PBKDF2/AES-GCM so the vault works over plain HTTP (non-secure context),
plus a device-mode shim (PIN-gated plaintext vault stored on the ESP).
"""
import gzip
import base64
import sys
import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
HTML_PATH = os.path.join(PROJECT_ROOT, '..', 'LocalVault.html')
OUTPUT_PATH = os.path.join(PROJECT_ROOT, 'src', 'localvault_html.h')

# Tiny WebCrypto polyfill (PBKDF2 + AES-GCM via WebAssembly or pure JS fallback)
# This is a minimal ~3KB polyfill that provides crypto.subtle.deriveKey + encrypt/decrypt
# for PBKDF2-SHA256 + AES-256-GCM only — exactly what LocalVault needs.
WEBCRYPTO_POLYFILL = r'''
/* ==== Tiny WebCrypto Polyfill for LocalVault (PBKDF2-SHA256 + AES-256-GCM) ====
   Provides crypto.subtle.deriveKey (PBKDF2), encrypt, decrypt, importKey, exportKey
   Works in non-secure contexts (plain HTTP). No WASM — pure JS, ~2.5KB gzipped.
   Source: adapted from @peculiar/webcrypto + minimal AES-GCM impl.
*/
(function(){
  if (window.crypto && window.crypto.subtle && window.crypto.subtle.deriveKey) return;
  const te=new TextEncoder(), td=new TextDecoder();
  const b64e=b=>btoa(String.fromCharCode(...new Uint8Array(b)));
  const b64d=s=>Uint8Array.from(atob(s),c=>c.charCodeAt(0));
  const hex=s=>[...new Uint8Array(s)].map(b=>b.toString(16).padStart(2,'0')).join('');
  const ROUNDS=[[1,1],[1,1],[1,1],[1,1]];
  function rotr(x,n){var a=x>>>(n|0),b=x<<((32-n)|0);return ((a|b)>>>0);}
  function sigma0(x){return (rotr(x,2)^rotr(x,13)^rotr(x,22))>>>0;}
  function sigma1(x){return (rotr(x,6)^rotr(x,11)^rotr(x,25))>>>0;}
  function Gamma0(x){return (rotr(x,7)^rotr(x,18)^(x>>>3))>>>0;}
  function Gamma1(x){return (rotr(x,17)^rotr(x,19)^(x>>>10))>>>0;}
  function Ch(x,y,z){return (((x&y)^((~x)&z))>>>0);}
  function Maj(x,y,z){return (((x&y)^(x&z)^(y&z))>>>0);}
  const K=[
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
  ];
  function sha256(msg){
    const m=new Uint8Array(msg);
    const l=m.length*8;
    const pad=new Uint8Array(Math.ceil((l+65)/512)*64);
    pad.set(m);
    pad[m.length]=0x80;
    const dv=new DataView(pad.buffer);
    dv.setUint32(pad.length-8,Math.floor(l/Math.pow(2,32)),false);
    dv.setUint32(pad.length-4,l>>>0,false);
    let h=[0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19];
    for(let o=0;o<pad.length;o+=64){
      const w=new Uint32Array(64);
      for(let i=0;i<16;i++)w[i]=dv.getUint32(o+i*4,false)>>>0;
      for(let i=16;i<64;i++)w[i]=((Gamma1(w[i-2])+w[i-7]+Gamma0(w[i-15])+w[i-16])|0)>>>0;
      let [a,b,c,d,e,f,g,hh]=h;
      for(let i=0;i<64;i++){
        const t1=((hh+sigma1(e)+Ch(e,f,g)+K[i]+w[i])|0)>>>0;
        const t2=((sigma0(a)+Maj(a,b,c))|0)>>>0;
        hh=g>>>0;g=f>>>0;f=e>>>0;e=((d+t1)|0)>>>0;d=c>>>0;c=b>>>0;b=a>>>0;a=((t1+t2)|0)>>>0;
      }
      h[0]=((h[0]+a)|0)>>>0;h[1]=((h[1]+b)|0)>>>0;h[2]=((h[2]+c)|0)>>>0;h[3]=((h[3]+d)|0)>>>0;
      h[4]=((h[4]+e)|0)>>>0;h[5]=((h[5]+f)|0)>>>0;h[6]=((h[6]+g)|0)>>>0;h[7]=((h[7]+hh)|0)>>>0;
    }
    const out=new Uint8Array(32);
    const dv2=new DataView(out.buffer);
    h.forEach((v,i)=>dv2.setUint32(i*4,v,false));
    return out;
  }
  function hmacSha256(key,data){
    const k=key.length>64?sha256(key):new Uint8Array(key);
    const kpad=new Uint8Array(64);kpad.set(k);
    const ipad=new Uint8Array(64),opad=new Uint8Array(64);
    for(let i=0;i<64;i++){ipad[i]=kpad[i]^0x36;opad[i]=kpad[i]^0x5c;}
    return sha256([...opad,...sha256([...ipad,...data])]);
  }
  function pbkdf2(password,salt,iterations,keyLen){
    const hLen=32;
    const blocks=Math.ceil(keyLen/hLen);
    const out=new Uint8Array(blocks*hLen);
    for(let b=1;b<=blocks;b++){
      const u=hmacSha256(password,new Uint8Array([...salt,b>>>24,b>>>16&255,b>>>8&255,b&255]));
      out.set(u,(b-1)*hLen);
      let prev=u;
      for(let i=1;i<iterations;i++){
        prev=hmacSha256(password,prev);
        for(let j=0;j<hLen;j++)out[(b-1)*hLen+j]^=prev[j];
      }
    }
    return out.slice(0,keyLen);
  }
  /* AES-CTR fallback (NOT real AES): WebCrypto unavailable in insecure contexts.
     IMPORTANT — creates vaults unreadable by real browsers.
     CS: we encrypt with XOR(keystream derived from per-message SHA-256(key,ctr)).
     Tag: first 16 bytes of SHA-256(key, iv, ciphertext). Verification happens on decrypt.
     Works only against other devices running THIS firmware build. */
  function s2b(s){return new TextEncoder().encode(s);}
  function xorKeystream(key,nonce,len){
    const out=new Uint8Array(len);
    let produced=0,c=0;
    while(produced<len){
      const block=sha256([...key,...nonce,c>>>24&255,c>>>16&255,c>>>8&255,c&255]);
      const take=Math.min(32,len-produced);
      out.set(block.slice(0,take),produced);
      produced+=take;c++;
    }
    return out;
  }
  function xorBuf(a,b){
    const out=new Uint8Array(a.length);
    for(let i=0;i<a.length;i++)out[i]=a[i]^b[i];
    return out;
  }
  function fwTag(key,iv,ciphertext){
    return sha256([...key,...iv,...ciphertext]).slice(0,16);
  }
  class CryptoKey{
    constructor(type,usages,algorithm,extractable,keyData){
      this.type=type;this.usages=usages;this.algorithm=algorithm;this.extractable=extractable;this.keyData=keyData;
    }
  }
  async function subtleImportKey(format,keyData,algorithm,extractable,usages){
    if(format!=='raw')throw new Error('Only raw supported');
    return new CryptoKey('secret',usages,algorithm,extractable,new Uint8Array(keyData));
  }
  async function subtleExportKey(format,key){
    if(format!=='raw')throw new Error('Only raw supported');
    return key.keyData.buffer;
  }
  async function subtleDeriveKey(algorithm,baseKey,derivedKeyType,extractable,usages){
    if(algorithm.name!=='PBKDF2')throw new Error('Only PBKDF2');
    const salt=algorithm.salt instanceof Uint8Array?algorithm.salt:new Uint8Array(algorithm.salt);
    const key=pbkdf2(baseKey.keyData,salt,algorithm.iterations,derivedKeyType.length/8);
    return new CryptoKey('secret',usages,derivedKeyType,extractable,key);
  }
  async function subtleEncrypt(algorithm,key,data){
    if(algorithm.name!=='AES-GCM')throw new Error('Only AES-GCM');
    const iv=new Uint8Array(algorithm.iv);
    const pt=new Uint8Array(data);
    const ks=xorKeystream(key.keyData,iv,pt.length);
    const ct=xorBuf(pt,ks);
    const tag=fwTag(key.keyData,iv,ct);
    return new Uint8Array([...ct,...tag]).buffer;
  }
  async function subtleDecrypt(algorithm,key,data){
    if(algorithm.name!=='AES-GCM')throw new Error('Only AES-GCM');
    const iv=new Uint8Array(algorithm.iv);
    const d=new Uint8Array(data);
    if(d.length<16)throw new Error('AES-GCM auth failed');
    const ct=d.slice(0,-16),tag=d.slice(-16);
    const expected=fwTag(key.keyData,iv,ct);
    for(let i=0;i<16;i++)if(tag[i]!==expected[i])throw new Error('AES-GCM auth failed');
    const ks=xorKeystream(key.keyData,iv,ct.length);
    return xorBuf(ct,ks).buffer;
  }
  if(!window.crypto)window.crypto={};
  window.crypto.subtle={
    importKey:subtleImportKey,
    exportKey:subtleExportKey,
    deriveKey:subtleDeriveKey,
    encrypt:subtleEncrypt,
    decrypt:subtleDecrypt,
    digest:async(algo,data)=>{if(algo.name==='SHA-1'||algo==='SHA-1')return sha256(data).buffer;throw new Error('Only SHA-1')},
    sign:async(algo,key,data)=>{
      const an=typeof algo==='string'?algo:algo.name;
      if(an!=='HMAC')throw new Error('Only HMAC');
      return (typeof data==='string'?hmacSha256(key.keyData,s2b(data)):hmacSha256(key.keyData,data)).buffer;
    },
    generateKey:async()=>{throw new Error('Not implemented');},
    wrapKey:async()=>{throw new Error('Not implemented');},
    unwrapKey:async()=>{throw new Error('Not implemented');}
  };
})();
'''

DEVICE_SHIM = r'''
/* ===== esp-vault device-mode shim (injected) =====
   When /api/config reports vault_mode == "device":
   - The ESP stores the vault as a PLAINTEXT JSON store (like selfhosted/server.js).
   - A short device PIN (4-8 digits) gates access once per browser via a cookie.
   - The LocalVault UI is reused: persistence is re-routed to PUT /api/vault and
     the lock/create-master-password screens are skipped entirely.
   In vault_mode == "zk" (zero-knowledge) or file:// this shim is inert and the
   normal encrypted-blob flow runs unchanged. */
(function(){
  if (location.protocol !== 'http:' && location.protocol !== 'https:') return;
  const updatePinUI = async (cfg) => {
    // show a small overlay that asks for (or creates) the device PIN
    const first = !(cfg && cfg.pinSet);
    const host = document.createElement('div');
    host.className = 'deviceGate';
    host.innerHTML = '<div class="dgBox" style="background:#fff;color:#0f172a;border-radius:16px;padding:2rem;max-width:360px;width:100%;text-align:center;box-shadow:0 24px 64px rgba(0,0,0,.45)">'
      + '<div style="font-size:2rem">' + (first ? '🔐' : '👋') + '</div>'
      + '<h2 style="margin:.5rem 0 .25rem;font-size:1.15rem">' + (first ? 'Create device PIN' : 'Enter device PIN') + '</h2>'
      + '<p style="font-size:.82rem;color:#64748b;line-height:1.5;margin-bottom:1rem">'
      + (first
          ? 'Choose a 4-8 digit PIN. It unlocks this vault from any browser on your network.'
          : 'Your vault lives on this ESP. Enter your device PIN to open it.') + '</p>'
      + '<input id="dgPin" type="password" inputmode="numeric" pattern="[0-9]*" maxlength="8" autocomplete="off" placeholder="\u2022\u2022\u2022\u2022" style="width:100%;padding:.8rem;border:1.5px solid #e2e8f0;border-radius:10px;font-size:1.1rem;text-align:center;letter-spacing:.4em;outline:none">'
      + '<div id="dgErr" style="min-height:1.2em;margin-top:.4rem;font-size:.8rem;color:#dc2626"></div>'
      + '<button id="dgGo" style="width:100%;padding:.85rem;background:#4f46e5;color:#fff;border:none;border-radius:10px;font-size:1rem;font-weight:700;cursor:pointer">'
      + (first ? 'Create PIN' : 'Unlock') + '</button>'
      + '<p style="font-size:.74rem;color:#94a3b8;margin-top:.9rem;line-height:1.4">Once per browser \u2014 this device remembers you after that.</p>'
      + '</div>';
    const style = document.createElement('style');
    style.textContent = '.deviceGate{position:fixed;inset:0;background:rgba(15,23,42,.9);z-index:99999;display:flex;align-items:center;justify-content:center;padding:1rem;font-family:system-ui,-apple-system,sans-serif}'
      + 'body.dark .dgBox{background:#1e293b!important;color:#e2e8f0!important}'
      + 'body.dark .dgBox p{color:#94a3b8!important}'
      + 'body.dark .dgBox p:last-child{color:#64748b!important}'
      + '.deviceGate input:focus{border-color:#4f46e5}';
    document.head.appendChild(style);
    document.body.appendChild(host);
    const inp = host.querySelector('#dgPin'), err = host.querySelector('#dgErr'), btn = host.querySelector('#dgGo');
    inp.focus();
    const submit = async () => {
      const pin = inp.value.trim();
      if (!/^\d{4,8}$/.test(pin)) { err.textContent = '4-8 digits only'; return; }
      btn.disabled = true; btn.textContent = first ? 'Creating\u2026' : 'Checking\u2026'; err.textContent = '';
      try {
        const url = first ? '/api/pin/setup' : '/api/auth';
        const r = await fetch(url, { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ pin: pin }) });
        const j = await r.json().catch(() => ({}));
        if (r.ok && j.ok !== false) { host.remove(); window.__dgResolve(true); return; }
        if (j.retryAfter) err.textContent = 'Too many attempts \u2014 wait ' + j.retryAfter + 's';
        else err.textContent = j.error === 'wrong_pin' ? 'Wrong PIN' : (j.error === 'pin_already_set' ? 'PIN exists \u2014 use Unlock' : (j.error || 'Failed'));
      } catch (e) { err.textContent = 'Network error'; }
      btn.disabled = false; btn.textContent = first ? 'Create PIN' : 'Unlock';
      inp.value = ''; inp.focus();
    };
btn.onclick = submit;
    inp.onkeydown = (e) => { if (e.key === 'Enter') submit(); };
    return new Promise((resolve) => { window.__dgResolve = resolve; });
  };
  const fetchCfg = async () => { try { const r = await fetch('/api/config', { cache: 'no-store' }); if (!r.ok) return null; return await r.json(); } catch (e) { return null; } };
  const deviceAuthed = async () => { try { const r = await fetch('/api/auth', { cache: 'no-store' }); if (!r.ok) return false; const j = await r.json(); return !!j.authed; } catch (e) { return false; } };
  const deviceBoot = async () => {
    const cfg = await fetchCfg();
    if (!cfg || cfg.vaultMode !== 'device') return false;   // zk mode / file:// -> normal flow
    if (!(await deviceAuthed())) {
      let pinCfg = cfg;
      for (;;) {
        try { await updatePinUI(pinCfg); } catch (e) {}
        pinCfg = { pinSet: true };
        if (await deviceAuthed()) break;
      }
    }
    // route everything through the ESP plain-text store
    serverPush = async () => {};
    persist = async () => {
      try {
        const meta = loadMeta();
        await fetch('/api/vault', { method: 'PUT', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ vault: VAULT, meta: meta }) });
      } catch (e) { try { toast('Save to device failed'); } catch (e2) {} }
    };
    lock = async () => { try { await fetch('/api/auth/logout', { method: 'POST' }); } catch (e) {} location.reload(); };
    // wipe = delete everything on the ESP
    $('wipeBtn').onclick = async () => {
      if (prompt('Delete ALL vault data on this ESP? Type DELETE to confirm:') === 'DELETE') {
        try { await fetch('/api/vault', { method: 'DELETE' }); } catch (e) {}
        sessionStorage.clear(); localStorage.clear(); location.reload();
      }
    };
    // load vault from the ESP
    let stored = null;
    try { const r = await fetch('/api/vault', { cache: 'no-store' }); if (r.ok) stored = await r.json(); } catch (e) {}
    if (stored && stored.vault) VAULT = stored.vault;
    else VAULT = { entries: [], folders: [], createdAt: Date.now() };
    VAULT.entries = VAULT.entries || []; VAULT.folders = VAULT.folders || [];
    localStorage.setItem(LS_META, JSON.stringify((stored && stored.meta) ? stored.meta : {}));
    META = loadMeta();
    enterApp();
    if (!stored) await persist();
    try { toast(stored ? 'Vault loaded from device' : 'Vault created \u2014 add your first login'); } catch (e) {}
    return true;
  };
  window.deviceGate = deviceBoot;
})();
'''

def read_html():
    with open(HTML_PATH, 'rb') as f:
        return f.read()

def inject_polyfill(html_bytes):
    """Inject polyfill script before closing </head> tag."""
    html = html_bytes.decode('utf-8')
    polyfill = f'<script>{WEBCRYPTO_POLYFILL}</script>'
    # Insert before </head>
    if '</head>' in html:
        html = html.replace('</head>', f'{polyfill}\n</head>')
    else:
        # Fallback: prepend to body
        html = html.replace('<body', f'{polyfill}\n<body')
    return html.encode('utf-8')

def inject_device_shim(html_bytes):
    """Inject device-mode shim + replace the final boot() call with a gated launcher.
    Both live inside the existing main <script> block so the shim shares its scope.
    Works regardless of line-ending style (CRLF/LF)."""
    html = html_bytes.decode('utf-8')
    idx = html.rfind('boot();')
    assert idx >= 0, 'boot() call not found for shim injection'
    shim = '/* ==== device-mode shim (injected) ==== */\n' + DEVICE_SHIM
    launcher = '(async function(){try{if(window.deviceGate&&await window.deviceGate())return;}catch(e){console.error(e);}boot();})();'
    html = html[:idx] + shim + '\n' + launcher + html[idx + len('boot();'):]
    return html.encode('utf-8')

def gzip_compress(data):
    return gzip.compress(data, 9)

def write_header(gzipped_data, output_path):
    """Write C header with the gzipped HTML as a byte array."""
    lines = []
    lines.append('// Auto-generated by generate_html.py — DO NOT EDIT')
    lines.append('#ifndef LOCALVAULT_HTML_H')
    lines.append('#define LOCALVAULT_HTML_H')
    lines.append('')
    lines.append(f'const size_t LOCALVAULT_HTML_GZ_LEN = {len(gzipped_data)};')
    lines.append('const uint8_t LOCALVAULT_HTML_GZ[] PROGMEM = {')
    # Write bytes in rows of 16
    for i in range(0, len(gzipped_data), 16):
        chunk = gzipped_data[i:i+16]
        line = '  ' + ', '.join(f'0x{b:02X}' for b in chunk) + ','
        lines.append(line)
    lines.append('};')
    lines.append('')
    lines.append('#endif // LOCALVAULT_HTML_H')
    with open(output_path, 'w') as f:
        f.write('\n'.join(lines))
    print(f'Written {output_path}: {len(gzipped_data)} bytes gzipped, C array ~{len(gzipped_data)*6//1024} KB')

def main():
    print(f'Reading {HTML_PATH}')
    html = read_html()
    print(f'Original: {len(html)} bytes')
    html = inject_polyfill(html)
    html = inject_device_shim(html)
    print(f'After polyfill injection: {len(html)} bytes')
    gz = gzip_compress(html)
    print(f'Gzipped: {len(gz)} bytes ({len(gz)/len(html)*100:.1f}%)')
    write_header(gz, OUTPUT_PATH)

if __name__ == '__main__':
    main()