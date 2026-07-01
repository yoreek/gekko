import { expect, test } from '@playwright/test'

test.beforeEach(({ page }) => {
  page.on('pageerror', err => {
    console.log('[PAGE_ERROR]', err.message)
  })
})

test('v2 devices list renders and links to detail', async ({ page }) => {
  await page.goto('/v2/devices?mockMode=1&mockReset=1')
  await expect(page.getByRole('heading', { name: 'Devices', level: 6 }).or(page.locator('.text-h6'))).toBeVisible()

  const items = page.locator('.v-list-item')
  await expect(items.first()).toBeVisible()

  await items.first().click()
  await expect(page).toHaveURL(/\/v2\/devices\/\d+$/)
  await expect(page.locator('input').first()).toBeVisible()
})

test('v2 device create flow: create a dummy device', async ({ page }) => {
  await page.goto('/v2/devices/new?mockMode=1&mockReset=1')

  await page.locator('input').first().fill('V2 Test Dummy')
  await page.getByRole('button', { name: 'Save' }).click()

  await expect(page).toHaveURL(/\/v2\/devices\/\d+$/)
  await expect(page.locator('input').first()).toHaveValue('V2 Test Dummy')
})

test('v2 gpio_switch detail shows type-specific fields', async ({ page }) => {
  await page.goto('/v2/devices?mockMode=1&mockReset=1')

  const gpioItem = page.locator('.v-list-item', { hasText: 'GPIO switch' }).first()
  await gpioItem.click()

  await expect(page).toHaveURL(/\/v2\/devices\/\d+$/)
  await expect(page.getByLabel('GPIO pin')).toBeVisible()
  await expect(page.getByLabel('Inverted')).toBeVisible()
})
