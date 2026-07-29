import path from 'node:path'
import { fileURLToPath } from 'node:url'

import sharp from 'sharp'

const scriptDirectory = path.dirname(fileURLToPath(import.meta.url))
const repositoryRoot = path.resolve(scriptDirectory, '../..')
const source = path.join(repositoryRoot, '.github/social-preview.svg')
const destination = path.join(repositoryRoot, '.github/social-preview.png')
const logo = path.join(repositoryRoot, 'docs-site/src/assets/logo.svg')
const dashboard = path.join(
  repositoryRoot,
  'docs-site/src/assets/screenshots/portal-dashboard.png',
)

const dashboardMask = Buffer.from(`
  <svg xmlns="http://www.w3.org/2000/svg" width="630" height="394">
    <rect width="630" height="394" rx="18" fill="white"/>
  </svg>
`)

const dashboardPreview = await sharp(dashboard)
  .resize(630, 394, { fit: 'cover' })
  .composite([{ input: dashboardMask, blend: 'dest-in' }])
  .png()
  .toBuffer()

const logoPreview = await sharp(logo)
  .resize(112, 112)
  .png()
  .toBuffer()

await sharp(source)
  .composite([
    { input: logoPreview, left: 70, top: 72 },
    { input: dashboardPreview, left: 600, top: 104 },
  ])
  .png({ compressionLevel: 9 })
  .toFile(destination)
