#!/usr/bin/env bash
# Download amalgamated Perfetto C++ SDK into ./sdk/
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
DEST="${ROOT}/sdk"
mkdir -p "${DEST}"

TAG="$(curl -fsSL https://api.github.com/repos/google/perfetto/releases/latest | python3 -c "import sys,json; print(json.load(sys.stdin)['tag_name'])")"
URL="https://github.com/google/perfetto/releases/download/${TAG}/perfetto-cpp-sdk-src.zip"
TMP="$(mktemp -d)"

echo "Fetching Perfetto SDK ${TAG}..."
curl -fsSL -o "${TMP}/sdk.zip" "${URL}"
unzip -qo "${TMP}/sdk.zip" -d "${DEST}"
rm -rf "${TMP}"

ls -la "${DEST}/perfetto.h" "${DEST}/perfetto.cc"
echo "SDK ready at ${DEST}"
