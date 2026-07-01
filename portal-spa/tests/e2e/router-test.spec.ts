import { test, expect } from '@playwright/test'

test('router navigation works', async ({ page }) => {
  console.log('Loading app...')
  await page.goto('http://127.0.0.1:5176/')

  console.log('Current URL:', page.url())

  // Navigate to devices page
  console.log('Navigating to /devices')
  await page.goto('http://127.0.0.1:5176/#/devices')
  const heading1 = await page.locator('h1').first().textContent()
  console.log('Devices page h1:', heading1)

  // Navigate to create page
  console.log('Navigating to /devices/new')
  await page.goto('http://127.0.0.1:5176/?mockMode=1#/devices/new')

  // Wait for navigation to complete
  await page.waitForLoadState('networkidle')

  const heading2 = await page.locator('h1').first().textContent().catch(() => 'NOT FOUND')
  console.log('Create page h1:', heading2)

  const inputs = await page.locator('input').count()
  console.log('Input count:', inputs)

  const buttons = await page.locator('button').count()
  console.log('Button count:', buttons)
})
