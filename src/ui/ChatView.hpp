#pragma once
// ── Telegram CLI — Chat View (Message History) ──────────────────────────────
#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include "../app/State.hpp"
#include <functional>

namespace tgcli::ui {

class ChatView {
public:
    using OnAction = std::function<void(const std::string& action, int64_t msg_id)>;

    ChatView(AppState& state);

    ftxui::Component component();
    void set_on_action(OnAction cb) { on_action_ = cb; }

private:
    AppState& state_;
    OnAction on_action_;
    int selected_msg_index_ = -1;

    ftxui::Element render_message(const MessageEntry& msg, bool selected);
    std::string format_time(int32_t timestamp);
};

} // namespace tgcli::ui
