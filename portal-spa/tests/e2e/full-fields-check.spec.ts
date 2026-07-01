import { test } from '@playwright/test'

test('device 670845755 with all fields', async ({ page }) => {
  await page.goto('http://127.0.0.1:5176/v2/devices/670845755?mockMode=1', { waitUntil: 'networkidle' })
  await page.waitForTimeout(1500)
  
  const inputs = await page.locator('input, select').count()
  console.log(`Found ${inputs} form fields`)
  
  // Get all labels
  const labels = await page.locator('label').allTextContents()
  console.log('Form fields:')
  labels.forEach(l => {
    const clean = l.trim()
    if (clean && clean.length > 0) console.log(`  - ${clean}`)
  })
  
  await page.screenshot({ path: '/tmp/device-with-all-fields.png' })
})
