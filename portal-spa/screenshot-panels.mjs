import { chromium } from '@playwright/test'

const browser = await chromium.launch()
const page = await browser.newPage({ viewport: { width: 1280, height: 900 } })
await page.goto('http://127.0.0.1:5176/v2/panels?mockMode=1&mockReset=1')
await page.waitForTimeout(1000)
await page.screenshot({ path: '/tmp/panels.png', fullPage: true })
console.log('saved')
await browser.close()
