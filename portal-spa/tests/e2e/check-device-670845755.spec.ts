import { test } from '@playwright/test'

test('check device 670845755', async ({ page }) => {
  const errors: string[] = []
  
  page.on('console', msg => {
    console.log(`[${msg.type()}] ${msg.text()}`)
    if (msg.type() === 'error') {
      errors.push(msg.text())
    }
  })
  
  await page.goto('http://127.0.0.1:5176/v2/devices/670845755?mockMode=1', { waitUntil: 'networkidle' })
  
  console.log('✓ Page loaded')
  console.log('  URL:', page.url())
  
  await page.waitForTimeout(1000)
  
  // Check heading
  const heading = page.locator('[role="heading"]').first()
  const headingVisible = await heading.isVisible({ timeout: 2000 }).catch(() => false)
  if (headingVisible) {
    const text = await heading.textContent()
    console.log('✓ Heading visible:', text)
  } else {
    console.log('✗ Heading not visible')
  }
  
  // Check inputs
  const inputs = page.locator('input')
  const inputCount = await inputs.count()
  console.log(`✓ Found ${inputCount} input fields`)
  
  // Check for cards
  const cards = page.locator('v-card')
  const cardCount = await cards.count()
  console.log(`✓ Found ${cardCount} cards`)
  
  // Check buttons
  const buttons = page.locator('button')
  const buttonCount = await buttons.count()
  console.log(`✓ Found ${buttonCount} buttons`)
  
  if (errors.length > 0) {
    console.log('✗ Console errors found:')
    errors.forEach(e => console.log('  -', e))
  } else {
    console.log('✓ No console errors')
  }
})
