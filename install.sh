#!/usr/bin/env bash
# ── Telegram-CLI — Release Install Script ────────────────────────────────────
set -euo pipefail

REPO="JettaXP/Telegram-CLI"
ASSET_HINT="tgcli"
PRIMARY_NAME="tgcli"
ALIAS_NAME="telegram-cli"
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
curl -fsSL -o "$TMP_DIR/$PRIMARY_NAME" "$LATEST_URL"

if [ ! -s "$TMP_DIR/$PRIMARY_NAME" ]; then
  echo "[-] Error: Downloaded binary is empty."
  exit 1
fi

echo "[*] Installing to $INSTALL_DIR/$PRIMARY_NAME (might prompt for sudo)..."
sudo install -m 755 "$TMP_DIR/$PRIMARY_NAME" "$INSTALL_DIR/$PRIMARY_NAME"
sudo ln -sfn "$PRIMARY_NAME" "$INSTALL_DIR/$ALIAS_NAME"

echo ""
echo "[+] Installation complete!"
echo "    You can now launch the app from any terminal by typing: $PRIMARY_NAME or $ALIAS_NAME"
