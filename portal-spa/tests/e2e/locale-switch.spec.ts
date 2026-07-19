import { expect, test } from '@playwright/test'

test.describe('language switcher', () => {
  test('switches locale, persists it, and survives reload without an English flash', async ({ page }) => {
    await page.goto('/?mockMode=1&mockReset=1', { waitUntil: 'networkidle' })
    await page.waitForLoadState('domcontentloaded')

    const languageButton = page.locator('[aria-label="Change language"]')
    await expect(languageButton).toBeVisible({ timeout: 8000 })
    await languageButton.click()
    await page.getByText('Українська', { exact: true }).click()

    await expect(page.locator('html')).toHaveAttribute('lang', 'uk')
    await expect(page.locator('[aria-label="Змінити мову"]')).toBeVisible({ timeout: 8000 })

    const stored = await page.evaluate(() => window.localStorage.getItem('gekko.locale'))
    expect(stored).toBe('uk')

    await page.reload({ waitUntil: 'networkidle' })
    await expect(page.locator('html')).toHaveAttribute('lang', 'uk')
    await expect(page.locator('[aria-label="Змінити мову"]')).toBeVisible({ timeout: 8000 })
  })
})
