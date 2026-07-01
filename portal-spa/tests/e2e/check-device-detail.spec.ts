import { test } from '@playwright/test'

test('check device detail page', async ({ page }) => {
  page.on('console', msg => {
    console.log(`[${msg.type().toUpperCase()}] ${msg.text()}`)
  })

  page.on('pageerror', err => {
    console.log('[PAGE_ERROR]', err.message, err.stack)
  })

  console.log('Opening device detail page...')
  await page.goto('http://127.0.0.1:5176/devices/670845748')

  await page.waitForTimeout(2000)

  console.log('Page URL:', page.url())

  const h1 = await page.locator('h1').first().textContent().catch(() => 'NOT FOUND')
  console.log('H1:', h1)

  const inputs = await page.locator('input').count()
  console.log('Input fields:', inputs)

  const buttons = await page.locator('button').count()
  console.log('Buttons:', buttons)

  const errorMsg = await page.locator('[role="alert"]').textContent().catch(() => null)
  if (errorMsg) console.log('Error message:', errorMsg)
})
