import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';
import path from 'path';

const useMock = process.env.USE_MOCK === 'true';
// Linux 上 inotify 监视器耗尽 (ENOSPC) 时改用 polling；可设 VITE_USE_POLLING=false 关闭
const usePolling = process.env.VITE_USE_POLLING !== 'false';

export default defineConfig(async () => {
  const plugins = [react()];
  if (useMock) {
    const { mockApiPlugin } = await import('./mock/server');
    plugins.push(mockApiPlugin());
  }

  return {
    plugins,
    resolve: {
      alias: {
        '@': path.resolve(__dirname, 'src'),
      },
    },
    server: {
      port: 8766,
      proxy: {
        '/api': {
          target: process.env.PROXY || 'http://localhost:8080',
          changeOrigin: true,
        },
      },
      watch: {
        usePolling,
        interval: usePolling ? 800 : undefined,
        ignored: ['**/node_modules/**', '**/dist/**', '**/.git/**'],
      },
    },
    build: {
      outDir: 'dist',
    },
    css: {
      preprocessorOptions: {
        less: {
          javascriptEnabled: true,
        },
      },
    },
  };
});
