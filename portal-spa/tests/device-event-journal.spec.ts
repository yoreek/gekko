import { expect, test } from '@playwright/test'

test('journal navigation opens the device event page in mock mode', async ({ page }) => {
  await page.goto('/?mockMode=1&mockReset=1')

  await page.getByRole('button', { name: 'Open menu' }).click()
  await page.getByRole('link', { name: 'Device events' }).click()

  await expect(page).toHaveURL(/device-events/)
  await expect(page.getByRole('heading', { name: 'Device event journal' })).toBeVisible()
  await expect(page.locator('.journal-table__row').filter({ hasText: 'Aquarium Lamp' })).toBeVisible()
  await expect(
    page
      .locator('.journal-table__row')
      .filter({ hasText: 'Aquarium Lamp' })
      .locator('td:nth-child(6) .v-chip__content'),
  ).toHaveText('Snapshot')
})
