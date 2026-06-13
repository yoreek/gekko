import { promises as fs } from 'node:fs'
import path from 'node:path'

const projectRoot = path.resolve(process.cwd(), '..')
const distDir = path.join(process.cwd(), 'dist')
const dataDir = path.join(projectRoot, 'data')

async function ensureDir(dir) {
  await fs.mkdir(dir, { recursive: true })
}

async function cleanDirectory(dir) {
  try {
    await fs.rm(dir, { recursive: true, force: true })
  } catch {
    // Let the copy step surface filesystem problems.
  }
  await ensureDir(dir)
}

async function copyGzipAssets(sourceDir, targetDir) {
  const entries = await fs.readdir(sourceDir, { withFileTypes: true })
  for (const entry of entries) {
    const sourcePath = path.join(sourceDir, entry.name)
    const targetPath = path.join(targetDir, entry.name)
    if (entry.isDirectory()) {
      await ensureDir(targetPath)
      await copyGzipAssets(sourcePath, targetPath)
      continue
    }

    if (!entry.isFile() || !entry.name.endsWith('.gz')) {
      continue
    }

    await fs.copyFile(sourcePath, targetPath)
  }
}

await cleanDirectory(dataDir)
await copyGzipAssets(distDir, dataDir)

console.log(`deployed gzip assets from ${distDir} to ${dataDir}`)
