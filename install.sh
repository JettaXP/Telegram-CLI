#!/usr/bin/env bash
# ── Telegram-CLI — Release Install Script ────────────────────────────────────
set -euo pipefail

REPO="JettaXP/Telegram-CLI"
ASSET_HINT="tgcli"
INSTALL_NAME="telegram-cli"
INSTALL_DIR="/usr/local/bin"
TMP_DIR="$(mktemp -d)"

cleanup() {
  rm -rf "$TMP_DIR"
}
trap cleanup EXIT

echo "╔══════════════════════════════════════════════╗"
echo "║       Telegram-CLI — Installer              ║"
echo "╚══════════════════════════════════════════════╝"
echo ""

echo "[*] Fetching the latest release..."
LATEST_URL=$(curl -fsSL "https://api.github.com/repos/$REPO/releases/latest" | grep "browser_download_url" | grep -E "(telegram-cli|$ASSET_HINT)" | cut -d '"' -f 4 | head -n 1)

if [ -z "$LATEST_URL" ]; then
  echo "[-] Error: Could not find the latest release binary."
  echo "    Make sure a release is published on GitHub with an asset named 'telegram-cli' or '$ASSET_HINT'."
  exit 1
fi

echo "[*] Downloading from $LATEST_URL..."
curl -fsSL -o "$TMP_DIR/$INSTALL_NAME" "$LATEST_URL"

if [ ! -s "$TMP_DIR/$INSTALL_NAME" ]; then
  echo "[-] Error: Downloaded binary is empty."
  exit 1
fi

echo "[*] Installing to $INSTALL_DIR/$INSTALL_NAME (might prompt for sudo)..."
sudo install -m 755 "$TMP_DIR/$INSTALL_NAME" "$INSTALL_DIR/$INSTALL_NAME"

echo ""
echo "[+] Installation complete!"
echo "    You can now launch the app from any terminal by typing: $INSTALL_NAME"
