import { expect, test } from '@playwright/test'
import { Buffer } from 'node:buffer'

const mockPath = '/devices?mockMode=1&mockReset=1'

test('OLED designer persists typed layout widgets in mock mode', async ({ page }) => {
  await page.goto(mockPath)

  await page.getByRole('row').filter({ hasText: 'OLED Display' }).click()
  await page.getByRole('link', { name: 'OLED display designer' }).click()

  await page.getByRole('button', { name: 'Text', exact: true }).click()
  await page.getByRole('textbox', { name: 'Text', exact: true }).fill('Hello OLED')
  await page.getByRole('button', { name: 'Save', exact: true }).click()

  await page.reload()
  await page.getByText('Hello OLED').click()
  await expect(page.getByRole('textbox', { name: 'Text', exact: true })).toHaveValue('Hello OLED')
})

test('OLED designer preview resolves placeholder samples and filters', async ({ page }) => {
  await page.goto(mockPath)

  await page.getByRole('row').filter({ hasText: 'OLED Display' }).click()
  await page.getByRole('link', { name: 'OLED display designer' }).click()

  await page.getByRole('button', { name: 'Text', exact: true }).click()
  await page.getByRole('textbox', { name: 'Text', exact: true }).fill('{{dev.670845750.state | upper}}')

  await expect(page.getByText('Fits', { exact: true })).toBeVisible()
  await expect(page.getByText('Metric is unavailable now, but the widget can be saved.')).toHaveCount(0)
  await expect(page.getByText('Metric placeholder syntax is invalid.')).toHaveCount(0)
})

test('OLED designer imports, resizes, saves, and reloads bitmap widgets', async ({ page }) => {
  await page.goto(mockPath)
  await page.getByRole('row').filter({ hasText: 'OLED Display' }).click()
  await page.getByRole('link', { name: 'OLED display designer' }).click()

  await page.getByRole('button', { name: 'Bitmap', exact: true }).click()

  const bitmapFile = Buffer.from('<svg xmlns="http://www.w3.org/2000/svg" width="4" height="4"><rect width="4" height="4" fill="white"/><rect x="1" y="1" width="2" height="2" fill="black"/></svg>')
  await page.locator('input[type="file"]').setInputFiles({
    name: 'bitmap.svg',
    mimeType: 'image/svg+xml',
    buffer: bitmapFile,
  })

  const widthField = page.getByLabel('Width', { exact: true })
  const heightField = page.getByLabel('Height', { exact: true })
  await widthField.fill('8')
  await widthField.press('Tab')
  await heightField.fill('8')
  await heightField.press('Tab')
  await page.waitForTimeout(300)
  await expect(widthField).toHaveValue('8')
  await expect(heightField).toHaveValue('8')

  await page.getByRole('button', { name: 'Save', exact: true }).click()

  await page.reload()
  await page.locator('.v-list-item-title', { hasText: 'Bitmap' }).first().click()
  await expect(page.getByLabel('Width', { exact: true })).toHaveValue('16')
  await expect(page.getByLabel('Height', { exact: true })).toHaveValue('16')
})
