import { expect, test } from '@playwright/test'
import { readFileSync } from 'node:fs'
import { fileURLToPath } from 'node:url'
import { join } from 'node:path'

const rootDir = fileURLToPath(new URL('../', import.meta.url))
const distDir = join(rootDir, 'dist')

function escapeScript(content: string): string {
  return content.replace(/<\/script>/g, '<\\/script>')
}

function renderBuiltApp(): string {
  const html = readFileSync(join(distDir, 'index.html'), 'utf8')
  const cssMatch = html.match(/href=\"\.\/assets\/(index-[^"]+\.css)\"/)
  const jsMatch = html.match(/src=\"\.\/assets\/(index-[^"]+\.js)\"/)
  if (!cssMatch || !jsMatch) {
    throw new Error('Unable to locate built asset references in dist/index.html')
  }

  const css = readFileSync(join(distDir, 'assets', cssMatch[1]), 'utf8')
  const js = readFileSync(join(distDir, 'assets', jsMatch[1]), 'utf8')

  return html
    .replace(
      `<link rel="stylesheet" crossorigin href="./assets/${cssMatch[1]}">`,
      `<style>${css}</style>`,
    )
    .replace(
      `<script type="module" crossorigin src="./assets/${jsMatch[1]}"></script>`,
      `<script>history.replaceState(null, '', '?mockMode=1&mockReset=1');</script><script type="module">${escapeScript(js)}</script>`,
    )
}

const scenarios = [
  {
    name: 'desktop',
    viewport: { width: 1440, height: 900 },
  },
  {
    name: 'mobile',
    viewport: { width: 390, height: 844 },
  },
] as const

for (const scenario of scenarios) {
  test(`renders the dashboard shell on ${scenario.name}`, async ({ page }) => {
    await page.setViewportSize(scenario.viewport)
    await page.setContent(renderBuiltApp(), { waitUntil: 'load' })
    await expect(page.getByText('Gekko Portal')).toBeVisible()
    await expect(page.getByText(/Offline controller dashboard|Офлайн панель управления/i)).toBeVisible()
    await expect(page.locator('.hero-card')).toBeVisible()
    await expect(page.locator('.device-card').first()).toBeVisible()
    await expect(page.locator('.wifi-summary')).toBeVisible()
  })
}
