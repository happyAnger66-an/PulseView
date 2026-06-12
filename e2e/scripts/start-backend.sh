#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT/backend"
PY="${ROOT}/backend/.venv/bin/python"
if [[ ! -x "$PY" ]]; then
  PY=python3
fi
exec "$PY" -m uvicorn app.main:app --host 127.0.0.1 --port 8080
