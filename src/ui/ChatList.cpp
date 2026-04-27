// ── Telegram CLI — Chat List Implementation ─────────────────────────────────
#include "ChatList.hpp"
#include "../app/Config.hpp"
#include "../app/ExteraGram.hpp"

using namespace ftxui;

namespace tgcli::ui {

ChatList::ChatList(AppState& state) : state_(state) {}

Component ChatList::component() {
    auto search_input = Input(&search_text_, "Search...");
    auto search_maybe = Maybe(search_input, &searching_);

    return Renderer(search_maybe, [this, search_input]() {
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

        // Manual scroll window
        int max_visible = 15; // approximate number of chats that fit
        int start_idx = std::max(0, state_.selected_chat_index - max_visible / 2);

        // Chat entries
        int idx = 0;
        int rendered = 0;
        
        // We'll store the mapping of visual row to actual chat_id for mouse clicks
        visible_chat_ids_.clear();

        for (const auto& chat : state_.chats) {
            // Search filter
            if (searching_ && !search_text_.empty()) {
                std::string lower_title = chat.title;
                std::string lower_search = search_text_;
                std::transform(lower_title.begin(), lower_title.end(), lower_title.begin(), ::tolower);
                std::transform(lower_search.begin(), lower_search.end(), lower_search.begin(), ::tolower);
                if (lower_title.find(lower_search) == std::string::npos) {
                    idx++;
                    continue;
                }
            }

            if (rendered < start_idx) {
                rendered++;
                idx++;
                continue;
            }
            if (visible_chat_ids_.size() >= static_cast<size_t>(max_visible)) {
                break;
            }

            visible_chat_ids_.push_back({idx, chat.id});

            bool selected = (idx == state_.selected_chat_index);

            // Chat type icon
            std::string icon;
            int icon_color = theme.chatlist_fg;
            if (chat.is_channel) {
                icon = "\xE2\x9A\xA1 "; // ⚡
                icon_color = theme.chatlist_channel;
            } else if (chat.is_group) {
                icon = "\xE2\x9C\xAA "; // ✪
                icon_color = theme.chatlist_group;
            } else if (chat.is_bot) {
                icon = "\xE2\x9A\x99 "; // ⚙
                icon_color = theme.chatlist_bot;
            } else {
                icon = "\xE2\x98\xBA "; // ☺
                icon_color = theme.chatlist_fg;
            }

            // Pin indicator
            std::string pin = chat.is_pinned ? "\xE2\x9C\xAF " : ""; // ✮

            // Title
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
                    | color(Color::Palette256(0))
                    | bgcolor(Color::Palette256(theme.chatlist_unread));
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
            
            rendered++;
            idx++;
        }

        // Fill remaining space
        items.push_back(filler() | bgcolor(Color::Palette256(theme.chatlist_bg)));

        return vbox(std::move(items))
            | size(WIDTH, EQUAL, 28)
            | bgcolor(Color::Palette256(theme.chatlist_bg))
            | reflect(box_);
    }) | CatchEvent([this](Event event) {
        // Handle mouse clicks
        if (event.is_mouse()) {
            if (!box_.Contain(event.mouse().x, event.mouse().y)) {
                return false; // let other components handle mouse!
            }

            if (event.mouse().button == Mouse::Left && event.mouse().motion == Mouse::Pressed) {
                int start_y = searching_ ? 3 : 2; // header(1) + search(1)? + sep(1)
                int local_y = event.mouse().y - box_.y_min;
                
                if (local_y >= start_y) {
                    int clicked_visual_idx = (local_y - start_y) / 3;
                    std::lock_guard<std::mutex> lock(state_.mtx);
                    if (clicked_visual_idx >= 0 && clicked_visual_idx < static_cast<int>(visible_chat_ids_.size())) {
                        state_.selected_chat_index = visible_chat_ids_[clicked_visual_idx].first;
                        state_.selected_chat_id = visible_chat_ids_[clicked_visual_idx].second;
                        if (on_select_) on_select_(state_.selected_chat_id);
                    }
                }
                return true;
            }
            if (event.mouse().button == Mouse::WheelDown) {
                std::lock_guard<std::mutex> lock(state_.mtx);
                if (state_.selected_chat_index < static_cast<int>(state_.chats.size()) - 1) {
                    state_.selected_chat_index++;
                }
                return true;
            }
            if (event.mouse().button == Mouse::WheelUp) {
                std::lock_guard<std::mutex> lock(state_.mtx);
                if (state_.selected_chat_index > 0) {
                    state_.selected_chat_index--;
                }
                return true;
            }
            return true; // We consumed the mouse event inside our box
        }

        // If searching, let input handle printable characters (except Esc and Enter)
        if (searching_) {
            if (event == Event::Escape) {
                searching_ = false;
                search_text_.clear();
                return true;
            }
            if (event == Event::Return) {
                std::lock_guard<std::mutex> lock(state_.mtx);
                // When we press enter in search, we might just want to select the top match
                if (!state_.chats.empty()) {
                    // For now, just exit search mode
                    searching_ = false;
                }
                return true;
            }
            if (event == Event::ArrowUp || event == Event::ArrowDown) {
                // allow arrows to navigate even while searching
            } else {
                return false; // let search_input handle 'j', 'k', etc.
            }
        }

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
