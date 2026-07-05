import { writeFileSync } from 'node:fs'
import { test } from '@playwright/test'

test('screenshot device page', async ({ page }) => {
  await page.goto('http://127.0.0.1:5176/devices/670845755?mockMode=1', { waitUntil: 'networkidle' })
  await page.waitForTimeout(1000)

  await page.screenshot({ path: '/tmp/device-page.png' })

  const html = await page.content()
  writeFileSync('/tmp/device-page.html', html)
})
