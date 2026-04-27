#pragma once
// ── Telegram CLI — Chat List Sidebar ────────────────────────────────────────
#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include "../app/State.hpp"
#include <functional>

namespace tgcli::ui {

class ChatList {
public:
    using OnChatSelect = std::function<void(int64_t chat_id)>;

    ChatList(AppState& state);

    ftxui::Component component();
    void set_on_select(OnChatSelect cb) { on_select_ = cb; }

private:
    AppState& state_;
    OnChatSelect on_select_;
    std::string search_text_;
    bool searching_ = false;
    int list_scroll_ = 0;
    ftxui::Box box_;
    std::vector<std::pair<int, int64_t>> visible_chat_ids_;
};

} // namespace tgcli::ui
