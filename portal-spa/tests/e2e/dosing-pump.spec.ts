import { expect, test } from '@playwright/test'
import type { Page } from '@playwright/test'
import { storageKey } from '../../src/mock/database.ts'

async function selectOption(page: Page, name: string, option: string | RegExp): Promise<void> {
  const input = page.getByRole('combobox', { name, exact: true })
  await input.locator('xpath=ancestor::*[contains(@class, "v-field")][1]').click()
  await page.getByRole('option', { name: option, exact: true }).click()
}

const seededPumpId = 670845772

async function openSeededPumpDetail(page: Page): Promise<void> {
  await page.goto(`/devices/${seededPumpId}?mockMode=1&mockReset=1`)
  await expect(page.getByRole('heading', { name: 'Calcium' })).toBeVisible()
}

test('seeded dosing pump renders its widget with schedule and container state', async ({ page }) => {
  await openSeededPumpDetail(page)

  await expect(page.getByText('25 / 50 ml')).toBeVisible()
  await expect(page.getByText('230 / 500 ml')).toBeVisible()
  await expect(page.getByRole('button', { name: 'Manual dose' })).toBeVisible()
  await expect(page.getByRole('button', { name: 'Calibration' })).toBeVisible()
  await expect(page.getByRole('button', { name: 'Dosing history' })).toBeVisible()
  // The four seeded schedule doses render as skip-toggle chips.
  await expect(page.getByText('08:00 · 12.5 ml')).toBeVisible()
  await expect(page.getByText('20:00 · 12.5 ml')).toBeVisible()
})

test('manual dose runs to completion and lands in totals, container and journal', async ({ page }) => {
  await openSeededPumpDetail(page)

  await page.getByRole('button', { name: 'Manual dose' }).click()
  const dialog = page.getByRole('dialog')
  await dialog.getByLabel('Amount (ml)').fill('5')
  await dialog.getByRole('button', { name: 'Start' }).click()

  // 5 ml at 1.25 ml/s = 4 s run; live progress ticks via the mock simulation.
  await expect(dialog.getByRole('button', { name: 'Stop' })).toBeVisible()
  await expect(dialog.getByText(/Last run dispensed 5 ml/)).toBeVisible({ timeout: 15000 })
  await dialog.getByRole('button', { name: 'Close' }).click()

  const state = await page.evaluate(key => {
    const db = JSON.parse(localStorage.getItem(key) ?? '{}')
    const pump = db.devices?.find((device: { record?: { typeName?: string } }) => device.record?.typeName === 'dosing_pump')
    return {
      todayDosedMl: pump?.runtime?.output?.todayDosedMl,
      currentMl: pump?.runtime?.output?.container?.currentMl,
      journalTail: db.doseJournal?.at(-1),
    }
  }, storageKey)
  expect(state.todayDosedMl).toBe(30) // seeded 25 + 5
  expect(state.currentMl).toBe(225) // seeded 230 - 5
  expect(state.journalTail).toMatchObject({ type: 'manual', amountMl: 5 })
})

test('calibration test run is not journaled and saves the corrected speed', async ({ page }) => {
  await openSeededPumpDetail(page)

  await page.getByRole('button', { name: 'Calibration' }).click()
  const dialog = page.getByRole('dialog')
  await dialog.getByLabel('Test amount (ml)').fill('5')
  await dialog.getByRole('button', { name: 'Run test dose' }).click()
  await expect(dialog.getByLabel('Measured volume (ml)')).toBeVisible({ timeout: 15000 })
  await dialog.getByLabel('Measured volume (ml)').fill('4')
  await expect(dialog.getByText(/new 1 ml\/s/)).toBeVisible()
  await dialog.getByRole('button', { name: 'Update speed' }).click()

  const readState = () =>
    page.evaluate(key => {
      const db = JSON.parse(localStorage.getItem(key) ?? '{}')
      const pump = db.devices?.find((device: { record?: { typeName?: string } }) => device.record?.typeName === 'dosing_pump')
      return {
        speed: pump?.config?.dosingSpeedMlPerSec,
        todayDosedMl: pump?.runtime?.output?.todayDosedMl,
        manualJournalEntries: (db.doseJournal ?? []).filter((entry: { type?: string }) => entry.type === 'manual').length,
      }
    }, storageKey)
  // 5 ml at 1.25 ml/s plans a 4 s run; 4 ml measured -> 1 ml/s. The updateConfig command posts
  // asynchronously after the dialog closes, so poll until it lands in the mock database.
  await expect.poll(async () => (await readState()).speed).toBe(1)
  const state = await readState()
  expect(state.todayDosedMl).toBe(25) // calibration is excluded from totals
  expect(state.manualJournalEntries).toBe(1) // only the seeded manual entry, no calibration record
})

test('container refill updates the level via setVolume', async ({ page }) => {
  await openSeededPumpDetail(page)

  await page.getByRole('button', { name: 'Container' }).click()
  const dialog = page.getByRole('dialog')
  await dialog.getByLabel('Current volume (ml)').fill('500')
  await dialog.getByRole('button', { name: 'Save' }).click()

  await expect(page.getByText('500 / 500 ml')).toBeVisible()
})

test('dosing history dialog lists seeded journal entries with stats', async ({ page }) => {
  await openSeededPumpDetail(page)

  await page.getByRole('button', { name: 'Dosing history' }).click()
  const dialog = page.getByRole('dialog')
  await expect(dialog.getByText(/doses/)).toBeVisible()
  await expect(dialog.getByText(/ml total/)).toBeVisible()
  await expect(dialog.getByRole('table')).toBeVisible()
  await expect(dialog.getByText('Scheduled').first()).toBeVisible()
})

test('creates a dosing pump bound to the seeded relay with a generated schedule', async ({ page }) => {
  await page.goto('/devices/new?mockMode=1&mockReset=1')
  // Select Type before filling Name - DeviceCreateView resets the whole draft (including name)
  // to createDefaultName(typeName) whenever typeName changes, which would wipe a name filled first.
  await selectOption(page, 'Type', 'Dosing pump')
  await page.getByRole('textbox', { name: 'Name', exact: true }).fill('Alkalinity')
  await selectOption(page, 'Pump switch', /GPIO Relay #670845750/)

  await page.getByRole('button', { name: 'Generator…' }).click()
  const generator = page.getByRole('dialog')
  await generator.getByLabel('Total amount (ml)').fill('10')
  await generator.getByLabel('Number of doses').fill('3')
  await generator.getByRole('button', { name: 'Apply' }).click()

  await page.getByRole('button', { name: 'Save' }).click()
  await expect(page).toHaveURL(/\/devices$/)

  const created = await page.evaluate(key => {
    const db = JSON.parse(localStorage.getItem(key) ?? '{}')
    return db.devices?.find((device: { config?: { name?: string } }) => device.config?.name === 'Alkalinity')
  }, storageKey)
  expect(created).toMatchObject({
    record: { typeName: 'dosing_pump' },
    config: {
      name: 'Alkalinity',
      deps: [{ role: 'switch', deviceId: 670845750 }],
      schedule: {
        doses: [
          { time: '09:00', amountMl: 3.3 },
          { time: '13:00', amountMl: 3.3 },
          { time: '17:00', amountMl: 3.4 },
        ],
      },
    },
  })
})
