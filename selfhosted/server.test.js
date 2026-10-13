'use strict';

const assert = require('node:assert/strict');
const { spawn, spawnSync } = require('node:child_process');
const fs = require('node:fs');
const http = require('node:http');
const os = require('node:os');
const path = require('node:path');
const { after, before, test } = require('node:test');

const root = path.join(__dirname, '..');
const serverPath = path.join(__dirname, 'server.js');
const storePath = path.join(__dirname, 'vault.store.json');
let child;
let port;
let base;

function request(method, pathname, body) {
  return new Promise((resolve, reject) => {
    const data = body === undefined ? null : Buffer.from(body);
    const req = http.request({
      host: '127.0.0.1',
      port,
      path: pathname,
      method,
      headers: data ? { 'Content-Length': data.length, 'Content-Type': 'application/json' } : {},
    }, (res) => {
      const chunks = [];
      res.on('data', (chunk) => chunks.push(chunk));
      res.on('end', () => resolve({ status: res.statusCode, headers: res.headers, body: Buffer.concat(chunks).toString('utf8') }));
    });
    req.on('error', reject);
    if (data) req.write(data);
    req.end();
  });
}

before(async () => {
  port = await new Promise((resolve, reject) => {
    const probe = http.createServer();
    probe.on('error', reject);
    probe.listen(0, '127.0.0.1', () => {
      const value = probe.address().port;
      probe.close((error) => error ? reject(error) : resolve(value));
    });
  });
  try { fs.unlinkSync(storePath); } catch (error) { if (error.code !== 'ENOENT') throw error; }
  child = spawn(process.execPath, [serverPath], {
    cwd: __dirname,
    env: { ...process.env, LOCALVAULT_PORT: String(port) },
    stdio: 'ignore',
  });
  base = `http://127.0.0.1:${port}`;
  const deadline = Date.now() + 5000;
  while (Date.now() < deadline) {
    if (child.exitCode !== null) throw new Error(`server exited: ${child.exitCode}`);
    try {
      const response = await request('GET', '/health');
      if (response.status === 200) return;
    } catch {}
    await new Promise((resolve) => setTimeout(resolve, 50));
  }
  throw new Error('server did not become ready');
});

after(() => {
  if (child && child.exitCode === null) child.kill();
  try { fs.unlinkSync(storePath); } catch (error) { if (error.code !== 'ENOENT') throw error; }
  for (const name of fs.readdirSync(__dirname)) {
    if (name.startsWith('vault.store.json.') && name.endsWith('.tmp')) fs.unlinkSync(path.join(__dirname, name));
  }
});

test('serves the HTML app with safe response headers', async () => {
  const response = await request('GET', '/');
  assert.equal(response.status, 200);
  assert.match(response.body, /LocalVault/);
  assert.equal(response.headers['x-content-type-options'], 'nosniff');
});

test('persists the encrypted store and responds to concurrent saves', async () => {
  const bodies = Array.from({ length: 8 }, (_, index) => JSON.stringify({
    blob: JSON.stringify({ salt: 'salt', iv: `iv-${index}`, data: `cipher-${index}` }),
    meta: '{}',
    rec: null,
  }));
  const responses = await Promise.all(bodies.map((body) => request('PUT', '/api/store', body)));
  assert.ok(responses.every((response) => response.status === 200));
  const saved = JSON.parse(fs.readFileSync(storePath, 'utf8'));
  assert.ok(bodies.some((body) => JSON.parse(body).blob === saved.blob));
  const loaded = await request('GET', '/api/store');
  assert.equal(loaded.status, 200);
  assert.equal(JSON.parse(loaded.body).blob, saved.blob);
});

test('rejects malformed and oversized store requests cleanly', async () => {
  assert.equal((await request('PUT', '/api/store', '{')).status, 400);
  const oversized = JSON.stringify({ blob: 'x'.repeat(25 * 1024 * 1024 + 1) });
  assert.equal((await request('PUT', '/api/store', oversized)).status, 413);
});

test('rejects invalid ports before listening', () => {
  const result = spawnSync(process.execPath, [serverPath], {
    cwd: __dirname,
    env: { ...process.env, LOCALVAULT_PORT: '70000' },
    encoding: 'utf8',
  });
  assert.notEqual(result.status, 0);
  assert.match(result.stderr, /LOCALVAULT_PORT/);
});
