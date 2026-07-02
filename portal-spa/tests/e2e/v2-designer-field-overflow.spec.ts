import { test, expect } from '@playwright/test'

test('designer inspector - Widget type field not cut off', async ({ page }) => {
  await page.goto('http://127.0.0.1:5176/v2/devices/670845755/design?mockMode=1&mockReset=1', { waitUntil: 'networkidle' })
  await page.waitForTimeout(1000)

  // Select the first widget in the Layers panel so the inspector renders its fields
  await page.locator('.v-sheet', { hasText: 'Layers' }).locator('.v-list-item').first().click()
  await page.waitForTimeout(800)

  // Find Widget type label and its enclosing Vuetify select (not the hidden native <select>)
  const label = page.locator('label', { hasText: 'Widget type' }).last()
  const select = page.locator('.v-select', { has: label }).first()

  // Check if label is visible
  await expect(label).toBeVisible({ timeout: 5000 })
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
