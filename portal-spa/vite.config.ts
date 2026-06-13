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
