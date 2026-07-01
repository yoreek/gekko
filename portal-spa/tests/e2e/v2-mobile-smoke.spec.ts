import { expect, test } from '@playwright/test'

test.describe('v2 mobile viewport (375×812)', () => {
  test.beforeEach(({ page }) => {
    page.setViewportSize({ width: 375, height: 812 })
    page.on('pageerror', err => {
      console.log('[PAGE_ERROR]', err.message)
    })
  })

  test('v2 devices list responsive layout', async ({ page }) => {
    await page.goto('/v2/devices?mockMode=1&mockReset=1', { waitUntil: 'networkidle' })
    await page.waitForLoadState('domcontentloaded')

    // Check that page toolbar exists
    const toolbar = page.locator('[role="heading"]').first()
    await expect(toolbar).toBeVisible({ timeout: 8000 })
  })

  test('v2 device create page form loads on mobile', async ({ page }) => {
    await page.goto('/v2/devices/new?mockMode=1&mockReset=1', { waitUntil: 'networkidle' })
    await page.waitForLoadState('domcontentloaded')

    // Form should be visible on mobile
    const toolbar = page.locator('[role="heading"]').first()
    await expect(toolbar).toBeVisible({ timeout: 5000 })

    // Check that form inputs are present
    const inputs = page.locator('input')
    const count = await inputs.count()
    expect(count).toBeGreaterThan(0)
  })

  test('v2 pages are not fullscreen dialogs', async ({ page }) => {
    // Verify that /v2 routes render as pages, not dialogs
    await page.goto('/v2/devices?mockMode=1&mockReset=1', { waitUntil: 'networkidle' })

    // Check that there are no fullscreen dialogs
    const dialogs = page.locator('[role="dialog"][class*="fullscreen"], .v-dialog--fullscreen')
    await expect(dialogs).toHaveCount(0)
  })
})

test.describe('v2 desktop viewport (1280×720)', () => {
  test.beforeEach(({ page }) => {
    page.setViewportSize({ width: 1280, height: 720 })
    page.on('pageerror', err => {
      console.log('[PAGE_ERROR]', err.message)
    })
  })

  test('v2 devices list loads on desktop', async ({ page }) => {
    await page.goto('/v2/devices?mockMode=1&mockReset=1')

    const heading = page.getByRole('heading', { name: /Devices/i })
    await expect(heading).toBeVisible({ timeout: 5000 })
  })

  test('v2 device create flow works on desktop', async ({ page }) => {
    await page.goto('/v2/devices/new?mockMode=1&mockReset=1')

    const input = page.locator('input').first()
    await expect(input).toBeVisible({ timeout: 5000 })

    await input.fill('Desktop Test Device')

    const saveBtn = page.getByRole('button', { name: /Save/i })
    await expect(saveBtn).toBeVisible()
  })
})
