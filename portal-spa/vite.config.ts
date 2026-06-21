import { fileURLToPath, URL } from 'node:url'

import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import vuetify from 'vite-plugin-vuetify'
import { compression } from 'vite-plugin-compression2'

export default defineConfig({
  base: './',
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
