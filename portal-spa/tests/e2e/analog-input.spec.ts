import { expect, test } from '@playwright/test'
import type { Page } from '@playwright/test'

const mockPath = '/devices?mockMode=1&mockReset=1'

async function selectOption(page: Page, name: string, option: string | RegExp): Promise<void> {
  const input = page.getByRole('combobox', { name, exact: true })
  await input.locator('xpath=ancestor::*[contains(@class, "v-field")][1]').click()
  await page.getByRole('option', { name: option }).click()
}

test('creates a CD74HC4067 hub and a channel bound to it', async ({ page }) => {
  await page.goto(mockPath)

  await page.getByRole('link', { name: 'Create device' }).click()
  await page.getByRole('textbox', { name: 'Name', exact: true }).fill('Test Analog Mux')
  await selectOption(page, 'Type', 'CD74HC4067 analog input hub')
  await expect(page.getByRole('button', { name: 'Save' })).toBeEnabled()
  await expect(page.getByLabel('Signal pin')).toBeVisible()

  await page.goto(mockPath)
  await page.getByRole('link', { name: 'Create device' }).click()
  await page.getByRole('textbox', { name: 'Name', exact: true }).fill('Test Mux Channel')
  await selectOption(page, 'Type', 'Analog input channel')
  const submit = page.getByRole('button', { name: 'Save' })
  await expect(submit).toBeEnabled()

  // Same leaf type, either hub -- the channel input's own max bound follows whichever hub is
  // actually selected (see analogInputHubChannelCount) rather than being baked into the type.
  const channelInput = page.getByLabel('Channel')

  await selectOption(page, 'Analog input hub device ID', /Analog Expansion ADC/)
  await expect(channelInput).toHaveAttribute('max', '3')

  await selectOption(page, 'Analog input hub device ID', /Analog Multiplexer/)
  await expect(channelInput).toHaveAttribute('max', '15')
  await expect(submit).toBeEnabled()
})

test('creates an NTC thermistor bound to an analog input and applies a preset', async ({ page }) => {
  await page.goto(mockPath)

  await page.getByRole('link', { name: 'Create device' }).click()
  await page.getByRole('textbox', { name: 'Name', exact: true }).fill('Test NTC Probe')
  await selectOption(page, 'Type', 'NTC thermistor temperature sensor')
  const submit = page.getByRole('button', { name: 'Save' })
  await expect(submit).toBeEnabled()

  await selectOption(page, 'Analog input device ID', /Air Temperature Analog Port/)
  await expect(submit).toBeEnabled()

  await selectOption(page, 'Sensor model preset', /Semitec 104GT-2 100k/)
  await expect(page.getByLabel('Nominal resistance (Ω)')).toHaveValue('100000')
  await expect(page.getByLabel('Beta coefficient (K)')).toHaveValue('4267')
})
