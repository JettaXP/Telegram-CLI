#pragma once
// ── Telegram-CLI — Commands Panel ───────────────────────────────────────────
#include <ftxui/component/component.hpp>
#include "../app/State.hpp"
#include <functional>

namespace tgcli::ui {

class CommandsPanel {
public:
    using OnCommand = std::function<void(const std::string& cmd)>;

    CommandsPanel(AppState& state);

    ftxui::Component component();
    void set_on_command(OnCommand cb) { on_command_ = cb; }

private:
    AppState& state_;
    OnCommand on_command_;
};

} // namespace tgcli::ui
