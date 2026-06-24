import { expect, test } from '@playwright/test'

const mockPath = '/devices?mockMode=1&mockReset=1'

test('exports and imports device setup bundles using record and config only', async ({ page }) => {
  await page.goto(mockPath)

  const bundle = await page.evaluate(async () => {
    const mod = await import('/src/mock/handlers.ts')
    return mod.mockExportDeviceSetupBundle()
  })

  const lines = bundle.trim().split(/\r?\n/)
  expect(lines.length).toBeGreaterThan(1)

  const firstDevice = JSON.parse(lines[1]) as Record<string, unknown>
  expect(firstDevice).toMatchObject({
    kind: 'device',
    record: {
      id: 670845748,
      typeName: 'dummy',
      configRevision: 4,
    },
    config: {
      enabled: true,
      name: 'Aquarium Lamp',
      deps: [],
    },
  })
  expect(firstDevice).not.toHaveProperty('runtime')
  expect(firstDevice).not.toHaveProperty('id')
  expect(firstDevice).not.toHaveProperty('typeName')
  expect(firstDevice).not.toHaveProperty('configRevision')

  await page.evaluate(async text => {
    const mod = await import('/src/mock/handlers.ts')
    const file = new File([text], 'device-setup.ndjson', { type: 'application/x-ndjson' })
    await mod.mockImportDeviceSetupBundle(file)
  }, bundle)

  const roundtrip = await page.evaluate(async () => {
    const mod = await import('/src/mock/handlers.ts')
    return mod.mockExportDeviceSetupBundle()
  })

  expect(roundtrip).toBe(bundle)
})
