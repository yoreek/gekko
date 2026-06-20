import { expect, test } from '@playwright/test'

const mockPath = '/devices?mockMode=1&mockReset=1'
const storageKey = 'gekko.mockDb.v4'

test('switch commands use structured output state fields', async ({ page }) => {
  await page.goto(mockPath)

  await page.getByText('GPIO Relay').click()
  await page.getByRole('button', { name: 'On', exact: true }).click()

  await expect.poll(async () => {
    return page.evaluate(key => {
      const db = JSON.parse(localStorage.getItem(key) || '{}')
      const devices = db.devices || []
      const device = devices.find(entry => entry.device_id === 670845750)
      if (!device || !device.output) {
        return ''
      }
      return device.output.state || ''
    }, storageKey)
  }).toBe('on')
})

test('dashboard switch power remains enabled when snapshot only has ready status', async ({ page }) => {
  await page.goto('/?mockMode=1&mockReset=1')

  await page.evaluate(key => {
    const db = JSON.parse(localStorage.getItem(key) || '{}')
    const device = db.devices?.find((entry: { device_id: number }) => entry.device_id === 670845750)
    window.__gekkoMockRealtime?.upsertDevice({
      ...device,
      lifecycle_status: undefined,
      effective_status: undefined,
      status: 'ready',
      output: {
        state: 'off',
      },
    })
  }, storageKey)

  await expect(page.getByRole('button', { name: 'Power', exact: true })).toBeEnabled()
})

test('onewire scan commands use the named scan action', async ({ page }) => {
  await page.goto(mockPath)

  await page.getByText('Sensor Bus').click()
  await page.getByRole('button', { name: 'Scan bus' }).click()

  await expect.poll(async () => {
    return page.evaluate(key => {
      const db = JSON.parse(localStorage.getItem(key) || '{}')
      const devices = db.devices || []
      const device = devices.find(entry => entry.device_id === 670845751)
      return !!(device && device.scan && device.scan.ready)
    }, storageKey)
  }).toBe(true)
})

test('onewire edit flows persist JSON config objects', async ({ page }) => {
  await page.goto(mockPath)

  await page.getByText('Sensor Bus').click()
  await page.getByRole('button', { name: 'Edit' }).last().click()

  const pinInput = page.getByLabel('GPIO pin')
  await pinInput.fill('19')
  await page.getByRole('button', { name: 'Save' }).click()

  await expect.poll(async () => {
    return page.evaluate(key => {
      const db = JSON.parse(localStorage.getItem(key) || '{}')
      const devices = db.devices || []
      const device = devices.find(entry => entry.device_id === 670845751)
      if (!device || !device.config) {
        return 0
      }
      return device.config.gpio_pin || 0
    }, storageKey)
  }).toBe(19)
})

test('dummy device has no command UI and creates only base config', async ({ page }) => {
  await page.goto(mockPath)

  await page.getByText('Aquarium Lamp').click()
  await expect(page.getByText('No additional type-specific settings.')).toBeVisible()
  await expect(page.getByText('Quick commands')).toHaveCount(0)
  await expect(page.getByRole('button', { name: 'On' })).toHaveCount(0)
  await expect(page.getByRole('button', { name: 'Off' })).toHaveCount(0)
  await page.getByRole('button', { name: 'Close', exact: true }).last().click()

  await page.getByRole('button', { name: 'Create device' }).click()
  await page.getByRole('dialog').getByRole('textbox', { name: 'Name', exact: true }).fill('Dummy Basic')
  await page.getByRole('button', { name: 'Create device' }).last().click()

  const created = await page.evaluate(key => {
    const db = JSON.parse(localStorage.getItem(key) || '{}')
    const devices = db.devices || []
    return devices.find((entry: { name?: string }) => entry.name === 'Dummy Basic')
  }, storageKey)

  expect(created).toMatchObject({
    type_id: 1,
    retained_state_supported: false,
    config: {
      enabled: true,
      name: 'Dummy Basic',
    },
  })
  expect(created?.config).not.toHaveProperty('restore_previous_state')
  expect(created?.config).not.toHaveProperty('default_output')
  expect(created?.config).not.toHaveProperty('current_output')
  expect(created).not.toHaveProperty('output')
})
