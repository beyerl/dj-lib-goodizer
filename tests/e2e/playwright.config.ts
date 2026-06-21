import { defineConfig, devices } from '@playwright/test';

// The WebAssembly app (or the local mock) is served by serve.cjs. Point it at
// the real build in CI via WASM_DIR; locally it defaults to ./mock, a faithful
// JS stand-in of the window.__djState / window.__djCmd contract.
const PORT = Number(process.env.PORT || 8080);
const BASE_URL = `http://127.0.0.1:${PORT}`;

export default defineConfig({
  testDir: './tests',
  fullyParallel: false,
  workers: 1,
  forbidOnly: !!process.env.CI,
  retries: process.env.CI ? 1 : 0,
  // The WASM module can take a while to download + instantiate on a cold runner.
  timeout: 60_000,
  expect: { timeout: 30_000 },
  reporter: [['list'], ['html', { open: 'never' }]],
  use: {
    baseURL: BASE_URL,
    trace: 'on-first-retry',
    screenshot: 'only-on-failure',
  },
  projects: [
    { name: 'chromium', use: { ...devices['Desktop Chrome'] } },
  ],
  webServer: {
    command: 'node serve.cjs',
    url: `${BASE_URL}/index.html`,
    reuseExistingServer: !process.env.CI,
    timeout: 120_000,
    env: {
      PORT: String(PORT),
      WASM_DIR: process.env.WASM_DIR || 'mock',
    },
  },
});
