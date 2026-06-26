import { expect, test } from '@playwright/test'
import type { Page } from '@playwright/test'

const mockPath = '/devices?mockMode=1&mockReset=1'

async function selectOption(page: Page, name: string, option: string | RegExp): Promise<void> {
  const dialog = page.getByRole('dialog').last()
  const input = dialog.getByRole('combobox', { name, exact: true })
  await input.locator('xpath=ancestor::*[contains(@class, "v-field")][1]').click()
  await page.getByRole('option', { name: option }).click()
}

test('OLED designer persists typed layout widgets in mock mode', async ({ page }) => {
  await page.goto(mockPath)

  await page.getByRole('button', { name: 'Create device' }).click()
  await page.getByRole('dialog').last().getByRole('textbox', { name: 'Name', exact: true }).fill('Desk OLED')
  await selectOption(page, 'Type', 'OLED display')
  await selectOption(page, 'I2C bus device ID', /I2C Bus #670845754/)

  await page.getByRole('dialog').last().getByRole('textbox', { name: 'OLED I2C address', exact: true }).fill('3C')
  await page.getByRole('dialog').last().getByRole('button', { name: 'Create device' }).click()

  await expect(page.getByText('Desk OLED')).toBeVisible()
  await page.getByText('Desk OLED').click()
  await page.getByRole('button', { name: 'Design display' }).click()

  const designer = page.getByRole('dialog').last()
  await designer.getByRole('button', { name: 'Text', exact: true }).click()
  await designer.getByRole('textbox', { name: 'Text', exact: true }).fill('Hello OLED')
  await designer.getByRole('button', { name: 'Save', exact: true }).click()

  await expect(page.getByRole('dialog').filter({ hasText: 'OLED display designer' })).toHaveCount(0)

  await page.getByRole('button', { name: 'Close', exact: true }).last().click()
  await page.getByRole('row').filter({ hasText: 'Desk OLED' }).click()
  await expect(page.getByText('Hello OLED')).toBeVisible()
  await page.getByRole('button', { name: 'Design display' }).click()
  await expect(page.getByRole('dialog').last().getByRole('textbox', { name: 'Text', exact: true })).toHaveValue('Hello OLED')
})
