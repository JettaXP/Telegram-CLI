#!/usr/bin/env bash
# ── Telegram CLI — Install system dependencies ───────────────────────────────
# Supports: Debian/Ubuntu, Arch Linux, Fedora
set -euo pipefail

echo "╔══════════════════════════════════════════════╗"
echo "║   Telegram CLI — Installing dependencies    ║"
echo "╚══════════════════════════════════════════════╝"

if command -v apt-get &>/dev/null; then
    echo "[*] Detected Debian/Ubuntu"
    sudo apt-get update -qq
    sudo apt-get install -y \
        build-essential cmake git gperf \
        libssl-dev zlib1g-dev \
        php-cli libcurl4-openssl-dev \
        libqrencode-dev \
        fonts-jetbrains-mono
elif command -v pacman &>/dev/null; then
    echo "[*] Detected Arch Linux"
    sudo pacman -Sy --noconfirm --needed \
        base-devel cmake git gperf \
        openssl zlib \
        php curl qrencode \
        ttf-jetbrains-mono-nerd
elif command -v dnf &>/dev/null; then
    echo "[*] Detected Fedora"
    sudo dnf install -y \
        gcc-c++ cmake git gperf \
        openssl-devel zlib-devel \
        php-cli libcurl-devel \
        qrencode-devel \
        jetbrains-mono-fonts
else
    echo "[!] Unsupported distro. Please install manually:"
    echo "    cmake, g++, git, gperf, openssl-dev, zlib-dev, php-cli, curl-dev"
    exit 1
fi

echo ""
echo "[+] All dependencies installed successfully!"
echo "[*] Next step: run ./scripts/build_tdlib.sh"
