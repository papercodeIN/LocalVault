'use strict';
/* LocalVault self-hosted loopback server — zero dependencies.
   Serves ../LocalVault.html on 127.0.0.1 only. Vault crypto stays
   client-side (AES-GCM); /api/store persists the already-encrypted
   blob + meta + recovery blob to vault.store.json on local disk.
   The server never sees plaintext. */
const http = require('http');
const fs = require('fs');
const path = require('path');

const HOST = '127.0.0.1';
const PORT = Number(process.env.LOCALVAULT_PORT || 18765);
if (!Number.isInteger(PORT) || PORT < 1 || PORT > 65535) {
  console.error('LOCALVAULT_PORT must be a whole number between 1 and 65535.');
  process.exit(1);
}
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
  } catch (error) {
    if (error.code === 'ENOENT') return null;
    throw error;
  }
}

function readBody(req) {
  return new Promise((resolve, reject) => {
    const chunks = [];
    let size = 0;
    let tooLarge = false;
    req.on('data', (chunk) => {
      size += chunk.length;
      if (size > MAX_BODY) {
        tooLarge = true;
        chunks.length = 0;
        return;
      }
      if (!tooLarge) chunks.push(chunk);
    });
    req.on('end', () => {
      if (tooLarge) reject(Object.assign(new Error('body too large'), { statusCode: 413 }));
      else resolve(Buffer.concat(chunks).toString('utf8'));
    });
    req.on('error', reject);
    req.on('aborted', () => reject(new Error('request aborted')));
  });
}

let writeQueue = Promise.resolve();
function writeStore(next) {
  writeQueue = writeQueue.catch(() => {}).then(() => {
    const tmp = `${STORE_FILE}.${process.pid}.${Date.now()}.tmp`;
    try {
      fs.writeFileSync(tmp, JSON.stringify(next), { mode: 0o600 });
      fs.renameSync(tmp, STORE_FILE);
    } finally {
      try { fs.unlinkSync(tmp); } catch (error) { if (error.code !== 'ENOENT') throw error; }
    }
  });
  return writeQueue;
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
      try {
        const s = readStore();
        if (!s) {
          res.writeHead(404, { 'Content-Type': 'application/json', 'Cache-Control': 'no-store' });
          res.end(JSON.stringify({ ok: false, error: 'empty' }));
          return;
        }
        res.writeHead(200, { 'Content-Type': 'application/json', 'Cache-Control': 'no-store' });
        res.end(JSON.stringify(s));
      } catch (error) {
        console.error('Failed to read encrypted vault store:', error.message);
        res.writeHead(500, { 'Content-Type': 'application/json', 'Cache-Control': 'no-store' });
        res.end(JSON.stringify({ ok: false, error: 'store unavailable' }));
      }
      return;
    }
    if (req.method === 'PUT') {
      try {
        const body = JSON.parse(await readBody(req));
        if (!body || typeof body !== 'object' || Array.isArray(body)) throw new Error('bad shape');
        for (const k of ['blob', 'meta', 'rec']) {
          const value = body[k];
          if (value !== null && value !== undefined && typeof value !== 'string') throw new Error('bad shape');
          if (typeof value === 'string' && Buffer.byteLength(value, 'utf8') > MAX_BODY) throw new Error('value too large');
        }
        if (typeof body.blob === 'string') {
          const blob = JSON.parse(body.blob);
          if (!blob || typeof blob.salt !== 'string' || typeof blob.iv !== 'string' || typeof blob.data !== 'string') throw new Error('bad blob');
        }
        const next = {
          blob: body.blob ?? null,
          meta: body.meta ?? null,
          rec: body.rec ?? null,
          savedAt: Date.now(),
        };
        await writeStore(next);
        res.writeHead(200, { 'Content-Type': 'application/json', 'Cache-Control': 'no-store' });
        res.end(JSON.stringify({ ok: true }));
      } catch (error) {
        const status = error.statusCode || (error instanceof SyntaxError ? 400 : 500);
        if (status === 500) console.error('Failed to save encrypted vault store:', error.message);
        res.writeHead(status, { 'Content-Type': 'application/json', 'Cache-Control': 'no-store' });
        res.end(JSON.stringify({ ok: false, error: status === 413 ? 'body too large' : status === 400 ? 'invalid request' : 'store unavailable' }));
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
