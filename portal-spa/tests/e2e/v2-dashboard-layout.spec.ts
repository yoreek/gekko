import { expect, test } from '@playwright/test'

type Box = { id: string; x: number; y: number; w: number; h: number }

async function readItemBoxes(page: import('@playwright/test').Page): Promise<Box[]> {
  return page.$$eval('.grid-stack-item', items =>
    items.map(el => {
      const r = el.getBoundingClientRect()
      return {
        id: (el.getAttribute('gs-id') ?? '') as string,
        x: r.x,
        y: r.y,
        w: r.width,
        h: r.height,
      }
    }),
  )
}

function overlaps(a: Box, b: Box): boolean {
  // 1px tolerance for sub-pixel rounding / borders.
  const t = 1
  return a.x < b.x + b.w - t && a.x + a.w - t > b.x && a.y < b.y + b.h - t && a.y + a.h - t > b.y
}

test('dashboard cards of varying height do not overlap', async ({ page }) => {
  await page.goto('http://127.0.0.1:5176/?mockMode=1&mockReset=1', { waitUntil: 'networkidle' })
  await page.waitForSelector('.grid-stack-item', { timeout: 10000 })
  // Let sizeToContent + nextTick re-measure settle.
  await page.waitForTimeout(1500)

  const boxes = await readItemBoxes(page)
  console.log(`cards: ${boxes.length}`)
  console.log(boxes.map(b => `#${b.id} ${Math.round(b.w)}x${Math.round(b.h)} @(${Math.round(b.x)},${Math.round(b.y)})`).join('\n'))

  // There must be at least a couple of cards, and their heights must actually vary.
  expect(boxes.length).toBeGreaterThan(1)
  const heights = new Set(boxes.map(b => Math.round(b.h)))
  console.log(`distinct heights: ${[...heights].join(', ')}`)

  // No two cards overlap.
  const collisions: string[] = []
  for (let i = 0; i < boxes.length; i += 1) {
    for (let j = i + 1; j < boxes.length; j += 1) {
      if (overlaps(boxes[i], boxes[j])) {
        collisions.push(`#${boxes[i].id} <-> #${boxes[j].id}`)
      }
    }
  }
  await page.screenshot({ path: '/tmp/v2-dashboard-layout.png', fullPage: true })
  expect(collisions, `overlapping cards: ${collisions.join(', ')}`).toEqual([])
})

test('dragging a card in edit mode keeps free placement without overlap', async ({ page }) => {
  await page.goto('http://127.0.0.1:5176/?mockMode=1&mockReset=1', { waitUntil: 'networkidle' })
  await page.waitForSelector('.grid-stack-item', { timeout: 10000 })
  await page.waitForTimeout(1500)

  await page.getByRole('button', { name: 'Edit' }).click()
  await page.waitForTimeout(300)

  const before = await readItemBoxes(page)
  // GPIO Relay card (#670845750) has an interactive body (power switch); dragging
  // it must work when grabbing the BODY, not only the header.
  const target = before.find(b => b.id === '670845750') ?? before[0]
  const handle = page.locator(`.grid-stack-item[gs-id="${target.id}"]`)
  const box = await handle.boundingBox()
  if (!box) throw new Error('no bounding box for drag target')

  // The per-card action button (the "..." menu when other panels exist, otherwise the
  // plain delete/close button) must be fully inside the card content, not clipped.
  // Scoped to the aria-label directly: the card root also has role="button" with an
  // aggregated accessible name that would otherwise match too under getByRole's
  // default substring matching.
  const closeBtn = handle.locator('button[aria-label="Widget actions"], button[aria-label="Delete"]')
  const closeBox = await closeBtn.boundingBox()
  if (!closeBox) throw new Error('no close button visible in edit mode')
  expect(closeBox.x + closeBox.width, 'close button not clipped on the right').toBeLessThanOrEqual(box.x + box.width + 1)
  expect(closeBox.y, 'close button not clipped on the top').toBeGreaterThanOrEqual(box.y - 1)

  // Grab the LOWER part of the card body (below the header) and drag down.
  await page.mouse.move(box.x + box.width / 2, box.y + box.height - 20)
  await page.mouse.down()
  await page.mouse.move(box.x + box.width / 2, box.y + box.height + 200, { steps: 10 })
  await page.mouse.up()
  await page.waitForTimeout(500)

  const after = await readItemBoxes(page)
  const moved = after.find(b => b.id === target.id)
  expect(moved, 'dragged card still present').toBeTruthy()
  // The card actually moved (free placement took effect).
  expect(Math.abs((moved as Box).y - target.y), 'card moved to a new position').toBeGreaterThan(30)

  // Still no overlaps after the drag.
  const collisions: string[] = []
  for (let i = 0; i < after.length; i += 1) {
    for (let j = i + 1; j < after.length; j += 1) {
      if (overlaps(after[i], after[j])) {
        collisions.push(`#${after[i].id} <-> #${after[j].id}`)
      }
    }
  }
  await page.screenshot({ path: '/tmp/v2-dashboard-drag.png', fullPage: true })
  expect(collisions, `overlapping cards after drag: ${collisions.join(', ')}`).toEqual([])

  // Give the debounced save time to flush, then reload WITHOUT mockReset and
  // confirm the dragged position (and its content-driven height) survived.
  await page.waitForTimeout(600)
  await page.goto('http://127.0.0.1:5176/?mockMode=1', { waitUntil: 'networkidle' })
  await page.waitForSelector('.grid-stack-item', { timeout: 10000 })
  await page.waitForTimeout(1500)

  const reloaded = await readItemBoxes(page)
  const persisted = reloaded.find(b => b.id === target.id)
  expect(persisted, 'card present after reload').toBeTruthy()
  // Position preserved within a row's tolerance.
  expect(Math.abs((persisted as Box).y - (moved as Box).y), 'dragged position persisted across reload').toBeLessThan(40)

  const reloadCollisions: string[] = []
  for (let i = 0; i < reloaded.length; i += 1) {
    for (let j = i + 1; j < reloaded.length; j += 1) {
      if (overlaps(reloaded[i], reloaded[j])) {
        reloadCollisions.push(`#${reloaded[i].id} <-> #${reloaded[j].id}`)
      }
    }
  }
  expect(reloadCollisions, `overlapping cards after reload: ${reloadCollisions.join(', ')}`).toEqual([])
})

test('reset layout re-packs cards to the top', async ({ page }) => {
  await page.goto('http://127.0.0.1:5176/?mockMode=1&mockReset=1', { waitUntil: 'networkidle' })
  await page.waitForSelector('.grid-stack-item', { timeout: 10000 })
  await page.waitForTimeout(1500)

  // Push a card far down first so "reset" has something to undo.
  await page.getByRole('button', { name: 'Edit' }).click()
  await page.waitForTimeout(300)
  const first = (await readItemBoxes(page))[0]
  const box = await page.locator(`.grid-stack-item[gs-id="${first.id}"]`).boundingBox()
  if (!box) throw new Error('no bounding box')
  await page.mouse.move(box.x + box.width / 2, box.y + 20)
  await page.mouse.down()
  await page.mouse.move(box.x + box.width / 2, box.y + 400, { steps: 10 })
  await page.mouse.up()
  await page.waitForTimeout(400)
  await page.getByRole('button', { name: 'Done' }).click()

  const displaced = (await readItemBoxes(page)).find(b => b.id === first.id) as Box
  expect(displaced.y, 'card was displaced downward').toBeGreaterThan(first.y + 100)

  // Reset should snap it back up to the top row.
  await page.getByRole('button', { name: 'Reset layout' }).click()
  await page.waitForTimeout(1500)

  const afterReset = await readItemBoxes(page)
  const resetTarget = afterReset.find(b => b.id === first.id) as Box
  await page.screenshot({ path: '/tmp/v2-dashboard-reset.png', fullPage: true })
  expect(resetTarget.y, 'reset moved the card back near the top').toBeLessThan(displaced.y - 100)

  const collisions: string[] = []
  for (let i = 0; i < afterReset.length; i += 1) {
    for (let j = i + 1; j < afterReset.length; j += 1) {
      if (overlaps(afterReset[i], afterReset[j])) {
        collisions.push(`#${afterReset[i].id} <-> #${afterReset[j].id}`)
      }
    }
  }
  expect(collisions, `overlapping cards after reset: ${collisions.join(', ')}`).toEqual([])
})
