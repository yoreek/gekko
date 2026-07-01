import { test, expect } from '@playwright/test'

const mockPath = '/?mockMode=1&mockReset=1'

test('Device create page loads', async ({ page }) => {
  const errors: string[] = []

  page.on('console', msg => {
    if (msg.type() === 'error') {
      errors.push(msg.text())
      console.log('[CONSOLE ERROR]', msg.text())
    }
  })

  page.on('pageerror', err => {
    console.log('[PAGE ERROR]', err.message)
    errors.push(err.message)
  })

  console.log('Navigating to', mockPath + '#/devices/new')
  await page.goto(`http://127.0.0.1:5176${mockPath}#/devices/new`, { waitUntil: 'domcontentloaded' })

  await page.waitForTimeout(2000)

  const heading = await page.locator('h1').first().textContent().catch(() => 'NOT FOUND')
  console.log('Page heading:', heading)

  const hasForm = await page.locator('input[type="text"]').first().isVisible().catch(() => false)
  console.log('Form inputs visible:', hasForm)

  console.log('Total console errors:', errors.length)
})

test('Device detail page loads', async ({ page }) => {
  const errors: string[] = []

  page.on('console', msg => {
    if (msg.type() === 'error') {
      errors.push(msg.text())
      console.log('[CONSOLE ERROR]', msg.text())
    }
  })

  console.log('Navigating to device detail page')
  await page.goto(`http://127.0.0.1:5176${mockPath}#/devices/670845748`, { waitUntil: 'domcontentloaded' })

  await page.waitForTimeout(2000)

  const heading = await page.locator('h1').first().textContent().catch(() => 'NOT FOUND')
  console.log('Device detail heading:', heading)

  console.log('Total console errors:', errors.length)
})
