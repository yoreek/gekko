import { test, expect } from '@playwright/test'

test('designer inspector panel - no content overflow', async ({ page }) => {
  await page.goto('http://127.0.0.1:5176/devices/670845755/design?mockMode=1&mockReset=1', { waitUntil: 'networkidle' })
  await page.waitForTimeout(2000)

  // Get inspector panel
  const inspector = page.locator('text=Inspector').first().locator('xpath=ancestor::*[contains(@class, "v-col")]').first()
  
  // Check if inspector exists
  await expect(inspector).toBeVisible()
  console.log('✓ Inspector panel visible')
  
  // Check for horizontal scroll (overflow)
  const inspectorBox = await inspector.boundingBox()
  const sheet = inspector.locator('.v-sheet').first()
  const sheetBox = await sheet.boundingBox()
  
  if (!inspectorBox || !sheetBox) {
    console.log('⚠ Could not measure bounds, taking screenshot')
    await page.screenshot({ path: '/tmp/designer-inspector.png' })
    return
  }
  
  const isOverflowing = sheetBox.width > inspectorBox.width
  console.log(`Inspector width: ${inspectorBox.width}px`)
  console.log(`Sheet width: ${sheetBox.width}px`)
  console.log(`Overflowing: ${isOverflowing}`)
  
  await page.screenshot({ path: '/tmp/designer-inspector-final.png' })
  
  expect(isOverflowing).toBe(false)
})
