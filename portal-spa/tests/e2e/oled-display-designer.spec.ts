import { expect, test } from '@playwright/test'

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
