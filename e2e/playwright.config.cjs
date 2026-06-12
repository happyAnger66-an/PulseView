const path = require('node:path');
const { defineConfig } = require('@playwright/test');

const E2E_DIR = __dirname;
const PULSEVIEW_ROOT = path.resolve(E2E_DIR, '..');
const TEST_DATA_DIR = path.join(E2E_DIR, '.data');

module.exports = defineConfig({
  testDir: './tests',
  fullyParallel: false,
  forbidOnly: !!process.env.CI,
  retries: process.env.CI ? 1 : 0,
  workers: 1,
  reporter: [['list'], ['html', { open: 'never' }]],
  timeout: 120_000,
  expect: { timeout: 15_000 },
  use: {
    baseURL: process.env.PULSEVIEW_WEB_BASE ?? 'http://127.0.0.1:8767',
    trace: 'on-first-retry',
  },
  webServer: [
    {
      command: 'bash e2e/scripts/start-backend.sh',
      cwd: PULSEVIEW_ROOT,
      env: {
        ...process.env,
        PULSEVIEW_DATA_DIR: TEST_DATA_DIR,
      },
      url: 'http://127.0.0.1:8080/api/datasource/plugins',
      reuseExistingServer: false,
      timeout: 60_000,
    },
    {
      command: 'npx vite --port 8767 --host',
      cwd: path.join(PULSEVIEW_ROOT, 'fronted'),
      env: {
        ...process.env,
        PROXY: 'http://127.0.0.1:8080',
      },
      url: 'http://127.0.0.1:8767',
      reuseExistingServer: false,
      timeout: 60_000,
    },
  ],
});
