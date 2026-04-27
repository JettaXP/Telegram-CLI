#!/usr/bin/env bash
# ── Telegram CLI — Release Install Script ─────────────────────────────────────
set -euo pipefail

REPO="JettaXP/Telegram-CLI"
BIN_NAME="tgcli"
INSTALL_DIR="/usr/local/bin"

echo "╔══════════════════════════════════════════════╗"
echo "║       Telegram CLI — Installer               ║"
echo "╚══════════════════════════════════════════════╝"
echo ""

echo "[*] Fetching the latest release..."
LATEST_URL=$(curl -s https://api.github.com/repos/$REPO/releases/latest | grep "browser_download_url" | grep "$BIN_NAME" | cut -d '"' -f 4 | head -n 1)

if [ -z "$LATEST_URL" ]; then
  echo "[-] Error: Could not find the latest release binary."
  echo "    Make sure a release is published on GitHub with the asset '$BIN_NAME'."
  exit 1
fi

echo "[*] Downloading from $LATEST_URL..."
curl -L -o "/tmp/$BIN_NAME" "$LATEST_URL"

echo "[*] Installing to $INSTALL_DIR/$BIN_NAME (might prompt for sudo)..."
sudo mv "/tmp/$BIN_NAME" "$INSTALL_DIR/$BIN_NAME"
sudo chmod +x "$INSTALL_DIR/$BIN_NAME"

echo ""
echo "[+] Installation complete!"
echo "    You can now launch the app from any terminal by typing: $BIN_NAME"
