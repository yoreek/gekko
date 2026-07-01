import { test } from '@playwright/test'

test('screenshot device page', async ({ page }) => {
  await page.goto('http://127.0.0.1:5176/v2/devices/670845755?mockMode=1', { waitUntil: 'networkidle' })
  await page.waitForTimeout(1000)
  
  // Take screenshot
  await page.screenshot({ path: '/tmp/device-page.png' })
  
  // Also save HTML for inspection
  const html = await page.content()
  const fs = require('fs')
  fs.writeFileSync('/tmp/device-page.html', html)
  
  console.log('Screenshots saved')
})
