import { test } from '@playwright/test'

test('check designer page layout', async ({ page }) => {
  await page.goto('http://127.0.0.1:5176/devices/670845755/design?mockMode=1', { waitUntil: 'networkidle' })
  await page.waitForTimeout(2000)
  
  // Check page width and content
  const viewport = page.viewportSize()
  console.log(`Viewport: ${viewport?.width}×${viewport?.height}`)
  
  // Take screenshot
  await page.screenshot({ path: '/tmp/designer-page.png' })
})
