import { chromium } from '@playwright/test'

const browser = await chromium.launch()
const page = await browser.newPage({ viewport: { width: 1280, height: 900 } })
await page.goto('http://127.0.0.1:5176/v2/dashboard?mockMode=1&mockReset=1', { waitUntil: 'networkidle' })
await page.waitForTimeout(1000)

// check compact card title font size
const cardTitle = page.locator('main .v-card-title').first()
const cardTitleClass = await cardTitle.getAttribute('class')
const cardTitleFontSize = await cardTitle.evaluate(el => getComputedStyle(el).fontSize)
console.log('compact card title class:', cardTitleClass)
console.log('compact card title font-size:', cardTitleFontSize)

// open dialog, check dialog title font size
const thermoCard = page.locator('main .v-card').filter({ hasText: 'Grow Room' })
await thermoCard.locator('.v-card-title').click()
await page.waitForTimeout(500)
const dialogTitle = page.locator('.v-dialog .v-card-title')
const dialogTitleClass = await dialogTitle.getAttribute('class')
const dialogTitleFontSize = await dialogTitle.evaluate(el => getComputedStyle(el).fontSize)
console.log('dialog title class:', dialogTitleClass)
console.log('dialog title font-size:', dialogTitleFontSize)
const dialogTitleText = await dialogTitle.textContent()
console.log('dialog title text:', dialogTitleText)

await page.screenshot({ path: '/tmp/font-check.png' })
await browser.close()
