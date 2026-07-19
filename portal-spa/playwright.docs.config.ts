import { defineConfig, devices } from '@playwright/test'

// Screenshot capture for the documentation site (docs-site/). Run on demand
// with `pnpm shots:docs` — not part of CI or the e2e suite.
export default defineConfig({
  testDir: './tests/docs-shots',
  timeout: 60000,
  expect: {
    timeout: 5000,
  },
  use: {
    baseURL: 'http://127.0.0.1:5176',
    viewport: { width: 1280, height: 800 },
    deviceScaleFactor: 2,
    colorScheme: 'light',
    locale: 'en-US',
  },
  webServer: {
    command: 'pnpm dev --host 127.0.0.1 --port 5176',
    url: 'http://127.0.0.1:5176',
    reuseExistingServer: !process.env.CI,
    timeout: 30000,
  },
  projects: [
    {
      name: 'chromium',
      use: { ...devices['Desktop Chrome'], viewport: { width: 1280, height: 800 }, deviceScaleFactor: 2 },
    },
  ],
})
