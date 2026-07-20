import { expect, test } from '@playwright/test'
import type { Page } from '@playwright/test'
import { storageKey } from '../../src/mock/database.ts'

const mockPath = '/devices?mockMode=1&mockReset=1'

async function selectOption(page: Page, name: string, option: string | RegExp): Promise<void> {
  const input = page.getByRole('combobox', { name, exact: true })
  await input.locator('xpath=ancestor::*[contains(@class, "v-field")][1]').click()
  await page.getByRole('option', { name: option, exact: true }).click()
}

test('creates thermostat devices with deps and config', async ({ page }) => {
  await page.goto(mockPath)

  await page.goto('/devices/new?mockMode=1&mockReset=1')
  await page.getByRole('textbox', { name: 'Name', exact: true }).fill('Greenhouse Thermostat')
  await selectOption(page, 'Type', 'Thermostat')

  await selectOption(page, 'Temperature sensor', /Water Temperature #670845752/)
  await selectOption(page, 'Switch device', /GPIO Relay #670845750/)
  await expect(page.getByLabel('Target temperature (°C)')).toHaveValue('25')
  await expect(page.getByLabel('Hysteresis (°C)')).toHaveValue('0.5')

  const submit = page.getByRole('button', { name: 'Save' })
  await expect(submit).toBeEnabled()
  await submit.click()

  // Create redirects to the devices list (not the new device's detail page) so "back" from the
  // list never returns to a stale create form - filter by name (the list paginates and the new
  // device's id-based sort position isn't guaranteed to land on the first page) then open it.
  await expect(page).toHaveURL(/\/devices$/)
  await page.getByLabel('Search by name').fill('Greenhouse Thermostat')
  await page.getByRole('row', { name: /Greenhouse Thermostat/ }).click()
  await expect(page.getByRole('heading', { name: 'Greenhouse Thermostat' })).toBeVisible()
  const created = await page.evaluate(key => {
    const db = JSON.parse(localStorage.getItem(key) ?? '{}')
    return db.devices?.find((device: { config?: { name?: string } }) => device.config?.name === 'Greenhouse Thermostat')
  }, storageKey)
  expect(created).toMatchObject({
    record: {
      typeName: 'thermostat',
    },
    config: {
      enabled: true,
      name: 'Greenhouse Thermostat',
      deps: [
        {
          role: 'temperature_sensor',
          deviceId: 670845752,
        },
        {
          role: 'switch',
          deviceId: 670845750,
        },
      ],
      mode: 'heat',
      algorithm: 'hysteresis',
      targetMilliCelsius: 25000,
      hysteresisCentiCelsius: 50,
    },
    runtime: {
      output: {
        desiredSwitchState: false,
        actualSwitchState: false,
        controlStatus: 'idle',
      },
    },
  })
})

test('realtime thermostat updates merge temperature and control state', async ({ page }) => {
  await page.goto(mockPath)

  const devices = await page.evaluate(key => {
    const db = JSON.parse(localStorage.getItem(key) ?? '{}')
    return db.devices || []
  }, storageKey)
  const thermostatId = devices.find((d: any) => d.config?.name === 'Grow Room Thermostat')?.record?.id
  await page.goto(`/devices/${thermostatId}?mockMode=1&mockReset=1`)
  await expect(page.getByLabel('Desired switch state')).toHaveValue('On')
  await expect(page.getByLabel('Actual switch state')).toHaveValue('Off')

  await page.evaluate(key => {
    const db = JSON.parse(localStorage.getItem(key) ?? '{}')
    const sensor = db.devices.find((device: { record?: { id?: number } }) => device.record?.id === 670845752)
    window.__gekkoMockRealtime?.upsertDevice({
      ...sensor,
      runtime: {
        ...sensor.runtime,
        output: {
          temperature: {
            value: 26.4,
            unit: 'celsius',
            unitSymbol: 'C',
            measuredAtMs: 30500,
            valid: true,
            status: 'ok',
          },
        },
      },
    })
  }, storageKey)

  await expect(page.getByText('26.4°C')).toBeVisible()
  await expect(page.getByLabel('Desired switch state')).toHaveValue('Off')
  await expect(page.getByText('Control status: Idle')).toBeVisible()
})

test('enables save after editing thermostat config', async ({ page }) => {
  await page.goto(mockPath)

  const devices = await page.evaluate(key => {
    const db = JSON.parse(localStorage.getItem(key) ?? '{}')
    return db.devices || []
  }, storageKey)
  const thermostatId = devices.find((d: any) => d.config?.name === 'Grow Room Thermostat')?.record?.id
  await page.goto(`/devices/${thermostatId}?mockMode=1&mockReset=1`)

  // The device detail page also renders a separate Home Assistant settings card with its own
  // always-enabled Save button when WITH_HOME_ASSISTANT is compiled in - scope to the first one
  // (the main config card's Save, gated on canSave) rather than matching either.
  const saveButton = page.getByRole('button', { name: 'Save' }).first()
  await expect(saveButton).toBeDisabled()

  await page.getByLabel('Target temperature (°C)').fill('29')
  await expect(saveButton).toBeEnabled()

  await page.getByLabel('Hysteresis (°C)').fill('0.7')
  await expect(saveButton).toBeEnabled()
})
