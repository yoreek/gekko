import { test } from '@playwright/test'

test('device 670845755 detail page loads with data', async ({ page }) => {
  const logs: string[] = []
  
  page.on('console', msg => {
    const text = msg.text()
    logs.push(`[${msg.type().toUpperCase()}] ${text}`)
    if (msg.type() === 'error' || msg.type() === 'warn') {
      console.log(`[${msg.type()}] ${text}`)
    }
  })
  
  page.on('response', response => {
    if (response.url().includes('devices') || response.status() >= 400) {
      console.log(`[${response.status()}] ${response.url()}`)
    }
  })
  
  // First load list
  await page.goto('http://127.0.0.1:5176/devices?mockMode=1&mockReset=1', { waitUntil: 'networkidle' })
  console.log('✓ Devices list loaded')
  
  // Then go to detail
  await page.goto('http://127.0.0.1:5176/devices/670845755?mockMode=1', { waitUntil: 'networkidle' })
  console.log('✓ Detail page URL loaded')
  
  await page.waitForTimeout(2000)
  
  // Check if data is there
  const content = await page.textContent('body')
  const hasOled = content?.includes('OLED') ? '✓' : '✗'
  console.log(`${hasOled} Contains "OLED"`)
  
  const hasDisplay = content?.includes('Display') ? '✓' : '✗'
  console.log(`${hasDisplay} Contains "Display"`)
  
  const hasInput = await page.locator('input').first().isVisible({ timeout: 1000 }).catch(() => false)
  console.log(`${hasInput ? '✓' : '✗'} Has input fields`)
  
  const hasCard = await page.locator('v-card, .v-card').first().isVisible({ timeout: 1000 }).catch(() => false)
  console.log(`${hasCard ? '✓' : '✗'} Has card`)
  
  const hasNotFound = content?.includes('not found') ? '✓ PROBLEM: not found message' : '✗'
  console.log(hasNotFound)
})
