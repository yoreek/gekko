import { test } from '@playwright/test'

test('find all errors on device detail page', async ({ page }) => {
  const allMessages: string[] = []

  page.on('console', msg => {
    allMessages.push(`[${msg.type().toUpperCase()}] ${msg.text()}`)
  })

  page.on('pageerror', err => {
    allMessages.push(`[PAGE_ERROR] ${err.message}`)
  })

  console.log('Opening page...')
  await page.goto('http://127.0.0.1:5176/devices/670845748')

  await page.waitForTimeout(3000)

  console.log('\n========== ALL CONSOLE MESSAGES ==========')
  allMessages.forEach(msg => {
    console.log(msg)
  })

  console.log('\n========== CHECKING FUNCTIONALITY ==========')

  const inputs = await page.locator('input').all()
  console.log(`Found ${inputs.length} input fields`)

  for (let i = 0; i < inputs.length; i++) {
    const label = await inputs[i].getAttribute('aria-label')
    const value = await inputs[i].inputValue().catch(() => '')
    console.log(`  Input ${i+1}: label="${label}", value="${value}"`)
  }

  const buttons = await page.locator('button').all()
  console.log(`\nFound ${buttons.length} buttons`)

  for (let i = 0; i < Math.min(5, buttons.length); i++) {
    const text = await buttons[i].textContent()
    console.log(`  Button ${i+1}: "${text?.trim()}"`)
  }

  console.log('\n========== CHECKING FOR RENDER ERRORS ==========')
  const alerts = await page.locator('[role="alert"]').all()
  for (const alert of alerts) {
    const text = await alert.textContent()
    console.log(`Alert: ${text}`)
  }
})
