import { test } from '@playwright/test'

test('debug device page content', async ({ page }) => {
  await page.goto('http://127.0.0.1:5176/v2/devices/670845755?mockMode=1', { waitUntil: 'domcontentloaded' })
  
  // Get all text content
  const text = await page.textContent('body')
  console.log('Body text length:', text?.length)
  
  // Check what elements exist
  const divCount = await page.locator('div').count()
  console.log('Total divs:', divCount)
  
  // Check for specific text patterns
  const hasDeviceText = text?.includes('Device') ? 'yes' : 'no'
  console.log('Has "Device" text:', hasDeviceText)
  
  const hasError = text?.includes('error') ? 'yes' : 'no'
  console.log('Has "error" text:', hasError)
  
  const hasNotFound = text?.includes('not found') ? 'yes' : 'no'
  console.log('Has "not found" text:', hasNotFound)
  
  // Check HTML structure
  const html = await page.content()
  const hasPageContainer = html.includes('PageContainer') ? 'yes' : 'no'
  console.log('Has PageContainer:', hasPageContainer)
  
  // List all visible text nodes (first 500 chars of each)
  const allText = await page.locator('*').allTextContents()
  console.log('First few text nodes:')
  allText.slice(0, 5).forEach((t, i) => {
    const clean = t.trim().substring(0, 80)
    if (clean) console.log(`  ${i}: ${clean}`)
  })
})
