#!/usr/bin/env bash
# ── Telegram CLI — Build TDLib from source ───────────────────────────────────
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
TDLIB_SRC="$PROJECT_DIR/deps/td-src"
TDLIB_BUILD="$PROJECT_DIR/deps/td-build"
TDLIB_INSTALL="$PROJECT_DIR/deps/td-install"

echo "╔══════════════════════════════════════════════╗"
echo "║       Telegram CLI — Building TDLib          ║"
echo "║  This may take 10-30 minutes. Please wait.   ║"
echo "╚══════════════════════════════════════════════╝"
echo ""

# Clone TDLib
if [ ! -d "$TDLIB_SRC" ]; then
    echo "[*] Cloning TDLib..."
    git clone --depth 1 https://github.com/tdlib/td.git "$TDLIB_SRC"
else
    echo "[*] TDLib source already exists, updating..."
    cd "$TDLIB_SRC" && git pull && cd -
fi

# Build
echo "[*] Configuring TDLib..."
mkdir -p "$TDLIB_BUILD"
cd "$TDLIB_BUILD"

cmake "$TDLIB_SRC" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$TDLIB_INSTALL"

echo "[*] Building TDLib (this will take a while)..."
cmake --build . --target install -j 1

echo ""
echo "[+] TDLib built and installed to: $TDLIB_INSTALL"
echo ""
echo "[*] Now configure your project with:"
echo "    mkdir -p build && cd build"
echo "    cmake .. -DTd_DIR=$TDLIB_INSTALL/lib/cmake/Td"
echo "    make -j 1"
