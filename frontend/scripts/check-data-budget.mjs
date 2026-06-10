import { promises as fs } from 'node:fs'
import path from 'node:path'

const budgetBytes = 220 * 1024
const dataDir = path.resolve(process.cwd(), '../data')

async function walk(dir) {
  let total = 0
  try {
    const entries = await fs.readdir(dir, { withFileTypes: true })
    for (const entry of entries) {
      const entryPath = path.join(dir, entry.name)
      if (entry.isDirectory()) {
        total += await walk(entryPath)
      } else if (entry.isFile() && entry.name.endsWith('.gz')) {
        const stat = await fs.stat(entryPath)
        total += stat.size
      }
    }
  } catch (error) {
    if (error && typeof error === 'object' && 'code' in error && error.code === 'ENOENT') {
      return 0
    }
    throw error
  }
  return total
}

const total = await walk(dataDir)
const kib = (total / 1024).toFixed(1)

console.log(`data/ gzip assets: ${total} bytes (${kib} KiB)`)

if (total > budgetBytes) {
  console.error(`budget exceeded: ${total} bytes > ${budgetBytes} bytes`)
  process.exitCode = 1
}

