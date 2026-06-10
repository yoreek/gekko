import { expect, test } from '@playwright/test'

const baseUrl = 'http://127.0.0.1:4173'

const scenarios = [
  {
    name: 'desktop',
    viewport: { width: 1440, height: 900 },
  },
  {
    name: 'mobile',
    viewport: { width: 390, height: 844 },
  },
] as const

for (const scenario of scenarios) {
  test(`renders the dashboard shell on ${scenario.name}`, async ({ page }) => {
    await page.setViewportSize(scenario.viewport)
    await page.goto(`${baseUrl}/?mockMode=1&mockReset=1`)
    await page.waitForLoadState('networkidle')

    await expect(page.getByText('Gekko Portal')).toBeVisible()
    await expect(page.locator('.portal-drawer__item')).toHaveCount(5)
    await expect(page.getByRole('heading', { name: 'Device dashboard' })).toBeVisible()
    await expect(page.locator('.device-card').first()).toBeVisible()
  })

  test(`opens the device modal and runs mock actions on ${scenario.name}`, async ({ page }) => {
    await page.setViewportSize(scenario.viewport)
    await page.goto(`${baseUrl}/?mockMode=1&mockReset=1`)
    await page.waitForLoadState('networkidle')

    const deviceCard = page.locator('.device-card').first()
    await expect(deviceCard).toBeVisible()
    await deviceCard.click()

    const dialog = page.locator('.device-dialog')
    await expect(dialog.getByText('Device details')).toBeVisible()
    await expect(dialog.getByText('Aquarium Lamp')).toBeVisible()

    const renameInput = dialog.getByLabel('New name')
    await renameInput.fill('Aquarium Lamp Smoke')
    await dialog.getByRole('button', { name: 'Rename' }).click()
    await expect(dialog.locator('.device-dialog__headline')).toHaveText('Aquarium Lamp Smoke')

    const typedStatusChip = dialog.locator('.typed-panel .v-chip').first()
    await expect(typedStatusChip).toHaveText('Output on')
    await dialog.getByRole('button', { name: 'Output off' }).click()
    await expect(typedStatusChip).toHaveText('Output off')
  })

  test(`creates a device from the dashboard on ${scenario.name}`, async ({ page }) => {
    await page.setViewportSize(scenario.viewport)
    await page.goto(`${baseUrl}/?mockMode=1&mockReset=1`)
    await page.waitForLoadState('networkidle')

    await page.getByRole('button', { name: 'Create device' }).click()
    const createDialog = page.locator('.device-dialog').filter({ hasText: 'Create device' }).last()
    await expect(createDialog).toBeVisible()

    await createDialog.getByLabel('Name').fill('Smoke Device')
    const typeSelect = createDialog.getByRole('combobox', { name: 'Type' })
    await typeSelect.focus()
    await typeSelect.press('ArrowDown')
    await typeSelect.press('Enter')
    await createDialog.getByRole('button', { name: 'Create device' }).click()

    await expect(page.locator('.device-card')).toHaveCount(3)
    await expect(page.locator('.device-card').filter({ hasText: 'Smoke Device' })).toBeVisible()
  })

  test(`emits mock websocket updates on ${scenario.name}`, async ({ page }) => {
    await page.setViewportSize(scenario.viewport)
    await page.goto(`${baseUrl}/?mockMode=1&mockReset=1`)
    await page.waitForLoadState('networkidle')

    await expect(page.getByText('WebSocket connected')).toBeVisible()

    const firstDeviceName = page.locator('.device-card__name').first()
    await expect(firstDeviceName).toHaveText('Aquarium Lamp')

    await page.evaluate(() => {
      const controller = (window as Window & {
        __gekkoMockRealtime?: {
          upsertDevice(device: Record<string, unknown>): void
          removeDevice(deviceId: number): void
        }
      }).__gekkoMockRealtime
      if (!controller) {
        throw new Error('mock realtime controller unavailable')
      }

      const raw = window.localStorage.getItem('gekko.mockDb.v2')
      if (!raw) {
        throw new Error('mock database unavailable')
      }

      const db = JSON.parse(raw) as { devices?: Array<Record<string, unknown>> }
      const device = db.devices?.find(entry => entry.device_id === 670845748)
      if (!device) {
        throw new Error('seed device unavailable')
      }

      device.name = 'Aquarium Lamp Live'
      controller.upsertDevice(device)
    })

    await expect(firstDeviceName).toHaveText('Aquarium Lamp Live')

    await page.evaluate(() => {
      const controller = (window as Window & {
        __gekkoMockRealtime?: {
          removeDevice(deviceId: number): void
        }
      }).__gekkoMockRealtime
      if (!controller) {
        throw new Error('mock realtime controller unavailable')
      }

      controller.removeDevice(670845749)
    })

    await expect(page.locator('.device-card')).toHaveCount(1)
  })

  test(`navigates to routed pages on ${scenario.name}`, async ({ page }) => {
    await page.setViewportSize(scenario.viewport)
    await page.goto(`${baseUrl}/?mockMode=1&mockReset=1`)
    await page.waitForLoadState('networkidle')

    await page.locator('.portal-drawer__item').filter({ hasText: 'WiFi' }).click()
    await expect(page.getByText('Current connection state and manual network scans')).toBeVisible()
    await expect(page.locator('.empty-state').getByText('No networks scanned yet.')).toBeVisible()
    const scanButton = page.getByRole('button', { name: 'Scan' })
    await scanButton.click()
    await expect(page.getByText('GekkoLab', { exact: true })).toBeVisible()

    await page.locator('.portal-drawer__item').filter({ hasText: 'OTA' }).click()
    await expect(page.getByText('Firmware update status')).toBeVisible()
    await expect(page.locator('.page-card').filter({ hasText: 'Firmware update status' }).getByText('Free sketch space', { exact: true })).toBeVisible()

    await page.locator('.portal-drawer__item').filter({ hasText: 'System' }).click()
    await expect(page.getByText('Restart and runtime status')).toBeVisible()
    await expect(page.getByRole('button', { name: 'Restart' })).toBeVisible()

    await page.locator('.portal-drawer__item').filter({ hasText: 'Controller overview' }).click()
    await expect(page.getByRole('heading', { name: 'Controller overview' })).toBeVisible()
    await expect(page.locator('.page-card').filter({ hasText: 'Registry' }).getByText('Registry', { exact: true })).toBeVisible()
  })
}
