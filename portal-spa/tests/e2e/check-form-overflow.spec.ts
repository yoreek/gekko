import { expect, test } from '@playwright/test'

test('check device detail form overflow', async ({ page }) => {
  await page.goto('http://127.0.0.1:5176/devices/670845755?mockMode=1', { waitUntil: 'networkidle' })
  await page.waitForTimeout(1500)

  await expect(page.locator('.v-card').first()).toBeVisible()

  const overflowingElements = await page.locator('*').evaluateAll(els =>
    els
      .filter(el => {
        const className = el.className.toString()
        // v-badge intentionally floats its indicator past the anchor icon's box (it decorates a
        // corner, e.g. the notification bell's unread count) - that's a deliberate overlap, not a
        // layout bug like clipped text/inputs. Any ancestor wrapper of the badge (e.g. the button's
        // v-btn__content) inherits the same "overflow" from that child and must be excluded too.
        if (className.includes('v-selection-control') || className.includes('v-badge') || el.querySelector('[class*="v-badge"]')) {
          return false
        }
        return el.scrollWidth > el.clientWidth + 1
      })
      .map(el => ({
        tag: el.tagName,
        class: el.className,
        scrollWidth: el.scrollWidth,
        clientWidth: el.clientWidth,
      })),
  )

  expect(overflowingElements).toEqual([])
})
