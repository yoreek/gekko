import { test } from '@playwright/test'

test('check device detail form overflow', async ({ page }) => {
  await page.goto('http://127.0.0.1:5176/v2/devices/670845755?mockMode=1', { waitUntil: 'networkidle' })
  await page.waitForTimeout(1500)
  
  // Check for overflow
  const card = page.locator('v-card').first()
  const cardBox = await card.boundingBox()
  console.log(`Card bounds: ${cardBox?.x}, ${cardBox?.y}, width=${cardBox?.width}, height=${cardBox?.height}`)
  
  // Check for overflow on any element
  const elements = await page.locator('*').evaluateAll(els =>
    els
      .filter(el => {
        const style = window.getComputedStyle(el)
        const overflow = style.overflow || style.overflowX
        const scrollWidth = el.scrollWidth
        const clientWidth = el.clientWidth
        return scrollWidth > clientWidth
      })
      .slice(0, 5)
      .map(el => ({
        tag: el.tagName,
        class: el.className,
        scrollWidth: el.scrollWidth,
        clientWidth: el.clientWidth,
        overflow: window.getComputedStyle(el).overflow
      }))
  )
  
  console.log('Elements with overflow:', JSON.stringify(elements, null, 2))
  
  await page.screenshot({ path: '/tmp/form-detail-check.png' })
})
