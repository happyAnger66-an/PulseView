import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';
import path from 'path';

const useMock = process.env.USE_MOCK === 'true';

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
