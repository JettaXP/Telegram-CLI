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

        bool can_send = true;
        {
            std::lock_guard<std::mutex> lock(state_.mtx);
            can_send = state_.selected_chat_details.can_send_messages;
        }

        if (!can_send) {
            // Show notice instead of input
            parts.push_back(
                hbox({
                    text(" ⚠ ") | color(Color::Palette256(theme.chatlist_unread)) | bold,
                    text("You cannot send messages in this chat (muted or read-only)") | color(Color::Palette256(theme.input_fg)),
                }) | bgcolor(Color::Palette256(theme.input_bg))
            );
            return vbox(std::move(parts));
        }

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
        {
            std::lock_guard<std::mutex> lock(state_.mtx);
            if (!state_.selected_chat_details.can_send_messages) {
                if (event == Event::Escape) {
                    state_.reply_to_msg_id = 0;
                    state_.edit_msg_id = 0;
                    return true;
                }
                return false;
            }
        }
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

        if (event == Event::ArrowUp) {
            if (on_nav_) on_nav_(NavAction::ArrowUp);
            return true;
        }
        if (event == Event::ArrowDown) {
            if (on_nav_) on_nav_(NavAction::ArrowDown);
            return true;
        }
        if (event == Event::PageUp) {
            if (on_nav_) on_nav_(NavAction::PageUp);
            return true;
        }
        if (event == Event::PageDown) {
            if (on_nav_) on_nav_(NavAction::PageDown);
            return true;
        }
        if (event == Event::Home) {
            if (on_nav_) on_nav_(NavAction::Home);
            return true;
        }
        if (event == Event::End) {
            if (on_nav_) on_nav_(NavAction::End);
            return true;
        }
        if (event == Event::F3) {
            if (on_nav_) on_nav_(NavAction::Reload);
            return true;
        }

        return false;
    });
}

} // namespace tgcli::ui
