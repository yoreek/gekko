import { expect, test } from '@playwright/test'
import type { Page } from '@playwright/test'

const mockPath = '/devices?mockMode=1&mockReset=1'
const storageKey = 'gekko.mockDb.v6'

async function selectOption(page: Page, name: string, option: string | RegExp): Promise<void> {
  const dialog = page.getByRole('dialog')
  const input = dialog.getByRole('combobox', { name, exact: true })
  await input.locator('xpath=ancestor::*[contains(@class, "v-field")][1]').click()
  await page.getByRole('option', { name: option }).click()
}

test('creates thermostat devices with deps and config', async ({ page }) => {
  await page.goto(mockPath)

  await page.getByRole('button', { name: 'Create device' }).click()
  await page.getByRole('textbox', { name: 'Name', exact: true }).fill('Greenhouse Thermostat')
  await selectOption(page, 'Type', 'Thermostat')

  const submit = page.getByRole('button', { name: 'Create device' }).last()
  await expect(submit).toBeDisabled()

  await selectOption(page, 'Temperature sensor', /Water Temperature #670845752/)
  await selectOption(page, 'Switch device', /GPIO Relay #670845750/)
  await expect(page.getByLabel('Target temperature')).toHaveValue('25')
  await expect(page.getByLabel('Hysteresis')).toHaveValue('0.5')
  await expect(submit).toBeEnabled()
  await submit.click()

  await expect(page.getByText('Greenhouse Thermostat')).toBeVisible()
  const created = await page.evaluate(key => {
    const db = JSON.parse(localStorage.getItem(key) ?? '{}')
    return db.devices?.find((device: { name?: string }) => device.name === 'Greenhouse Thermostat')
  }, storageKey)
  expect(created).toMatchObject({
    type_id: 5,
    has_deps: true,
    deps: [
      {
        role: 'temperature_sensor',
        device_id: 670845752,
      },
      {
        role: 'switch',
        device_id: 670845750,
      },
    ],
    config: {
      mode: 'heat',
      algorithm: 'hysteresis',
      target_milli_celsius: 25000,
      hysteresis_centi_celsius: 50,
      temperature_sensor_device_id: 670845752,
      switch_device_id: 670845750,
    },
    output: {
      desired_switch_state: 'off',
      actual_switch_state: 'off',
      control_status: 'idle',
    },
  })
})

test('realtime thermostat updates merge temperature and control state', async ({ page }) => {
  await page.goto(mockPath)

  await page.getByText('Grow Room Thermostat').click()
  await expect(page.getByLabel('Current temperature')).toHaveValue('24.6°C')
  await expect(page.getByLabel('Desired switch state')).toHaveValue('On')
  await expect(page.getByLabel('Actual switch state')).toHaveValue('Off')

  await page.evaluate(key => {
    const db = JSON.parse(localStorage.getItem(key) ?? '{}')
    const sensor = db.devices.find((device: { device_id: number }) => device.device_id === 670845752)
    window.__gekkoMockRealtime?.upsertDevice({
      ...sensor,
      output: {
        temperature: {
          value: 26.4,
          unit: 'celsius',
          unit_symbol: 'C',
          measured_at_ms: 30500,
          valid: true,
          status: 'ok',
        },
      },
    })
  }, storageKey)

  await expect(page.getByLabel('Current temperature')).toHaveValue('26.4°C')
  await expect(page.getByLabel('Desired switch state')).toHaveValue('Off')
  await expect(page.getByText('Control status: Idle')).toBeVisible()
})
