import { test } from '@playwright/test'

test('final ssd1306 form with i2c picker', async ({ page }) => {
  await page.goto('http://127.0.0.1:5176/v2/devices/670845755?mockMode=1', { waitUntil: 'networkidle' })
  await page.waitForTimeout(1500)
  
  // Check that scan button exists
  const scanBtn = page.locator('button:has-text("Scan"), button:has-text("scan")')
  const hasScanBtn = await scanBtn.isVisible({ timeout: 2000 }).catch(() => false)
  console.log(hasScanBtn ? '✓ Scan button visible' : '✗ Scan button not found')
  
  await page.screenshot({ path: '/tmp/ssd1306-with-scanner.png' })
})
