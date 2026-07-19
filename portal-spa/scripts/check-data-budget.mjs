import { promises as fs } from 'node:fs'
import { fileURLToPath } from 'node:url'
import path from 'node:path'

// Keep this in sync with `littlefs` in `my_partitions.csv` (0x80000 = 512 KiB).
const dataBudgetBytes = 512 * 1024
const dataDir = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../../data')

async function walk(dir) {
  let total = 0
  const jsAssets = []
  try {
    const entries = await fs.readdir(dir, { withFileTypes: true })
    for (const entry of entries) {
      const entryPath = path.join(dir, entry.name)
      if (entry.isDirectory()) {
        const child = await walk(entryPath)
        total += child.total
        jsAssets.push(...child.jsAssets)
      } else if (entry.isFile() && entry.name.endsWith('.gz')) {
        const stat = await fs.stat(entryPath)
        total += stat.size
        if (entry.name.endsWith('.js.gz')) {
          jsAssets.push({ path: entryPath, size: stat.size })
        }
      }
    }
  } catch (error) {
    if (error && typeof error === 'object' && 'code' in error && error.code === 'ENOENT') {
      return { total: 0, jsAssets: [] }
    }
    throw error
  }
  return { total, jsAssets }
}

const { total, jsAssets } = await walk(dataDir)
const primaryJs = jsAssets.reduce((largest, asset) => (asset.size > largest.size ? asset : largest), { path: '', size: 0 })
const kib = (total / 1024).toFixed(1)
const primaryJsKib = (primaryJs.size / 1024).toFixed(1)

console.log(`data/ gzip assets: ${total} bytes (${kib} KiB)`)
console.log(`primary JS gzip asset: ${primaryJs.size} bytes (${primaryJsKib} KiB)`)

if (total > dataBudgetBytes) {
  console.error(`data budget exceeded: ${total} bytes > ${dataBudgetBytes} bytes`)
  process.exitCode = 1
}
