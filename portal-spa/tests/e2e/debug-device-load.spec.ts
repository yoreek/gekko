import { expect, test } from '@playwright/test'

test('debug device loading', async ({ page }) => {
  const errors: string[] = []
  page.on('console', msg => {
    if (msg.type() === 'error') errors.push(msg.text())
  })

  await page.goto('http://127.0.0.1:5176/devices/670845748?mockMode=1&mockReset=1', { waitUntil: 'networkidle' })

  await expect(page.getByRole('heading', { name: 'Aquarium Lamp' })).toBeVisible()

  const html = await page.innerHTML('body')
  expect(html).toContain('Aquarium Lamp')
  expect(errors).toEqual([])
})
