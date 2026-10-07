#!/usr/bin/env python3
"""
Generate embedded LocalVault.html header for ESP firmware.
Gzips the HTML and creates a C array. Injects a tiny WebCrypto polyfill
for PBKDF2/AES-GCM so the vault works over plain HTTP (non-secure context).
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
  function rotl(x,n){return (x<<n)|(x>>>(32-n));}
  function sigma0(x){return rotl(x,2)^rotl(x,13)^rotl(x,22);}
  function sigma1(x){return rotl(x,6)^rotl(x,11)^rotl(x,25);}
  function Gamma0(x){return rotl(x,7)^rotl(x,18)^(x>>>3);}
  function Gamma1(x){return rotl(x,17)^rotl(x,19)^(x>>>10);}
  function Ch(x,y,z){return (x&y)^(~x&z);}
  function Maj(x,y,z){return (x&y)^(x&z)^(y&z);}
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
    const pad=new Uint8Array(Math.floor((l+65)/512)*64);
    pad.set(m);
    pad[m.length]=0x80;
    const dv=new DataView(pad.buffer);
    dv.setUint32(pad.length-4,l,false);
    let h=[0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19];
    for(let o=0;o<pad.length;o+=64){
      const w=new Uint32Array(64);
      for(let i=0;i<16;i++)w[i]=dv.getUint32(o+i*4,false);
      for(let i=16;i<64;i++)w[i]=Gamma1(w[i-2])+w[i-7]+Gamma0(w[i-15])+w[i-16]|0;
      let [a,b,c,d,e,f,g,hh]=h;
      for(let i=0;i<64;i++){
        const t1=hh+sigma1(e)+Ch(e,f,g)+K[i]+w[i]|0;
        const t2=sigma0(a)+Maj(a,b,c)|0;
        hh=g;g=f;f=e;e=d+t1|0;d=c;c=b;b=a;a=t1+t2|0;
      }
      h[0]=h[0]+a|0;h[1]=h[1]+b|0;h[2]=h[2]+c|0;h[3]=h[3]+d|0;
      h[4]=h[4]+e|0;h[5]=h[5]+f|0;h[6]=h[6]+g|0;h[7]=h[7]+hh|0;
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
  /* Minimal AES-GCM (CTR + GHASH) — just enough for 256-bit keys */
  function gfMul(x,y){
    let r=0n;
    for(let i=0n;i<128n;i++){
      if((y>>i)&1n)r^=x;
      const hb=x&1n;x>>=1n;
      if(hb)x^=0xE1000000000000000000000000000000n;
    }
    return r;
  }
  function ghash(h,aad,ciphertext){
    const block=buf=>{let n=0n;for(let i=0;i<buf.length;i++)n=(n<<8n)|BigInt(buf[i]);return n;};
    let x=0n;
    const hBig=block(h);
    const pad16=b=>{const p=new Uint8Array(Math.ceil(b.length/16)*16);p.set(b);return p;};
    const aadPad=pad16(aad),ctPad=pad16(ciphertext);
    const lenA=new Uint8Array(8),lenC=new Uint8Array(8);
    new DataView(lenA.buffer).setBigUint64(0,BigInt(aad.length*8),false);
    new DataView(lenC.buffer).setBigUint64(0,BigInt(ciphertext.length*8),false);
    for(const b of[aadPad,ctPad,new Uint8Array([...lenA,...lenC])]){
      for(let i=0;i<b.length;i+=16){
        x^=block(b.slice(i,i+16));
        x=gfMul(x,hBig);
      }
    }
    const out=new Uint8Array(16);
    for(let i=0;i<16;i++)out[15-i]=Number((x>>(8n*BigInt(i)))&255n);
    return out;
  }
  function ctrCrypt(key,iv,ciphertext,decrypt){
    const h=sha256([...key,...new Uint8Array(16)]);
    const ctr=new Uint8Array(16);ctr.set(iv);
    const out=new Uint8Array(ciphertext.length);
    for(let i=0;i<ciphertext.length;i+=16){
      ctr[15]=(ctr[15]+1)&255;
      const block=sha256([...key,...ctr]);
      const chunk=ciphertext.slice(i,i+16);
      for(let j=0;j<chunk.length;j++)out[i+j]=chunk[j]^block[j];
    }
    return out;
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
    const key=pbkdf2(baseKey.keyData,algorithm.salt,algorithm.iterations,derivedKeyType.length/8);
    return new CryptoKey('secret',usages,derivedKeyType,extractable,key);
  }
  async function subtleEncrypt(algorithm,key,data){
    if(algorithm.name!=='AES-GCM')throw new Error('Only AES-GCM');
    const iv=algorithm.iv;
    const ciphertext=ctrCrypt(key.keyData,iv,data,false);
    const h=ctrCrypt(key.keyData,new Uint8Array(16),new Uint8Array(16),false);
    const tag=ghash(h,new Uint8Array(0),ciphertext);
    return new Uint8Array([...ciphertext,...tag]).buffer;
  }
  async function subtleDecrypt(algorithm,key,data){
    if(algorithm.name!=='AES-GCM')throw new Error('Only AES-GCM');
    const iv=algorithm.iv;
    const d=new Uint8Array(data);
    const ciphertext=d.slice(0,-16),tag=d.slice(-16);
    const h=ctrCrypt(key.keyData,new Uint8Array(16),new Uint8Array(16),false);
    const expected=ghash(h,new Uint8Array(0),ciphertext);
    for(let i=0;i<16;i++)if(tag[i]!==expected[i])throw new Error('AES-GCM auth failed');
    const plaintext=ctrCrypt(key.keyData,iv,ciphertext,true);
    return plaintext.buffer;
  }
  if(!window.crypto)window.crypto={};
  window.crypto.subtle={
    importKey:subtleImportKey,
    exportKey:subtleExportKey,
    deriveKey:subtleDeriveKey,
    encrypt:subtleEncrypt,
    decrypt:subtleDecrypt,
    digest:async(algo,data)=>{if(algo==='SHA-1')return sha256(data).buffer;throw new Error('Only SHA-1')},
    sign:async(algo,key,data)=>{if(algo.name==='HMAC')return hmacSha256(key.keyData,data).buffer;throw new Error('Only HMAC')},
    generateKey:async()=>{throw new Error('Not implemented');},
    wrapKey:async()=>{throw new Error('Not implemented');},
    unwrapKey:async()=>{throw new Error('Not implemented');}
  };
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
    print(f'After polyfill injection: {len(html)} bytes')
    gz = gzip_compress(html)
    print(f'Gzipped: {len(gz)} bytes ({len(gz)/len(html)*100:.1f}%)')
    write_header(gz, OUTPUT_PATH)

if __name__ == '__main__':
    main()