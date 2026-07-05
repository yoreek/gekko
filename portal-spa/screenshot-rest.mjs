import { chromium } from '@playwright/test'

const routes = ['overview', 'wifi', 'device-events', 'system', 'ota', 'devices']
const browser = await chromium.launch()
const page = await browser.newPage({ viewport: { width: 1280, height: 900 } })
for (const r of routes) {
  await page.goto(`http://127.0.0.1:5176/v2/${r}?mockMode=1&mockReset=1`)
  await page.waitForTimeout(900)
  await page.screenshot({ path: `/tmp/${r}.png`, fullPage: true })
  console.log('saved', r)
}
await browser.close()
