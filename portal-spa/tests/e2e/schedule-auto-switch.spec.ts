import { expect, test } from '@playwright/test'
import type { Page } from '@playwright/test'
import { storageKey } from '../../src/mock/database.ts'

const mockPath = '/devices?mockMode=1&mockReset=1'

async function selectOption(page: Page, name: string, option: string | RegExp): Promise<void> {
  const input = page.getByRole('combobox', { name, exact: true })
  await input.locator('xpath=ancestor::*[contains(@class, "v-field")][1]').click()
  await page.getByRole('option', { name: option, exact: true }).click()
}

async function saveAndOpenByName(page: Page, name: string): Promise<void> {
  const submit = page.getByRole('button', { name: 'Save' })
  await expect(submit).toBeEnabled()
  await submit.click()
  await expect(page).toHaveURL(/\/devices$/)
  await page.getByLabel('Search by name').fill(name)
  await page.getByRole('row', { name: new RegExp(name) }).click()
}

test('creates a schedule device with a rule', async ({ page }) => {
  await page.goto('/devices/new?mockMode=1&mockReset=1')
  await page.getByRole('textbox', { name: 'Name', exact: true }).fill('Night Light Schedule')
  await selectOption(page, 'Type', 'Schedule')

  await expect(page.getByText('No rules yet.')).toBeVisible()
  await page.getByRole('button', { name: 'Add rule' }).click()
  await expect(page.getByText('Rule 1')).toBeVisible()

  await saveAndOpenByName(page, 'Night Light Schedule')
  await expect(page.getByRole('heading', { name: 'Night Light Schedule' })).toBeVisible()

  const created = await page.evaluate(key => {
    const db = JSON.parse(localStorage.getItem(key) ?? '{}')
    return db.devices?.find((device: { config?: { name?: string } }) => device.config?.name === 'Night Light Schedule')
  }, storageKey)
  expect(created).toMatchObject({
    record: { typeName: 'schedule' },
    config: {
      enabled: true,
      name: 'Night Light Schedule',
      rules: [
        {
          enabled: true,
          weekDays: [0, 1, 2, 3, 4, 5, 6],
          mode: 'alwaysOn',
        },
      ],
    },
  })
})

test('creates an auto switch bound to a target switch and a schedule', async ({ page }) => {
  // Freeze the browser clock outside the default rule's 08:00-20:00 window (see
  // schedule-preview.ts, which now evaluates schedule activity client-side from the rule config
  // and the browser's own clock instead of a server-pushed value) so this test is deterministic
  // regardless of when it actually runs. 2024-01-08 is a Monday, included in the default rule's
  // weekDays.
  await page.clock.setFixedTime(new Date('2024-01-08T02:00:00'))
  await page.goto(mockPath)

  // Seed a schedule device to depend on, same create flow as the previous test.
  await page.goto('/devices/new?mockMode=1&mockReset=1')
  await page.getByRole('textbox', { name: 'Name', exact: true }).fill('Pump Schedule')
  await selectOption(page, 'Type', 'Schedule')
  await page.getByRole('button', { name: 'Add rule' }).click()
  await saveAndOpenByName(page, 'Pump Schedule')

  await page.goto('/devices/new?mockMode=1&mockReset=0')
  await page.getByRole('textbox', { name: 'Name', exact: true }).fill('Pump Auto Switch')
  await selectOption(page, 'Type', 'Auto switch')
  await selectOption(page, 'Target switch', /GPIO Relay #670845750/)
  await page.getByRole('button', { name: 'Add condition' }).click()
  await selectOption(page, 'Condition device', /Pump Schedule/)

  await saveAndOpenByName(page, 'Pump Auto Switch')
  await expect(page.getByRole('heading', { name: 'Pump Auto Switch' })).toBeVisible()

  const created = await page.evaluate(key => {
    const db = JSON.parse(localStorage.getItem(key) ?? '{}')
    return db.devices?.find((device: { config?: { name?: string } }) => device.config?.name === 'Pump Auto Switch')
  }, storageKey)
  expect(created).toMatchObject({
    record: { typeName: 'auto_switch' },
    config: {
      enabled: true,
      name: 'Pump Auto Switch',
      deps: expect.arrayContaining([
        expect.objectContaining({ role: 'switch', deviceId: 670845750 }),
        expect.objectContaining({ role: 'condition' }),
      ]),
    },
  })

  async function outputOf(): Promise<Record<string, unknown>> {
    return page.evaluate(
      ({ key, id }) => {
        const db = JSON.parse(localStorage.getItem(key) ?? '{}')
        const device = db.devices?.find((entry: { record?: { id?: number } }) => entry.record?.id === id)
        return device?.runtime?.output ?? {}
      },
      { key: storageKey, id: created.record.id },
    )
  }

  // Manual override: "On" forces the target regardless of the (currently inactive, per the frozen
  // 02:00 clock) schedule.
  await page.getByRole('button', { name: 'On', exact: true }).click()
  await expect.poll(async () => (await outputOf()).mode).toBe('on')
  await expect.poll(async () => (await outputOf()).state).toBe(true)

  // Back to Auto: the schedule is inactive at this (frozen) time, so this must turn the target off
  // - it must not keep whatever the manual "On" override left it at.
  await page.getByRole('button', { name: 'Auto', exact: true }).click()
  await expect.poll(async () => (await outputOf()).mode).toBe('auto')
  await expect.poll(async () => (await outputOf()).state).toBe(false)

  // Paused is a flat 4th mode (mirrors ReefDuino's ScheduledSwitchMode), only reachable from Auto -
  // pausedUntilMs must be set and mode itself reports "paused".
  await page.getByRole('button', { name: 'Paused', exact: true }).click()
  await expect.poll(async () => (await outputOf()).mode).toBe('paused')
  await expect.poll(async () => (await outputOf()).paused).toBe(true)
  expect(((await outputOf()).pausedUntilMs as number) ?? 0).toBeGreaterThan(0)

  // There is no separate "resume" action - the same "Auto" button that enters Auto from Off/On is
  // also the only way out of Paused.
  await page.getByRole('button', { name: 'Auto', exact: true }).click()
  await expect.poll(async () => (await outputOf()).mode).toBe('auto')
  await expect.poll(async () => (await outputOf()).paused).toBe(false)
})
