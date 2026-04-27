// ── Telegram CLI — Chat List Implementation ─────────────────────────────────
#include "ChatList.hpp"
#include "../app/Config.hpp"
#include "../app/ExteraGram.hpp"

using namespace ftxui;

namespace tgcli::ui {

ChatList::ChatList(AppState& state) : state_(state) {}

Component ChatList::component() {
    auto search_input = Input(&search_text_, "Search...");

    return Renderer(search_input, [this, search_input]() {
        std::lock_guard<std::mutex> lock(state_.mtx);
        auto& theme = Config::instance().theme;

        Elements items;

        // Header
        items.push_back(
            hbox({
                text("  Chats") | bold | color(Color::Palette256(theme.accent)),
                filler(),
                text(std::to_string(state_.chats.size())) | dim,
                text(" "),
            }) | bgcolor(Color::Palette256(theme.chatlist_bg))
        );

        // Search bar (when active)
        if (searching_) {
            items.push_back(
                hbox({
                    text(" / ") | color(Color::Palette256(theme.accent)),
                    search_input->Render() | flex,
                })
            );
        }

        items.push_back(separator() | color(Color::Palette256(theme.border_color)));

        // Chat entries
        int idx = 0;
        for (const auto& chat : state_.chats) {
            // Search filter
            if (searching_ && !search_text_.empty()) {
                std::string lower_title = chat.title;
                std::string lower_search = search_text_;
                // Simple case-insensitive search
                std::transform(lower_title.begin(), lower_title.end(), lower_title.begin(), ::tolower);
                std::transform(lower_search.begin(), lower_search.end(), lower_search.begin(), ::tolower);
                if (lower_title.find(lower_search) == std::string::npos) {
                    idx++;
                    continue;
                }
            }

            bool selected = (idx == state_.selected_chat_index);

            // Chat type icon (Nerd Font)
            std::string icon;
            int icon_color = theme.chatlist_fg;
            if (chat.is_channel) {
                icon = "\xEF\x81\xA8 "; // nf-fa-bullhorn
                icon_color = theme.chatlist_channel;
            } else if (chat.is_group) {
                icon = "\xEF\x83\x80 "; // nf-fa-users
                icon_color = theme.chatlist_group;
            } else if (chat.is_bot) {
                icon = "\xEF\x84\xA4 "; // nf-fa-robot
                icon_color = theme.chatlist_bot;
            } else {
                icon = "\xEF\x80\x87 "; // nf-fa-user
                icon_color = theme.chatlist_fg;
            }

            // Pin indicator
            std::string pin = chat.is_pinned ? "\xEF\x82\x8D " : ""; // nf-fa-thumb-tack

            // Title with ExteraGram badge
            auto title_elem = hbox({
                text(icon) | color(Color::Palette256(icon_color)),
                text(chat.title) | bold | color(Color::Palette256(
                    selected ? theme.chatlist_selected_fg : theme.chatlist_fg
                )),
            });

            // Unread badge
            auto unread_elem = text("");
            if (chat.unread_count > 0) {
                unread_elem = text(" " + std::to_string(chat.unread_count) + " ")
                    | bold
                    | color(Color::Palette256(0))               // black text
                    | bgcolor(Color::Palette256(theme.chatlist_unread)); // orange bg
            }

            // Last message preview
            auto preview_elem = text(" " + chat.last_message)
                | dim
                | color(Color::Palette256(theme.chatlist_fg))
                | size(WIDTH, LESS_THAN, 24);

            // Compose chat row
            auto row = vbox({
                hbox({
                    text(pin) | color(Color::Palette256(theme.chatlist_pinned)),
                    title_elem | flex,
                    unread_elem,
                    text(" "),
                }),
                preview_elem,
            });

            // Selection highlight
            if (selected) {
                row = row | bgcolor(Color::Palette256(theme.chatlist_selected_bg));
            } else {
                row = row | bgcolor(Color::Palette256(theme.chatlist_bg));
            }

            items.push_back(row);
            items.push_back(separator() | dim | color(Color::Palette256(theme.border_color)));
            idx++;
        }

        // Fill remaining space
        items.push_back(filler() | bgcolor(Color::Palette256(theme.chatlist_bg)));

        return vbox(std::move(items))
            | size(WIDTH, EQUAL, 28)
            | bgcolor(Color::Palette256(theme.chatlist_bg));
    }) | CatchEvent([this](Event event) {
        // Keyboard navigation
        if (event == Event::ArrowDown || event == Event::Character('j')) {
            std::lock_guard<std::mutex> lock(state_.mtx);
            if (state_.selected_chat_index < static_cast<int>(state_.chats.size()) - 1) {
                state_.selected_chat_index++;
                if (on_select_) {
                    state_.selected_chat_id = state_.chats[state_.selected_chat_index].id;
                    on_select_(state_.selected_chat_id);
                }
            }
            return true;
        }
        if (event == Event::ArrowUp || event == Event::Character('k')) {
            std::lock_guard<std::mutex> lock(state_.mtx);
            if (state_.selected_chat_index > 0) {
                state_.selected_chat_index--;
                if (on_select_) {
                    state_.selected_chat_id = state_.chats[state_.selected_chat_index].id;
                    on_select_(state_.selected_chat_id);
                }
            }
            return true;
        }
        if (event == Event::Character('/')) {
            searching_ = !searching_;
            if (!searching_) search_text_.clear();
            return true;
        }
        if (event == Event::Return) {
            std::lock_guard<std::mutex> lock(state_.mtx);
            if (!state_.chats.empty() && state_.selected_chat_index < static_cast<int>(state_.chats.size())) {
                state_.selected_chat_id = state_.chats[state_.selected_chat_index].id;
                if (on_select_) on_select_(state_.selected_chat_id);
            }
            return true;
        }
        return false;
    });
}

} // namespace tgcli::ui
