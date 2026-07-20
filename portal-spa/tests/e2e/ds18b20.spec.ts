import { expect, test } from '@playwright/test'
import type { Page } from '@playwright/test'
import { storageKey } from '../../src/mock/database.ts'

const mockPath = '/devices?mockMode=1&mockReset=1'

async function selectOption(page: Page, name: string, option: string | RegExp): Promise<void> {
  const input = page.getByRole('combobox', { name, exact: true })
  await input.locator('xpath=ancestor::*[contains(@class, "v-field")][1]').click()
  await page.getByRole('option', { name: option, exact: true }).click()
}

test('validates DS18B20 dependency selection and filtered scan candidates', async ({ page }) => {
  await page.goto(mockPath)

  await page.getByRole('link', { name: 'Create device' }).click()
  await page.getByRole('textbox', { name: 'Name', exact: true }).fill('Cabinet Probe')
  await selectOption(page, 'Type', 'DS18B20 temperature sensor')

  const submit = page.getByRole('button', { name: 'Save' })
  await expect(submit).toBeEnabled()

  await selectOption(page, 'OneWire dependency', /Sensor Bus #670845751/)
  const scanCandidateInput = page.getByRole('combobox', { name: 'Scan candidate', exact: true })
  await scanCandidateInput.locator('xpath=ancestor::*[contains(@class, "v-field")][1]').click()
  await expect(page.getByRole('option', { name: /28FF641D621603AD/ })).toBeVisible()
  await expect(page.getByRole('option', { name: /10FFAA0000000001/ })).toHaveCount(0)
  await page.keyboard.press('Escape')
})

test('renders DS18B20 temperature and merges realtime unavailable updates', async ({ page }) => {
  await page.goto(mockPath)

  await page.getByText('Water Temperature').click()
  await expect(page.getByLabel('Temperature', { exact: true })).toHaveValue('24.63 C')

  await page.evaluate(key => {
    const db = JSON.parse(localStorage.getItem(key) ?? '{}')
    const sensor = db.devices.find((device: { record?: { id?: number } }) => device.record?.id === 670845752)
    window.__gekkoMockRealtime?.upsertDevice({
      ...sensor,
      runtime: {
        ...sensor.runtime,
        output: {
          temperature: {
            value: 25.5,
            unit: 'celsius',
            unitSymbol: 'C',
            measuredAtMs: 24000,
            valid: true,
            status: 'ok',
          },
        },
      },
    })
  }, storageKey)
  await expect(page.getByLabel('Temperature', { exact: true })).toHaveValue('25.50 C')

  await page.evaluate(key => {
    const db = JSON.parse(localStorage.getItem(key) ?? '{}')
    const dependency = db.devices.find((device: { record?: { id?: number } }) => device.record?.id === 670845751)
    const sensor = db.devices.find((device: { record?: { id?: number } }) => device.record?.id === 670845752)
    window.__gekkoMockRealtime?.upsertDevice({
      ...dependency,
      config: {
        ...dependency.config,
        enabled: false,
      },
      runtime: {
        ...dependency.runtime,
        lifecycleStatus: 'disabled',
        effectiveStatus: 'disabled',
        status: 'disabled',
      },
    })
    window.__gekkoMockRealtime?.upsertDevice({
      ...sensor,
      runtime: {
        ...sensor.runtime,
        effectiveStatus: 'disabled',
        status: 'disabled',
        output: {
          temperature: {
            value: 0,
            unit: 'celsius',
            unitSymbol: 'C',
            measuredAtMs: 0,
            valid: false,
            status: 'dependency_disabled',
          },
        },
      },
    })
  }, storageKey)

  await expect(page.getByLabel('Temperature', { exact: true })).toHaveValue('Unavailable')
})
