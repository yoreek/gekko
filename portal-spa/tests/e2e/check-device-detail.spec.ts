import { expect, test } from '@playwright/test'

test('check device detail page', async ({ page }) => {
  const pageErrors: string[] = []
  page.on('pageerror', err => pageErrors.push(err.message))

  await page.goto('http://127.0.0.1:5176/devices/670845748?mockMode=1&mockReset=1')

  await expect(page.getByRole('heading', { name: 'Aquarium Lamp' })).toBeVisible()
  await expect(page.getByRole('textbox', { name: 'Name', exact: true })).toHaveValue('Aquarium Lamp')
  await expect(page.getByRole('button', { name: 'Save' })).toBeVisible()
  await expect(page.locator('[role="alert"]')).toHaveCount(0)
  expect(pageErrors).toEqual([])
})
