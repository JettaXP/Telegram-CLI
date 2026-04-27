// ── Telegram CLI — Input Bar Implementation ─────────────────────────────────
#include "InputBar.hpp"
#include "../app/Config.hpp"

using namespace ftxui;

namespace tgcli::ui {

InputBar::InputBar(AppState& state) : state_(state) {}

Component InputBar::component() {
    auto input = Input(&input_text_, "Type a message...");

    return Renderer(input, [this, input]() {
        auto& theme = Config::instance().theme;

        Elements parts;

        // Context banner (replying/editing)
        {
            std::lock_guard<std::mutex> lock(state_.mtx);
            if (state_.reply_to_msg_id != 0) {
                parts.push_back(
                    hbox({
                        text(" Replying to message")
                            | color(Color::Palette256(theme.accent))
                            | bold,
                        filler(),
                        text(" [Esc to cancel] ") | dim,
                    }) | bgcolor(Color::Palette256(theme.input_bg))
                );
            } else if (state_.edit_msg_id != 0) {
                parts.push_back(
                    hbox({
                        text(" Editing message")
                            | color(Color::Palette256(theme.accent))
                            | bold,
                        filler(),
                        text(" [Esc to cancel] ") | dim,
                    }) | bgcolor(Color::Palette256(theme.input_bg))
                );
            }
        }

        // Input field
        parts.push_back(
            hbox({
                text(" > ") | color(Color::Palette256(theme.accent)) | bold,
                input->Render() | flex | color(Color::Palette256(theme.input_fg)),
            }) | bgcolor(Color::Palette256(theme.input_bg))
              | border
              | color(Color::Palette256(theme.input_border))
        );

        return vbox(std::move(parts));
    }) | CatchEvent([this](Event event) {
        if (event == Event::Return) {
            if (!input_text_.empty()) {
                // Check for commands
                if (input_text_[0] == ':') {
                    if (on_command_) on_command_(input_text_.substr(1));
                } else {
                    if (on_send_) on_send_(input_text_);
                }
                input_text_.clear();
            }
            return true;
        }

        if (event == Event::Escape) {
            // Cancel reply/edit
            std::lock_guard<std::mutex> lock(state_.mtx);
            state_.reply_to_msg_id = 0;
            state_.edit_msg_id = 0;
            return true;
        }

        return false;
    });
}

} // namespace tgcli::ui
