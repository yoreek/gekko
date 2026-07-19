import { expect, test } from '@playwright/test'

test('journal navigation opens the device event page in mock mode', async ({ page }) => {
  await page.goto('/?mockMode=1&mockReset=1')

  await page.getByRole('button', { name: 'Open menu' }).click()
  await page.getByRole('link', { name: 'Device events' }).click()

  await expect(page).toHaveURL(/device-events/)
  await expect(page.getByRole('heading', { name: 'Device events' })).toBeVisible()

  // The journal table paginates (10 rows/page) and the growing set of seed mock devices can push
  // "Aquarium Lamp" past the first page, so filter by name instead of relying on default sort order.
  await page.getByLabel('Name').fill('Aquarium Lamp')

  const row = page.getByRole('row', { name: /Aquarium Lamp/ })
  await expect(row).toBeVisible()
  await expect(row.locator('.v-chip__content')).toHaveText('Snapshot')
})
