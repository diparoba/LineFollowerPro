import { svelte } from '@sveltejs/vite-plugin-svelte'
import { defineConfig } from 'vite'

// https://vite.dev/config/
export default defineConfig({
  plugins: [svelte()],
  server: {
    port: 5011,
    host: '0.0.0.0',
    proxy: {
      '/api': {
        target: 'http://127.0.0.1:5010',
        changeOrigin: true
      },
      '/ws': {
        target: 'ws://127.0.0.1:5010',
        ws: true
      }
    }
  },
  preview: {
    port: 5011,
    host: '0.0.0.0',
    proxy: {
      '/api': {
        target: 'http://127.0.0.1:5010',
        changeOrigin: true
      },
      '/ws': {
        target: 'ws://127.0.0.1:5010',
        ws: true
      }
    }
  }
})
