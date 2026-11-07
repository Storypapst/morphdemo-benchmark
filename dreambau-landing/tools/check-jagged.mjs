#!/usr/bin/env node
// Browser and byte checks for the campaign. Run against the local apex CSP or the live directory.
import assert from 'node:assert/strict';
import fs from 'node:fs/promises';
import path from 'node:path';
import { JAGGED_BUDGET } from './budget.mjs';
import { launch, openProduction, pageUrl, ROOT } from './lib.mjs';

const url = new URL(process.env.BASE_URL || 'http://localhost:8080/jagged-intelligence/');
assert(url.pathname.endsWith('/'), 'BASE_URL must be the campaign directory with a trailing slash');
process.env.BASE_URL = url.href;
const output = process.env.CHECK_OUT || path.join(ROOT, 'verification/jagged/tmp');
await fs.mkdir(output, { recursive: true });
const ids = Object.keys(JAGGED_BUDGET);
const stageTimes = [10, 25, 40, 55];
const report = { url: url.href, productions: {} };
const files = ['index.html', 'style.css', 'shell.js', 'story.js', ...ids.map(id => 'p/' + id + '.js')];
const distance = (a, b) => a.reduce((sum, n, i) => sum + Math.abs(n - b[i]), 0) / a.length;
const site = path.join(ROOT, 'site/jagged-intelligence');

for (const file of files) {
  const local = await fs.readFile(path.join(site, file));
  const response = await fetch(new URL(file, url), { signal: AbortSignal.timeout(10000), redirect: 'error' });
  assert.equal(response.status, 200, file);
  assert.deepEqual(Buffer.from(await response.arrayBuffer()), local, 'same bytes over HTTP: ' + file);
  if (file === 'index.html') {
    const csp = response.headers.get('content-security-policy') || '';
    assert(csp.includes("script-src 'self'") && csp.includes("style-src 'self'") && !csp.includes('unsafe-inline'), 'strict CSP');
  }
}
assert.equal((await fetch(new URL('p/does-not-exist.js', url))).status, 404, 'missing assets return 404');

const browser = await launch();
try {
  for (const id of ids) {
    const size = (await fs.stat(path.join(site, 'p', id + '.js'))).size;
    assert(size > 0 && size <= JAGGED_BUDGET[id], id + ' byte budget');
    const { page, logs, requests, audioMs } = await openProduction(browser, id, { w: 1440, h: 900, q: .85 });
    try {
      const state = await page.evaluate(times => {
        const t = Dream.test;
        return {
          def: t.def(), intro: t.lum(0), end: t.lum(55), images: times.map(s => t.thumb(s)),
          captions: [47, 49.3, 53].map(s => t.textContrast(s).map(l => l.ratio)),
          audio: t.audioInfo(),
          cueEnergy: [45.7, 48.1, 50.5].map(s => t.env(s)[3]),
        };
      }, stageTimes);
      assert.equal(state.def.dur, 60);
      assert(state.def.fin < state.def.cta && state.def.cta < state.def.dur);
      assert(state.intro.mean < .08 && state.end.bottom < .12, id + ' dark opening and closing band');
      assert(state.images.every(image => image.some(n => n > .15)), id + ' non-blank chapters');
      const changes = state.images.slice(1).map((thumb, i) => distance(thumb, state.images[i]));
      assert(changes.every(d => d > .003), id + ' visible change between phases: ' + changes);
      assert(state.captions.every((lines, i) => lines.every((ratio, j) => j <= i ? ratio >= 4.5 : ratio < 4.5)), id + ' ordered, legible copy');
      assert(state.audio?.dur > 60 && state.cueEnergy.every(n => n > .08), id + ' audible title cues');
      assert.deepEqual(logs.filter(s => /^\[(error|pageerror)\]/.test(s)), []);
      const loaded = requests.map(r => new URL(r));
      assert(loaded.every(r => r.origin === url.origin), id + ' no third-party requests');
      assert.deepEqual(loaded.filter(r => /\/p\/[^/]+\.js$/.test(r.pathname)).map(r => path.basename(r.pathname)), [id + '.js']);
      assert.equal(await page.locator('[data-cut][aria-current]').getAttribute('data-cut'), id);
      for (const t of stageTimes) {
        await page.evaluate(t => Dream.test.seek(t), t);
        await page.locator('.stage').screenshot({ path: path.join(output, id + '-' + t + '.png') });
      }
      if (id === '64k') {
        await page.evaluate(() => Dream.test.seek(42));
        await page.screenshot({ path: path.join(output, 'desktop.png') });
      }
      report.productions[id] = { size, budget: JAGGED_BUDGET[id], renderWaitMs: audioMs, changes, endContrast: state.captions[2], cueEnergy: state.cueEnergy };
      console.log(id + ': ' + size + ' bytes · phases, copy, music, loaded assets OK');
    } finally {
      await page.close();
    }
  }

  // Check responsive geometry and a crisp final image in the fourth cut at representative widths.
  for (const [w, h] of [[390, 844], [820, 1180], [1680, 720], [1024, 768]]) {
    const page = await browser.newPage({ viewport: { width: w, height: h }, reducedMotion: 'reduce' });
    try {
      await page.goto(pageUrl({ anim: '60k' }));
      await page.waitForFunction(() => window.Dream?.state.mode === 'still', null, { timeout: 12000 });
      assert(await page.evaluate(() => document.documentElement.scrollWidth === innerWidth), w + ' no overflow');
      assert.equal(await page.locator('#progress').evaluate(p => p.value), 60);
      assert.equal(await page.evaluate(() => Dream.dbg().playing), false, w + ' reduced motion is silent');
      if (w === 390 || w === 820) await page.screenshot({ path: path.join(output, w === 390 ? 'mobile.png' : 'tablet.png'), fullPage: true });
    } finally {
      await page.close();
    }
  }

  const page = await browser.newPage({ viewport: { width: 920, height: 640 } });
  try {
    await page.goto(pageUrl({ anim: '4k' }));
    await page.waitForFunction(() => window.Dream?.dbg().playing, null, { timeout: 12000 });
    await page.getByRole('switch', { name: 'Ton' }).click();
    await page.waitForFunction(() => Dream.dbg().muted);
    await page.keyboard.press('m');
    await page.waitForFunction(() => Dream.dbg().playing && !Dream.dbg().muted);
    await page.keyboard.press('Escape');
    await page.waitForFunction(() => document.documentElement.classList.contains('done') && !Dream.dbg().playing);
    assert.equal(await page.locator('#cta a').getAttribute('href'), 'mailto:info@dreambau.com');
    await page.locator('[data-cut="60k"]').click();
    await page.waitForURL(pageUrl({ anim: '60k' }));
    await page.waitForFunction(() => window.Dream?.state.mode === 'play' && Dream.state.id === '60k');
  } finally {
    await page.close();
  }

  for (const setting of ['no-js', 'no-webgl', 'random']) {
    const p = await browser.newPage({ javaScriptEnabled: setting !== 'no-js', reducedMotion: 'reduce' });
    try {
      if (setting === 'no-webgl') await p.addInitScript(() => {
        const original = HTMLCanvasElement.prototype.getContext;
        HTMLCanvasElement.prototype.getContext = function (kind, ...options) {
          return kind === 'webgl2' ? null : original.call(this, kind, ...options);
        };
      });
      await p.goto(pageUrl(setting === 'random' ? {} : { anim: '16k' }));
      assert(await p.getByRole('heading', { level: 1, name: 'Jagged Intelligence' }).isVisible());
      if (setting === 'random') {
        await p.waitForFunction(() => window.Dream?.state.mode === 'still');
        assert(ids.includes(await p.locator('[aria-current]').getAttribute('data-cut')));
      } else {
        if (setting === 'no-webgl') await p.waitForFunction(() => document.documentElement.classList.contains('static'));
        assert(await p.locator('.still-art').isVisible(), setting + ' fallback is visible');
        assert.equal(await p.locator('#c').isVisible(), false);
      }
    } finally {
      await p.close();
    }
  }

  // Optional real-time acceptance for all four scores, in one Chromium process at small viewports.
  if (process.argv.includes('--complete')) {
    const runs = await Promise.allSettled(ids.map(async id => {
      const p = await browser.newPage({ viewport: { width: 640, height: 520 } });
      try {
        await p.goto(pageUrl({ anim: id, q: .43 }));
        await p.waitForFunction(() => window.Dream?.dbg().playing, null, { timeout: 12000 });
        await p.waitForFunction(() => Dream.dbg().T >= 60, null, { timeout: 78000 });
        assert(await p.locator('#cta').isVisible());
        assert.equal(await p.evaluate(() => Dream.state.error), null);
        console.log(id + ': full 60-second playback reached the ending');
      } finally { await p.close(); }
    }));
    for (const run of runs) if (run.status === 'rejected') throw run.reason;
    report.completePlayback = true;
  }
} finally {
  await browser.close();
}

const blocked = await launch({ autoplay: false, extra: ['--autoplay-policy=document-user-activation-required'] });
try {
  const p = await blocked.newPage({ viewport: { width: 920, height: 640 } });
  await p.goto(pageUrl({ anim: '60k' }));
  await p.waitForFunction(() => window.Dream?.state.audio === 'blocked' && Dream.state.T > .4, null, { timeout: 12000 });
  assert.equal(await p.evaluate(() => Dream.dbg().playing), false);
  await p.getByRole('switch', { name: 'Ton' }).click();
  await p.waitForFunction(() => Dream.dbg().playing && Dream.dbg().audioT > 0, null, { timeout: 12000 });
  await p.close();
} finally {
  await blocked.close();
}
report.behavior = 'Strict CSP, byte parity, responsive layout, reduced motion, fallback, variant choice, mute, Esc, autoplay allow/block';
await fs.writeFile(path.join(output, 'report.json'), JSON.stringify(report, null, 2) + '\n');
console.log(report.behavior + ' OK');
