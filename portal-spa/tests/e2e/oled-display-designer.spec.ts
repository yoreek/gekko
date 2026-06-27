import { expect, test } from '@playwright/test'
import { Buffer } from 'node:buffer'

const mockPath = '/devices?mockMode=1&mockReset=1'

test('OLED designer persists typed layout widgets in mock mode', async ({ page }) => {
  await page.goto(mockPath)

  await page.getByRole('row').filter({ hasText: 'OLED Display' }).click()
  await page.getByRole('button', { name: 'Design display' }).click()

  const designer = page.getByRole('dialog').last()
  await designer.getByRole('button', { name: 'Text', exact: true }).click()
  await designer.getByRole('textbox', { name: 'Text', exact: true }).fill('Hello OLED')
  await designer.getByRole('button', { name: 'Save', exact: true }).click()

  await expect(page.getByRole('dialog').filter({ hasText: 'OLED display designer' })).toHaveCount(0)

  await page.getByRole('button', { name: 'Close', exact: true }).last().click()
  await page.getByRole('row').filter({ hasText: 'OLED Display' }).click()
  await page.getByRole('button', { name: 'Design display' }).click()
  await page.getByText('Hello OLED').click()
  await expect(page.getByRole('dialog').last().getByRole('textbox', { name: 'Text', exact: true })).toHaveValue('Hello OLED')
})

test('OLED designer imports, resizes, saves, and reloads bitmap widgets', async ({ page }) => {
  await page.goto(mockPath)
  await page.getByRole('row').filter({ hasText: 'OLED Display' }).click()
  await page.getByRole('button', { name: 'Design display' }).click()

  const designer = page.getByRole('dialog').last()
  await designer.getByRole('button', { name: 'Bitmap', exact: true }).click()

  const bitmapFile = Buffer.from('<svg xmlns="http://www.w3.org/2000/svg" width="4" height="4"><rect width="4" height="4" fill="white"/><rect x="1" y="1" width="2" height="2" fill="black"/></svg>')
  await designer.locator('input[type="file"]').setInputFiles({
    name: 'bitmap.svg',
    mimeType: 'image/svg+xml',
    buffer: bitmapFile,
  })

  const numberFields = designer.locator('input[type="number"]')
  const widthField = numberFields.nth(2)
  const heightField = numberFields.nth(3)
  await widthField.fill('8')
  await widthField.press('Tab')
  await heightField.fill('8')
  await heightField.press('Tab')
  await page.waitForTimeout(300)
  await expect(widthField).toHaveValue('8')
  await expect(heightField).toHaveValue('8')

  await designer.getByRole('button', { name: 'Save', exact: true }).click()
  await page.getByRole('button', { name: 'Close', exact: true }).last().click()

  await page.getByRole('row').filter({ hasText: 'OLED Display' }).click()
  await page.getByRole('button', { name: 'Design display' }).click()
  const reopened = page.getByRole('dialog').last()
  await reopened.getByRole('button', { name: 'Bitmap', exact: true }).click()
  await expect(reopened.locator('input[type="number"]').nth(2)).toHaveValue('16')
  await expect(reopened.locator('input[type="number"]').nth(3)).toHaveValue('16')
})
