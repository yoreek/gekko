import { expect, test } from '@playwright/test'

test('check device detail form overflow', async ({ page }) => {
  await page.goto('http://127.0.0.1:5176/devices/670845755?mockMode=1', { waitUntil: 'networkidle' })
  await page.waitForTimeout(1500)

  await expect(page.locator('.v-card').first()).toBeVisible()

  const overflowingElements = await page.locator('*').evaluateAll(els =>
    els
      .filter(el => !el.className.toString().includes('v-selection-control') && el.scrollWidth > el.clientWidth + 1)
      .map(el => ({
        tag: el.tagName,
        class: el.className,
        scrollWidth: el.scrollWidth,
        clientWidth: el.clientWidth,
      })),
  )

  expect(overflowingElements).toEqual([])
})
