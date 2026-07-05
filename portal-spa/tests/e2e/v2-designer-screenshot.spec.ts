import { test } from '@playwright/test'

test('designer - take screenshot for inspection', async ({ page }) => {
  await page.goto('http://127.0.0.1:5176/devices/670845755/design?mockMode=1', { waitUntil: 'networkidle' })
  await page.waitForTimeout(2000)
  await page.screenshot({ path: '/tmp/designer-full.png', fullPage: false })
  console.log('Screenshot saved')
})
