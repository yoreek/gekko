import { test } from '@playwright/test'

test('check field order and translations', async ({ page }) => {
  await page.goto('http://127.0.0.1:5176/v2/devices/670845755?mockMode=1', { waitUntil: 'networkidle' })
  await page.waitForTimeout(1500)
  await page.screenshot({ path: '/tmp/device-fields-fixed.png' })
})
