import type { Plugin } from 'vite';
import type { IncomingMessage, ServerResponse } from 'http';

interface Datasource {
  id: number;
  name: string;
  description?: string;
  plugin_type: 'sqlite';
  plugin_type_name: string;
  settings: {
    'sqlite.path': string;
  };
  is_default: boolean;
  created_at: number;
  updated_at: number;
}

let datasources: Datasource[] = [
  {
    id: 1,
    name: 'local-metrics',
    description: '本地 SQLite 指标库',
    plugin_type: 'sqlite',
    plugin_type_name: 'SQLite',
    settings: { 'sqlite.path': './data/metrics.db' },
    is_default: true,
    created_at: Date.now(),
    updated_at: Date.now(),
  },
];

let nextId = 2;

function readBody(req: IncomingMessage): Promise<any> {
  return new Promise((resolve, reject) => {
    let data = '';
    req.on('data', (chunk) => (data += chunk));
    req.on('end', () => {
      try {
        resolve(data ? JSON.parse(data) : {});
      } catch (e) {
        reject(e);
      }
    });
    req.on('error', reject);
  });
}

function sendJson(res: ServerResponse, data: unknown, status = 200) {
  res.statusCode = status;
  res.setHeader('Content-Type', 'application/json');
  res.end(JSON.stringify({ dat: data, err: '' }));
}

function mockQueryRange(query: string, start: number, end: number, step: number) {
  const points: [number, string][] = [];
  for (let t = start; t <= end; t += step) {
    const value = (Math.sin(t / 300) * 50 + 50 + Math.random() * 5).toFixed(2);
    points.push([t, value]);
  }

  const metricName = query.match(/[a-zA-Z_:][a-zA-Z0-9_:]*/)?.[0] || 'metric';

  return {
    status: 'success',
    data: {
      resultType: 'matrix',
      result: [
        {
          metric: { __name__: metricName, datasource: 'sqlite' },
          values: points,
        },
        {
          metric: { __name__: metricName, datasource: 'sqlite', instance: 'b' },
          values: points.map(([t, v]) => [t, (Number(v) * 0.8).toFixed(2)]),
        },
      ],
    },
  };
}

export function mockApiPlugin(): Plugin {
  return {
    name: 'mock-api',
    configureServer(server) {
      server.middlewares.use(async (req, res, next) => {
        if (!req.url?.startsWith('/api')) return next();

        const url = new URL(req.url, 'http://localhost');
        const pathname = url.pathname;
        const method = req.method || 'GET';

        try {
          if (pathname === '/api/datasource/plugins' && method === 'GET') {
            return sendJson(res, [
              {
                plugin_type: 'sqlite',
                plugin_type_name: 'SQLite',
                category: 'timeseries',
              },
            ]);
          }

          if (pathname === '/api/datasources' && method === 'GET') {
            return sendJson(res, datasources);
          }

          if (pathname === '/api/datasources' && method === 'POST') {
            const body = await readBody(req);
            const ds: Datasource = {
              id: nextId++,
              name: body.name,
              description: body.description,
              plugin_type: 'sqlite',
              plugin_type_name: 'SQLite',
              settings: body.settings,
              is_default: datasources.length === 0,
              created_at: Date.now(),
              updated_at: Date.now(),
            };
            datasources.push(ds);
            return sendJson(res, ds);
          }

          const dsMatch = pathname.match(/^\/api\/datasources\/(\d+)$/);
          if (dsMatch) {
            const id = Number(dsMatch[1]);
            const idx = datasources.findIndex((d) => d.id === id);
            if (idx === -1) return sendJson(res, null, 404);

            if (method === 'GET') return sendJson(res, datasources[idx]);
            if (method === 'PUT') {
              const body = await readBody(req);
              datasources[idx] = {
                ...datasources[idx],
                ...body,
                id,
                plugin_type: 'sqlite',
                updated_at: Date.now(),
              };
              return sendJson(res, datasources[idx]);
            }
            if (method === 'DELETE') {
              datasources.splice(idx, 1);
              return sendJson(res, 'ok');
            }
          }

          if (pathname === '/api/query_range' && method === 'POST') {
            const body = await readBody(req);
            const start = Number(body.start);
            const end = Number(body.end);
            const step = Number(body.step) || 15;
            return sendJson(res, mockQueryRange(body.query || 'up', start, end, step));
          }

          if (pathname === '/api/query' && method === 'POST') {
            const body = await readBody(req);
            const now = Math.floor(Date.now() / 1000);
            return sendJson(res, {
              status: 'success',
              data: {
                resultType: 'vector',
                result: [
                  {
                    metric: { __name__: body.query || 'up', datasource: 'sqlite' },
                    value: [now, String(Math.random() * 100)],
                  },
                ],
              },
            });
          }

          if (pathname === '/api/labels' && method === 'GET') {
            return sendJson(res, ['__name__', 'instance', 'job']);
          }

          if (pathname === '/api/label/__name__/values' && method === 'GET') {
            return sendJson(res, ['cpu_usage', 'memory_usage', 'disk_io', 'http_requests_total']);
          }

          return sendJson(res, { message: 'not found' }, 404);
        } catch (e) {
          return sendJson(res, { message: String(e) }, 500);
        }
      });
    },
  };
}
