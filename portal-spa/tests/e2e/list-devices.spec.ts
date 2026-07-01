import { test } from '@playwright/test'

test('list available devices', async ({ page }) => {
  await page.goto('http://127.0.0.1:5176/v2/devices?mockMode=1&mockReset=1', { waitUntil: 'networkidle' })
  await page.waitForTimeout(1500)
  
  // Get all list item texts
  const items = page.locator('v-list-item, .v-list-item, [role="link"]').filter({ hasText: /^[A-Za-z]/ })
  const count = await items.count()
  console.log(`Found ${count} items`)
  
  // Get all device links
  const links = await page.locator('a[href*="/v2/devices/"]').evaluateAll(els => 
    els.map(el => ({
      href: el.getAttribute('href'),
      text: el.textContent,
      id: el.getAttribute('href')?.match(/\/(\d+)/)?.[1]
    }))
  )
  
  console.log('Device links found:')
  links.forEach(link => {
    console.log(`  ID: ${link.id}, Name: ${link.text?.trim()}`)
  })
})
