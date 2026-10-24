// End-to-end test using Node 22+ and an installed Chrome. No npm dependencies.
// node tests/browser_test.mjs /absolute/path/to/offscreen_renderer [artifact-directory]
import assert from 'node:assert/strict';
import { spawn } from 'node:child_process';
import { once } from 'node:events';
import { mkdtemp, mkdir, readFile, rm, writeFile } from 'node:fs/promises';
import { createServer } from 'node:http';
import { tmpdir } from 'node:os';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { setTimeout as delay } from 'node:timers/promises';

const root = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const executable = process.argv[2] ? resolve(process.argv[2]) : resolve(root, '../../build/webrtc/offscreen_renderer');
const artifacts = resolve(process.argv[3] || dirname(executable));
const chromePath = process.env.CHROME_BIN || (process.platform === 'darwin'
    ? '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome' : 'google-chrome');
const profile = await mkdtemp(join(tmpdir(), 'offscreen-chrome-'));
const html = await readFile(join(root, 'index.html'));
const http = createServer((request, response) => {
    response.writeHead(200, { 'Content-Type': 'text/html; charset=utf-8' });
    response.end(html);
});
let server, chrome, cdp;
let serverError, chromeError;
let serverLog = '', chromeLog = '';
const pageErrors = [];

async function waitFor(check, description, timeout = 20000) {
    const deadline = Date.now() + timeout;
    while (Date.now() < deadline) {
        const result = await check();
        if (result) return result;
        await delay(100);
    }
    throw new Error(`Timed out: ${description}`);
}

async function stop(child, signal = 'SIGTERM') {
    if (!child?.pid || child.exitCode !== null || child.signalCode !== null) return;
    const exited = once(child, 'exit');
    child.kill(signal);
    if (!(await Promise.race([exited.then(() => true), delay(5000).then(() => false)]))) {
        child.kill('SIGKILL');
        await exited;
    }
}

class DevTools {
    constructor(socket) {
        this.socket = socket;
        this.sequence = 0;
        this.pending = new Map();
        socket.addEventListener('message', event => {
            const message = JSON.parse(event.data);
            if (message.method === 'Runtime.exceptionThrown') pageErrors.push(message.params.exceptionDetails.text);
            const entry = this.pending.get(message.id);
            if (entry) {
                this.pending.delete(message.id);
                clearTimeout(entry.timer);
                if (message.error) entry.reject(new Error(JSON.stringify(message.error)));
                else entry.resolve(message.result);
            }
        });
    }
    call(method, params = {}) {
        const id = ++this.sequence;
        return new Promise((resolve, reject) => {
            const timer = setTimeout(() => { this.pending.delete(id); reject(new Error(`CDP timeout: ${method}`)); }, 15000);
            this.pending.set(id, { resolve, reject, timer });
            this.socket.send(JSON.stringify({ id, method, params }));
        });
    }
    async evaluate(expression) {
        const response = await this.call('Runtime.evaluate', { expression, returnByValue: true, awaitPromise: true });
        if (response.exceptionDetails) throw new Error(JSON.stringify(response.exceptionDetails));
        return response.result.value;
    }
}

try {
    await mkdir(artifacts, { recursive: true });
    const serverArgs = ['--port', '0'];
    if (process.env.OFFSCREEN_TEST_ENCODER)
        serverArgs.push('--encoder', process.env.OFFSCREEN_TEST_ENCODER);
    if (process.env.OFFSCREEN_TEST_NVENC_INPUT)
        serverArgs.push('--nvenc-input', process.env.OFFSCREEN_TEST_NVENC_INPUT);
    server = spawn(executable, serverArgs, { stdio: ['ignore', 'pipe', 'pipe'] });
    server.on('error', error => { serverError = error; });
    server.stdout.on('data', bytes => { serverLog += bytes; });
    server.stderr.on('data', bytes => { serverLog += bytes; });
    const port = await waitFor(() => {
        if (serverError) throw serverError;
        if (server.exitCode !== null) throw new Error(`Server exited: ${serverLog}`);
        return serverLog.match(/Signaling: ws:\/\/127\.0\.0\.1:(\d+)/)?.[1];
    }, 'server initialization');
    if (process.env.OFFSCREEN_TEST_NVENC_INPUT && !serverLog.includes('NVENC'))
        throw new Error(`Expected NVENC hardware backend: ${serverLog}`);
    http.listen(0, '127.0.0.1');
    await once(http, 'listening');
    chrome = spawn(chromePath, ['--headless=new', '--no-first-run', '--no-default-browser-check',
        '--disable-background-networking', '--disable-component-update', '--remote-debugging-port=0',
        '--autoplay-policy=no-user-gesture-required', '--window-size=1024,1100', `--user-data-dir=${profile}`, 'about:blank'],
        { stdio: ['ignore', 'pipe', 'pipe'] });
    chrome.on('error', error => { chromeError = error; });
    chrome.stderr.on('data', bytes => { chromeLog += bytes; });
    const debugPort = await waitFor(() => {
        if (chromeError) throw chromeError;
        if (chrome.exitCode !== null) throw new Error(`Chrome exited: ${chromeLog}`);
        return chromeLog.match(/DevTools listening on ws:\/\/127\.0\.0\.1:(\d+)/)?.[1];
    }, 'Chrome DevTools');
    const pageUrl = `http://127.0.0.1:${http.address().port}`;
    const target = await (await fetch(`http://127.0.0.1:${debugPort}/json/new?${encodeURIComponent(pageUrl)}`, { method: 'PUT' })).json();
    const ws = new WebSocket(target.webSocketDebuggerUrl);
    await new Promise((resolve, reject) => { ws.onopen = resolve; ws.onerror = reject; });
    cdp = new DevTools(ws);
    await cdp.call('Runtime.enable');
    await waitFor(() => cdp.evaluate(`document.readyState === 'complete' && !!document.getElementById('connectButton')`), 'client page');
    await cdp.evaluate(`document.getElementById('serverUrl').value = 'ws://127.0.0.1:${port}'; document.getElementById('connectButton').click()`);
    const connected = `video.videoWidth === 640 && video.videoHeight === 480 && video.readyState >= 2 && peer?.connectionState === 'connected' && controls?.readyState === 'open'`;
    await waitFor(() => cdp.evaluate(connected), 'decoded H.264 video and controls channel');
    const statsExpression = `(async () => {
        const reports = Array.from((await peer.getStats()).values());
        const inbound = reports.find(r => r.type === 'inbound-rtp' && r.kind === 'video');
        const codec = reports.find(r => r.id === inbound?.codecId);
        return { frames: inbound?.framesDecoded, timestamp: inbound?.timestamp,
                 codec: codec?.mimeType, width: video.videoWidth, height: video.videoHeight };
    })()`;
    const before = await cdp.evaluate(statsExpression);
    await delay(4000);
    const after = await cdp.evaluate(statsExpression);
    const fps = (after.frames - before.frames) * 1000 / (after.timestamp - before.timestamp);
    assert.equal(after.codec, 'video/H264');
    assert.ok(fps >= 45, `Decoded frame rate too low: ${fps}`);
    const pixels = await cdp.evaluate(`(() => {
        const canvas = document.createElement('canvas'); canvas.width = 640; canvas.height = 480;
        const context = canvas.getContext('2d'); context.drawImage(video, 0, 0, 640, 480);
        const data = context.getImageData(0, 0, 640, 480).data;
        let colorful = 0;
        for (let i = 0; i < data.length; i += 4) if (Math.max(data[i], data[i+1], data[i+2]) > 70) colorful++;
        return { corner: Array.from(data.slice(0, 3)), colorful };
    })()`);
    assert.ok(pixels.corner.every(value => value < 45), 'Clear color was not decoded correctly');
    assert.ok(pixels.colorful > 1000, 'Decoded frame does not contain the triangle');
    const bounds = await cdp.evaluate(`(() => { const r = video.getBoundingClientRect(); return { x:r.x+150, y:r.y+150 }; })()`);
    await cdp.call('Input.dispatchMouseEvent', { type: 'mousePressed', x: bounds.x, y: bounds.y, button: 'left', clickCount: 1 });
    await cdp.call('Input.dispatchMouseEvent', { type: 'mouseMoved', x: bounds.x + 80, y: bounds.y + 35, button: 'left', buttons: 1 });
    await cdp.call('Input.dispatchMouseEvent', { type: 'mouseReleased', x: bounds.x + 80, y: bounds.y + 35, button: 'left', clickCount: 1 });
    const pose = await waitFor(() => cdp.evaluate(`cameraStatus.dataset.yaw ? { yaw: Number(cameraStatus.dataset.yaw), pitch: Number(cameraStatus.dataset.pitch) } : null`), 'server camera acknowledgement');
    assert.ok(pose.yaw > 0.1 && pose.pitch > 0.05, 'Drag did not update the server camera');
    const screenshot = await cdp.call('Page.captureScreenshot', { format: 'png' });
    await writeFile(join(artifacts, 'browser-test.png'), Buffer.from(screenshot.data, 'base64'));
    await cdp.evaluate(`document.getElementById('disconnectButton').click()`);
    await delay(250);
    await cdp.evaluate(`document.getElementById('connectButton').click()`);
    await waitFor(() => cdp.evaluate(connected), 'reconnection and keyframe recovery');
    await cdp.evaluate(`document.getElementById('disconnectButton').click()`);
    await stop(server, 'SIGINT');
    assert.equal(server.exitCode, 0, 'Server did not shut down cleanly');
    assert.ok(!serverLog.includes('[validation error]'), serverLog);
    assert.deepEqual(pageErrors, [], 'Uncaught browser exceptions');
    console.log(JSON.stringify({ codec: after.codec, decodedFps: fps, camera: pose, pixels,
        reconnect: 'passed', shutdown: 'passed', validationErrors: 0 }, null, 2));
} catch (error) {
    console.error(serverLog);
    console.error(error);
    process.exitCode = 1;
} finally {
    if (cdp) cdp.socket.close();
    await stop(chrome);
    await stop(server, 'SIGINT');
    http.close();
    await writeFile(join(artifacts, 'browser-test-server.log'), serverLog).catch(() => {});
    await rm(profile, { recursive: true, force: true, maxRetries: 3, retryDelay: 100 }).catch(() => {});
}
