#!/usr/bin/env bash
# Record a real ros2_tracing session with a demo node, then verify PulseView can import it.
set -eo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WS="$ROOT"
TRACE_BASE="$WS/traces"
SESSION="${PV_TRACE_SESSION:-pv_trace_demo}"
DURATION="${PV_TRACE_DURATION_SEC:-5}"
BACKEND="$ROOT/../../backend"
PKG_SRC="$WS/src/pv_trace_demo"
BUILD_DIR="$PKG_SRC/build"

if [[ ! -f /opt/ros/jazzy/setup.bash ]]; then
  echo "ERROR: ROS 2 Jazzy not found at /opt/ros/jazzy/setup.bash" >&2
  exit 1
fi

# shellcheck disable=SC1091
source /opt/ros/jazzy/setup.bash

if ! command -v lttng-sessiond >/dev/null; then
  echo "ERROR: lttng-sessiond not installed (sudo apt install lttng-tools liblttng-ust-dev)" >&2
  exit 1
fi

TRACE_SESSION_DIR="$TRACE_BASE/$SESSION"
rm -rf "$TRACE_SESSION_DIR"
mkdir -p "$TRACE_BASE"

echo "==> building pv_trace_demo (cmake, rclcpp — callback tracepoints require C++)"
cmake -S "$PKG_SRC" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build "$BUILD_DIR" -j"$(nproc)" >/dev/null
NODE_BIN="$BUILD_DIR/trace_demo_node"

echo "==> starting ros2 trace session '$SESSION'"
ros2 trace start "$SESSION" -p "$TRACE_BASE"

echo "==> running trace_demo_node for ${DURATION}s"
set +e
"$NODE_BIN" --ros-args -p "duration_sec:=${DURATION}"
NODE_RC=$?
set -e
if [[ "$NODE_RC" != "0" ]]; then
  echo "WARN: trace_demo_node exit code $NODE_RC"
fi

echo "==> stopping trace session"
ros2 trace stop "$SESSION"

if [[ ! -d "$TRACE_SESSION_DIR" ]]; then
  echo "ERROR: trace directory missing: $TRACE_SESSION_DIR" >&2
  exit 1
fi

echo "==> verifying with PulseView lttng_ctf + CtfImporter"
if [[ ! -x "$BACKEND/.venv/bin/python" ]]; then
  echo "ERROR: backend venv missing at $BACKEND/.venv" >&2
  exit 1
fi

"$BACKEND/.venv/bin/python" "$ROOT/verify_trace.py" "$TRACE_SESSION_DIR" --min-spans 10

echo
echo "Trace saved to: $TRACE_SESSION_DIR"
echo "PulseView CTF datasource path: $TRACE_SESSION_DIR"
