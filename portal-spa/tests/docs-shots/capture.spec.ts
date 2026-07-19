import { mkdirSync } from 'node:fs'
import { dirname, resolve } from 'node:path'
import { fileURLToPath } from 'node:url'
import { test, type Page } from '@playwright/test'

// Captures the screenshots embedded in docs-site/ content pages. Output names
// are referenced from docs-site/src/content/docs/** — keep them in sync.
const outputDir = resolve(dirname(fileURLToPath(import.meta.url)), '../../../docs-site/src/assets/screenshots')

const shots: Array<{ name: string; path: string; fullPage?: boolean }> = [
  { name: 'portal-dashboard', path: '/' },
  { name: 'portal-devices', path: '/devices' },
  { name: 'portal-device-create', path: '/devices/new' },
  { name: 'device-gpio-switch', path: '/devices/670845750' },
  { name: 'device-ds18b20', path: '/devices/670845752' },
  { name: 'device-thermostat', path: '/devices/670845753' },
  { name: 'device-schedule', path: '/devices/670845764' },
  { name: 'device-dosing-pump', path: '/devices/670845772' },
  { name: 'device-analog-composer', path: '/devices/670845792' },
  { name: 'portal-display-designer', path: '/devices/670845755/design' },
  { name: 'portal-wifi', path: '/wifi' },
]

async function openMockPage(page: Page, path: string, resetMock: boolean) {
  const separator = path.includes('?') ? '&' : '?'
  const suffix = resetMock ? 'mockMode=1&mockReset=1' : 'mockMode=1'
  await page.goto(`${path}${separator}${suffix}`, { waitUntil: 'networkidle' })
  await page.waitForTimeout(1500)
}

test('capture ds18b20 history dialog', async ({ page }) => {
  test.setTimeout(180000)
  mkdirSync(outputDir, { recursive: true })
  await page.addInitScript(() => {
    window.localStorage.setItem('gekko.locale', 'en')
  })
  await openMockPage(page, '/', true)
  // The mock simulation drifts sensor values once per second; the history chart
  // needs a minute of samples before it has something to draw.
  await page.waitForTimeout(65000)
  await page.getByText('Water Temperature', { exact: true }).first().click()
  const dialog = page.locator('.v-dialog .v-card').first()
  await dialog.waitFor({ state: 'visible' })
  await page.waitForTimeout(1500)
  await dialog.screenshot({ path: resolve(outputDir, 'device-ds18b20-history.png') })
})

test('capture docs screenshots', async ({ page }) => {
  mkdirSync(outputDir, { recursive: true })
  await page.addInitScript(() => {
    window.localStorage.setItem('gekko.locale', 'en')
  })

  let first = true
  for (const shot of shots) {
    await openMockPage(page, shot.path, first)
    first = false
    await page.screenshot({
      path: resolve(outputDir, `${shot.name}.png`),
      fullPage: shot.fullPage ?? false,
    })
  }
})
