import { expect, test } from '@playwright/test'

test('Device create page loads', async ({ page }) => {
  const errors: string[] = []
  page.on('pageerror', err => errors.push(err.message))

  await page.goto('http://127.0.0.1:5176/devices/new?mockMode=1&mockReset=1', { waitUntil: 'domcontentloaded' })

  await expect(page.getByRole('heading', { name: 'Create device' })).toBeVisible()
  await expect(page.getByRole('textbox', { name: 'Name', exact: true })).toBeVisible()
  expect(errors).toEqual([])
})

test('Device detail page loads', async ({ page }) => {
  const errors: string[] = []
  page.on('pageerror', err => errors.push(err.message))

  await page.goto('http://127.0.0.1:5176/devices/670845748?mockMode=1&mockReset=1', { waitUntil: 'domcontentloaded' })

  await expect(page.getByRole('heading', { name: 'Aquarium Lamp' })).toBeVisible()
  expect(errors).toEqual([])
})
