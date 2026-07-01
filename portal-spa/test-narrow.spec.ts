import { test, expect } from '@playwright/test'

test('check narrow viewport 1000px', async ({ browser }) => {
  const context = await browser.newContext({ viewport: { width: 1000, height: 900 } })
  const page = await context.newPage()
  await page.goto('http://127.0.0.1:5176/v2/devices/670845755/design?mockMode=1', { waitUntil: 'networkidle' })
  await page.waitForTimeout(2000)
  await page.screenshot({ path: '/tmp/designer-1000px.png' })
  console.log('Screenshot saved')
  await context.close()
})
