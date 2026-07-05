import { expect, test } from '@playwright/test'

test('router navigation works', async ({ page }) => {
  await page.goto('http://127.0.0.1:5176/?mockMode=1&mockReset=1')
  await expect(page).toHaveURL(/^http:\/\/127\.0\.0\.1:5176\/\?/)

  await page.goto('http://127.0.0.1:5176/devices?mockMode=1&mockReset=1')
  await expect(page).toHaveURL(/\/devices\?/)
  await expect(page.getByRole('heading', { name: 'Devices' })).toBeVisible()

  await page.goto('http://127.0.0.1:5176/devices/new?mockMode=1')
  await expect(page).toHaveURL(/\/devices\/new\?/)
  await expect(page.getByRole('heading', { name: 'Create device' })).toBeVisible()
  await expect(page.getByRole('textbox', { name: 'Name', exact: true })).toBeVisible()
})
