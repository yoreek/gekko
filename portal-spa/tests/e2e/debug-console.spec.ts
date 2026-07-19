import { test } from '@playwright/test'

test('check console for errors', async ({ page }) => {
  const errors: string[] = []
  const warnings: string[] = []

  page.on('console', msg => {
    const text = msg.text()
    console.log(`[${msg.type().toUpperCase()}]`, text)
    if (msg.type() === 'error') errors.push(text)
    if (msg.type() === 'warning') warnings.push(text)
  })

  page.on('pageerror', err => {
    console.log('[PAGEERROR]', err.message)
    errors.push(err.message)
  })

  console.log('Navigating to index...')
  await page.goto('http://127.0.0.1:5176/?mockMode=1#/devices', { waitUntil: 'domcontentloaded' })

  await page.waitForTimeout(3000)

  console.log('=== FINAL STATS ===')
  console.log('Console errors:', errors.length)
  console.log('Console warnings:', warnings.length)
  console.log('Page title:', await page.title())

  const h1Count = await page.locator('h1').count()
  console.log('H1 elements:', h1Count)

  const allText = await page.textContent('body')
  if (allText) {
    console.log('Body has text:', allText.substring(0, 200))
  }
})
