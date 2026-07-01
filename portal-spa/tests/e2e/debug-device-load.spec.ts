import { test } from '@playwright/test'

test('debug device loading', async ({ page }) => {
  page.on('console', msg => {
    if (msg.type() === 'error' || msg.type() === 'warning') {
      console.log(`[${msg.type()}] ${msg.text()}`)
    }
  })

  console.log('Opening app with mockMode...')
  await page.goto('http://127.0.0.1:5176/?mockMode=1&mockReset=1')

  console.log('Navigating to device page...')
  await page.goto('http://127.0.0.1:5176/devices/670845748')

  await page.waitForTimeout(2000)

  const heading = await page.locator('h1').first().textContent()
  console.log('Page heading:', heading)

  const inputs = await page.locator('input').all()
  for (let i = 0; i < inputs.length; i++) {
    const value = await inputs[i].inputValue()
    const name = await inputs[i].getAttribute('name')
    console.log(`Input ${i}: name="${name}" value="${value}"`)
  }

  const html = await page.innerHTML('body')
  if (html.includes('670845748')) {
    console.log('Found device ID in DOM')
  }
  if (html.includes('Aquarium Lamp')) {
    console.log('Found device name in DOM')
  }
})
