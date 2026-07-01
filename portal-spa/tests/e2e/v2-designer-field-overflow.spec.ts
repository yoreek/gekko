import { test, expect } from '@playwright/test'

test('designer inspector - Widget type field not cut off', async ({ page }) => {
  await page.goto('http://127.0.0.1:5176/v2/devices/670845755/design?mockMode=1', { waitUntil: 'networkidle' })
  await page.waitForTimeout(2000)
  
  // Find Widget type label and select
  const label = page.locator('text=Widget type').first()
  const select = page.locator('[aria-label*="Widget type"], select').first()
  
  // Check if label is visible
  await expect(label).toBeVisible({ timeout: 3000 })
  console.log('✓ Widget type label visible')
  
  // Check if select is visible and not clipped
  await expect(select).toBeVisible()
  const box = await select.boundingBox()
  const parentBox = await select.locator('xpath=ancestor::*[contains(@class, "v-col")]').first().boundingBox()
  
  if (!box || !parentBox) {
    throw new Error('Could not get bounding boxes')
  }
  
  const isClipped = box.x + box.width > parentBox.x + parentBox.width
  
  console.log(`Select bounds: x=${box.x}, width=${box.width}`)
  console.log(`Parent bounds: x=${parentBox.x}, width=${parentBox.width}`)
  console.log(`Clipped: ${isClipped}`)
  
  expect(isClipped).toBe(false)
  console.log('✓ Widget type field fits within bounds')
  
  await page.screenshot({ path: '/tmp/designer-widget-type-check.png' })
})
