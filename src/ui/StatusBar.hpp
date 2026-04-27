#pragma once
// ── Telegram CLI — Status Bar ───────────────────────────────────────────────
#include <ftxui/dom/elements.hpp>
#include "../app/State.hpp"

namespace tgcli::ui {

class StatusBar {
public:
    StatusBar(AppState& state);

    // Render the status bar as an FTXUI Element
    ftxui::Element render();

private:
    AppState& state_;
};

} // namespace tgcli::ui
