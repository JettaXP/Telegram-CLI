#pragma once
// ── Telegram CLI — Input Bar ────────────────────────────────────────────────
#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include "../app/State.hpp"
#include <functional>

namespace tgcli::ui {

class InputBar {
public:
    using OnSend = std::function<void(const std::string& text)>;
    using OnCommand = std::function<void(const std::string& cmd)>;

    InputBar(AppState& state);

    ftxui::Component component();
    void set_on_send(OnSend cb) { on_send_ = cb; }
    void set_on_command(OnCommand cb) { on_command_ = cb; }
    void set_text(const std::string& text) { input_text_ = text; }

private:
    AppState& state_;
    OnSend on_send_;
    OnCommand on_command_;
    std::string input_text_;
};

} // namespace tgcli::ui
