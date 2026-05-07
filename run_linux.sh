#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$ROOT_DIR/build"
BIN="$BUILD_DIR/bvp_solver_gui"

cmake -S "$ROOT_DIR" -B "$BUILD_DIR"
cmake --build "$BUILD_DIR" -j

# Clean environment to avoid snap-injected glibc/libpthread conflicts.
exec env -i \
  HOME="${HOME:-}" \
  USER="${USER:-}" \
  LOGNAME="${LOGNAME:-}" \
  SHELL="${SHELL:-/bin/bash}" \
  PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin" \
  LANG="${LANG:-C.UTF-8}" \
  DISPLAY="${DISPLAY:-}" \
  WAYLAND_DISPLAY="${WAYLAND_DISPLAY:-}" \
  XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-}" \
  XAUTHORITY="${XAUTHORITY:-}" \
  QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-}" \
  "$BIN"
