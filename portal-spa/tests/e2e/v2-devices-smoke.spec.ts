import { expect, test } from '@playwright/test'

test.beforeEach(({ page }) => {
  page.on('pageerror', err => {
    console.log('[PAGE_ERROR]', err.message)
  })
})

test('v2 devices list renders and links to detail', async ({ page }) => {
  await page.goto('/devices?mockMode=1&mockReset=1')
  await expect(page.getByRole('heading', { name: 'Devices' })).toBeVisible()

  const rows = page.getByRole('row')
  await expect(rows.nth(1)).toBeVisible()

  await rows.nth(1).click()
  await expect(page).toHaveURL(/\/devices\/\d+$/)
  await expect(page.locator('input').first()).toBeVisible()
})

test('v2 device create flow: create a dummy device', async ({ page }) => {
  await page.goto('/devices/new?mockMode=1&mockReset=1')

  await page.locator('input').first().fill('V2 Test Dummy')
  await page.getByRole('button', { name: 'Save' }).click()

  // Create redirects to the devices list (not the new device's detail page) so "back" from the
  // list never returns to a stale create form - filter by name (the list paginates and the new
  // device's id-based sort position isn't guaranteed to land on the first page) then open it.
  await expect(page).toHaveURL(/\/devices$/)
  await page.getByLabel('Search by name').fill('V2 Test Dummy')
  await page.getByRole('row', { name: /V2 Test Dummy/ }).click()
  await expect(page).toHaveURL(/\/devices\/\d+$/)
  await expect(page.locator('input').first()).toHaveValue('V2 Test Dummy')
})

test('v2 gpio_switch detail shows type-specific fields', async ({ page }) => {
  await page.goto('/devices?mockMode=1&mockReset=1')

  const gpioItem = page.getByRole('row', { name: /GPIO switch/ }).first()
  await gpioItem.click()

  await expect(page).toHaveURL(/\/devices\/\d+$/)
  await expect(page.getByLabel('GPIO pin')).toBeVisible()

  // Inverted lives inside the collapsible "Config details" panel
  await page.getByText('Config details').click()
  await expect(page.getByLabel('Inverted')).toBeVisible()
})
