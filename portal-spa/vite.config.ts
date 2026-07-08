import { execSync } from 'node:child_process'
import { fileURLToPath, URL } from 'node:url'

import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import vuetify from 'vite-plugin-vuetify'
import { compression } from 'vite-plugin-compression2'

function gitDescribe(): string {
  try {
    return execSync('git describe --tags --always --dirty', { cwd: __dirname }).toString().trim()
  } catch {
    return 'unknown'
  }
}

const appVersion = gitDescribe()
const appBuildDate = new Date().toISOString()

export default defineConfig({
  define: {
    __APP_VERSION__: JSON.stringify(appVersion),
    __APP_BUILD_DATE__: JSON.stringify(appBuildDate),
  },
  // Must be absolute, not relative: the firmware always serves /assets/* from the
  // site root regardless of the requested path (PortalAssetController registers a
  // fixed "/assets/" prefix route), while vue-router's createWebHistory means deep
  // links like /devices/123 are real URLs. A relative base resolves asset URLs
  // against the current path depth, so anything but the root page 404s into the
  // SPA fallback and gets index.html back for a <script type="module"> request.
  base: '/',
  plugins: [
    vue(),
    vuetify({ autoImport: false }),
    compression({
      algorithm: 'gzip',
      include: [/\.(js|mjs|json|css|html|svg)$/i],
      deleteOriginalAssets: false,
    }),
  ],
  build: {
    assetsInlineLimit: 0,
    emptyOutDir: true,
    cssCodeSplit: false,
    minify: 'terser',
    reportCompressedSize: true,
    sourcemap: false,
    target: 'es2022',
    terserOptions: {
      compress: {
        drop_console: true,
        drop_debugger: true,
      },
    },
    rollupOptions: {
      output: {
        entryFileNames: 'assets/i-[hash].js',
        chunkFileNames: 'assets/c-[hash].js',
        assetFileNames: 'assets/a-[hash][extname]',
        manualChunks(id) {
          if (!id.includes('node_modules')) {
            return undefined
          }

          if (id.includes('/vuetify/')) {
            return 'vendor-vuetify'
          }

          if (
            id.includes('/vue/') ||
            id.includes('/vue-router/') ||
            id.includes('/vue-i18n/') ||
            id.includes('/pinia/') ||
            id.includes('/vue-grid-layout-v3/')
          ) {
            return 'vendor-vue'
          }

          return 'vendor'
        },
      },
    },
  },
  resolve: {
    alias: {
      '@': fileURLToPath(new URL('./src', import.meta.url)),
    },
  },
  server: {
    host: '127.0.0.1',
    port: 5176,
    strictPort: true,
  },
})
