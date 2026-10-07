'use strict';
/* LocalVault self-hosted loopback server — zero dependencies.
   Serves ../LocalVault.html on 127.0.0.1 only. Vault crypto stays
   client-side (AES-GCM); /api/store persists the already-encrypted
   blob + meta + recovery blob to vault.store.json on local disk.
   The server never sees plaintext. */
const http = require('http');
const fs = require('fs');
const path = require('path');

const HOST = process.env.LOCALVAULT_HOST || '127.0.0.1';
const PORT = parseInt(process.env.LOCALVAULT_PORT || '18765', 10);
const FILE = path.join(__dirname, '..', 'LocalVault.html');
const STORE_FILE = path.join(__dirname, 'vault.store.json');
const MAX_BODY = 25 * 1024 * 1024;

let cached = null;
let mtimeMs = 0;
function loadHtml() {
  const st = fs.statSync(FILE);
  if (!cached || st.mtimeMs !== mtimeMs) {
    cached = fs.readFileSync(FILE);
    mtimeMs = st.mtimeMs;
  }
  return cached;
}

function readStore() {
  try {
    return JSON.parse(fs.readFileSync(STORE_FILE, 'utf8'));
  } catch {
    return null;
  }
}

function readBody(req) {
  return new Promise((resolve, reject) => {
    const chunks = [];
    let size = 0;
    req.on('data', (c) => {
      size += c.length;
      if (size > MAX_BODY) {
        reject(new Error('body too large'));
        req.destroy();
        return;
      }
      chunks.push(c);
    });
    req.on('end', () => resolve(Buffer.concat(chunks).toString('utf8')));
    req.on('error', reject);
  });
}

const server = http.createServer(async (req, res) => {
  let pathname = '/';
  try {
    pathname = new URL(req.url, 'http://localhost').pathname;
  } catch { /* fall through to 404 */ }

  if (pathname === '/health') {
    res.writeHead(200, { 'Content-Type': 'application/json', 'Cache-Control': 'no-store' });
    res.end(JSON.stringify({ ok: true, app: 'localvault', time: new Date().toISOString() }));
    return;
  }

  if (pathname === '/api/store') {
    if (req.method === 'GET') {
      const s = readStore();
      if (!s) {
        res.writeHead(404, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ ok: false, error: 'empty' }));
        return;
      }
      res.writeHead(200, { 'Content-Type': 'application/json', 'Cache-Control': 'no-store' });
      res.end(JSON.stringify(s));
      return;
    }
    if (req.method === 'PUT') {
      try {
        const body = JSON.parse(await readBody(req));
        if (!body || typeof body !== 'object' || Array.isArray(body)) throw new Error('bad shape');
        for (const k of ['blob', 'meta', 'rec']) {
          const v = body[k];
          if (v !== null && v !== undefined && typeof v !== 'string') throw new Error('bad shape');
          if (typeof v === 'string' && v.length > MAX_BODY) throw new Error('value too large');
        }
        if (typeof body.blob === 'string') {
          const b = JSON.parse(body.blob);
          if (!b || !b.salt || !b.data) throw new Error('bad blob');
        }
        const next = {
          blob: body.blob ?? null,
          meta: body.meta ?? null,
          rec: body.rec ?? null,
          savedAt: Date.now(),
        };
        const tmp = STORE_FILE + '.tmp';
        fs.writeFileSync(tmp, JSON.stringify(next));
        fs.renameSync(tmp, STORE_FILE);
        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ ok: true }));
      } catch (e) {
        res.writeHead(400, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ ok: false, error: String((e && e.message) || e) }));
      }
      return;
    }
    res.writeHead(405, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify({ ok: false, error: 'method not allowed' }));
    return;
  }

  if (req.method === 'GET' && (pathname === '/' || pathname === '/index.html')) {
    try {
      const body = loadHtml();
      res.writeHead(200, {
        'Content-Type': 'text/html; charset=utf-8',
        'Cache-Control': 'no-store',
        'X-Content-Type-Options': 'nosniff',
        'Referrer-Policy': 'no-referrer',
      });
      res.end(body);
    } catch {
      res.writeHead(500, { 'Content-Type': 'text/plain' });
      res.end('LocalVault.html not found next to selfhosted/');
    }
    return;
  }
  res.writeHead(404, { 'Content-Type': 'text/plain' });
  res.end('Not found');
});

server.on('error', (e) => {
  console.error('Server error:', e.message);
  process.exit(1);
});

server.listen(PORT, HOST, () => {
  console.log(`LocalVault running at http://${HOST}:${PORT} (loopback only)`);
});
