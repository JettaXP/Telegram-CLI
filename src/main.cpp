// ── Telegram-CLI — Entry Point ──────────────────────────────────────────────
// A full-featured Telegram client for Linux terminals.
// Built with TDLib (Telegram's official C++ library) + FTXUI (modern TUI).
//
// Usage:
//   telegram-cli
//
// First launch will prompt for phone number authentication.
// Config is stored in: ~/.config/tgcli/config.ini
//
// Keyboard:
//   j/k or arrows  — navigate chats
//   Enter           — select chat / send message
//   /               — search chats
//   i               — toggle info panel
//   r               — reply to last message
//   e               — edit last outgoing message
//   d               — delete last message
//   f               — forward last message
//   Esc             — cancel reply/edit
//   Ctrl+C          — quit
//
// Commands (type in input bar):
//   :stars          — toggle Stars panel
//   :gifts          — toggle Gifts/NFT panel
//   :info           — toggle info panel
//   :theme dark     — switch to dark theme
//   :theme nord     — switch to nord theme
//   :theme gruvbox  — switch to gruvbox theme
//   :react <emoji>  — react to last message
//   :logout         — log out
//   :quit           — exit
//
// ─────────────────────────────────────────────────────────────────────────────

#include "app/App.hpp"

#include <csignal>
#include <iostream>

namespace {
void handle_sigint(int) {
    std::_Exit(130);
}
} // namespace

int main() {
    try {
        std::signal(SIGINT, handle_sigint);
        tgcli::App app;
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
