import { test } from '@playwright/test'

test('device 670845755 loads correctly', async ({ page }) => {
  await page.goto('http://127.0.0.1:5176/v2/devices/670845755?mockMode=1', { waitUntil: 'networkidle' })
  await page.waitForTimeout(1000)
  await page.screenshot({ path: '/tmp/device-detail-fixed.png' })
  console.log('Screenshot saved to /tmp/device-detail-fixed.png')
})
