// ── Telegram CLI — Info Panel Implementation ────────────────────────────────
#include "InfoPanel.hpp"
#include "../app/Config.hpp"
#include "../app/ExteraGram.hpp"

using namespace ftxui;

namespace tgcli::ui {

InfoPanel::InfoPanel(AppState& state) : state_(state) {}

Component InfoPanel::component() {
    return Renderer([this]() {
        std::lock_guard<std::mutex> lock(state_.mtx);
        auto& theme = Config::instance().theme;

        if (state_.selected_chat_id == 0) {
            return vbox({
                text(" Info") | bold | color(Color::Palette256(theme.accent)),
                separator(),
                text("  No chat selected") | dim,
                filler(),
            }) | size(WIDTH, EQUAL, 24)
               | bgcolor(Color::Palette256(theme.chatlist_bg));
        }

        // Find selected chat
        const ChatEntry* chat = nullptr;
        for (const auto& c : state_.chats) {
            if (c.id == state_.selected_chat_id) {
                chat = &c;
                break;
            }
        }

        if (!chat) {
            return vbox({
                text(" Info") | bold,
                text("  Loading...") | dim,
                filler(),
            }) | size(WIDTH, EQUAL, 24)
               | bgcolor(Color::Palette256(theme.chatlist_bg));
        }

        Elements info_items;
        info_items.push_back(text(" Info") | bold | color(Color::Palette256(theme.accent)));
        info_items.push_back(separator() | color(Color::Palette256(theme.border_color)));

        // Chat type icon
        std::string type_label;
        if (chat->is_channel) type_label = "Channel";
        else if (chat->is_group) type_label = "Group";
        else if (chat->is_bot) type_label = "Bot";
        else type_label = "Private Chat";

        info_items.push_back(text(""));
        info_items.push_back(text("  " + chat->title) | bold);
        info_items.push_back(text("  " + type_label) | dim);

        // ExteraGram badge section (for private chats)
        if (chat->is_private) {
            // We'd need the user ID, which is the chat_id for private
            int64_t user_id = chat->id;  // In TDLib, private chat ID != user ID,
                                          // but we approximate here
            if (ExteraGram::has_badge(state_, user_id)) {
                std::string badge = ExteraGram::badge_symbol(state_, user_id);
                int badge_col = ExteraGram::badge_color(state_, user_id);
                std::string status = state_.extera_status(user_id);

                info_items.push_back(text(""));
                info_items.push_back(
                    hbox({
                        text("  " + badge + " ") | color(Color::Palette256(badge_col)),
                        text("exteraGram " + status) | color(Color::Palette256(badge_col)),
                    })
                );
            }
        }

        // Unread count
        if (chat->unread_count > 0) {
            info_items.push_back(text(""));
            info_items.push_back(hbox({
                text("  Unread: "),
                text(std::to_string(chat->unread_count))
                    | bold
                    | color(Color::Palette256(theme.chatlist_unread)),
            }));
        }

        info_items.push_back(text(""));
        info_items.push_back(separator() | color(Color::Palette256(theme.border_color)));

        // Actions
        info_items.push_back(text("  Actions:") | bold);
        info_items.push_back(text("   :stars  View Stars") | dim);
        info_items.push_back(text("   :gifts  View Gifts") | dim);
        info_items.push_back(text("   :mute   Mute Chat") | dim);
        info_items.push_back(text("   F2      Toggle panel") | dim);

        info_items.push_back(filler());

        return vbox(std::move(info_items))
            | size(WIDTH, EQUAL, 24)
            | bgcolor(Color::Palette256(theme.chatlist_bg));
    });
}

} // namespace tgcli::ui
