import { expect, test } from '@playwright/test'
import type { Page } from '@playwright/test'

const mockPath = '/devices?mockMode=1&mockReset=1'
const storageKey = 'gekko.mockDb.v4'

async function selectOption(page: Page, name: string, option: string | RegExp): Promise<void> {
  const dialog = page.getByRole('dialog')
  const input = dialog.getByRole('combobox', { name, exact: true })
  await input.locator('xpath=ancestor::*[contains(@class, "v-field")][1]').click()
  await page.getByRole('option', { name: option }).click()
}

test('creates DS18B20 devices with parent validation and filtered scan candidates', async ({ page }) => {
  await page.goto(mockPath)

  await page.getByRole('button', { name: 'Create device' }).click()
  await page.getByRole('textbox', { name: 'Name', exact: true }).fill('Cabinet Probe')
  await selectOption(page, 'Type', 'DS18B20 temperature sensor')

  const submit = page.getByRole('button', { name: 'Create device' }).last()
  await expect(submit).toBeDisabled()

  await selectOption(page, 'OneWire parent', /Sensor Bus #670845751/)
  await expect(submit).toBeDisabled()

  const scanCandidateInput = page.getByRole('dialog').getByRole('combobox', { name: 'Scan candidate', exact: true })
  await scanCandidateInput.locator('xpath=ancestor::*[contains(@class, "v-field")][1]').click()
  await expect(page.getByRole('option', { name: /28FF641D621603AD/ })).toBeVisible()
  await expect(page.getByRole('option', { name: /10FFAA0000000001/ })).toHaveCount(0)
  await page.keyboard.press('Escape')

  await page.getByLabel('DS18B20 address').fill('28FF641D621603AE')
  await expect(submit).toBeEnabled()
  await submit.click()

  await expect(page.getByText('Cabinet Probe')).toBeVisible()
  const created = await page.evaluate(key => {
    const db = JSON.parse(localStorage.getItem(key) ?? '{}')
    return db.devices?.find((device: { name?: string }) => device.name === 'Cabinet Probe')
  }, storageKey)
  expect(created).toMatchObject({
    type_id: 4,
    has_parent: true,
    parent_device_id: 670845751,
    config: {
      address: '28FF641D621603AE',
      resolution: 12,
      unit: 'celsius',
    },
  })
})

test('renders DS18B20 temperature and merges realtime unavailable updates', async ({ page }) => {
  await page.goto(mockPath)

  await page.getByText('Water Temperature').click()
  await expect(page.getByLabel('Temperature')).toHaveValue('24.63 C')

  await page.evaluate(key => {
    const db = JSON.parse(localStorage.getItem(key) ?? '{}')
    const sensor = db.devices.find((device: { device_id: number }) => device.device_id === 670845752)
    window.__gekkoMockRealtime?.upsertDevice({
      ...sensor,
      output: {
        temperature: {
          value: 25.5,
          unit: 'celsius',
          unit_symbol: 'C',
          measured_at_ms: 24000,
          valid: true,
          status: 'ok',
        },
      },
    })
  }, storageKey)
  await expect(page.getByLabel('Temperature')).toHaveValue('25.50 C')

  await page.evaluate(key => {
    const db = JSON.parse(localStorage.getItem(key) ?? '{}')
    const parent = db.devices.find((device: { device_id: number }) => device.device_id === 670845751)
    const sensor = db.devices.find((device: { device_id: number }) => device.device_id === 670845752)
    window.__gekkoMockRealtime?.upsertDevice({
      ...parent,
      enabled: false,
      lifecycle_status: 'disabled',
      effective_status: 'disabled',
      status: 'disabled',
      config: {
        ...parent.config,
        enabled: false,
      },
    })
    window.__gekkoMockRealtime?.upsertDevice({
      ...sensor,
      effective_status: 'disabled',
      status: 'disabled',
      output: {
        temperature: {
          value: 0,
          unit: 'celsius',
          unit_symbol: 'C',
          measured_at_ms: 0,
          valid: false,
          status: 'parent_disabled',
        },
      },
    })
  }, storageKey)

  await expect(page.getByLabel('Temperature')).toHaveValue('Unavailable')
  await expect(page.getByText('Temperature is unavailable until the sensor reports a valid reading.')).toBeVisible()
  await expect(page.getByText('disabled').first()).toBeVisible()
})
