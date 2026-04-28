// ── Telegram CLI — Chat List Implementation ─────────────────────────────────
#include "ChatList.hpp"
#include "../app/Config.hpp"
#include "../app/exteraGram.hpp"
#include <algorithm>

using namespace ftxui;

namespace tgcli::ui {

ChatList::ChatList(AppState& state) : state_(state) {}

Component ChatList::component() {
    auto search_input = Input(&search_text_, "Search...");
    auto search_maybe = Maybe(search_input, &searching_);

    auto renderer = Renderer(search_maybe, [this, search_input] {
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
            }) | size(WIDTH, EQUAL, 30) | bgcolor(Color::Palette256(theme.chatlist_bg))
        );

        if (searching_) {
            items.push_back(
                hbox({
                    text(" / ") | color(Color::Palette256(theme.accent)),
                    search_input->Render() | flex,
                }) | size(WIDTH, EQUAL, 30)
            );
        }

        items.push_back(separator() | color(Color::Palette256(theme.border_color)));

        // Display logic
        int max_visible = box_.y_max > 0 ? std::max(5, (box_.y_max - box_.y_min) / 3 - 2) : 10;
        int start_idx = std::max(0, state_.selected_chat_index - max_visible / 2);

        int idx = 0;
        int rendered = 0;
        visible_chat_ids_.clear();

        for (const auto& chat : state_.chats) {
            // Search filter
            if (searching_ && !search_text_.empty()) {
                std::string title = chat.title;
                std::string search = search_text_;
                std::transform(title.begin(), title.end(), title.begin(), ::tolower);
                std::transform(search.begin(), search.end(), search.begin(), ::tolower);
                if (title.find(search) == std::string::npos) {
                    idx++;
                    continue;
                }
            }

            if (idx < start_idx) { idx++; continue; }
            if (visible_chat_ids_.size() >= (size_t)max_visible) break;

            visible_chat_ids_.push_back({idx, chat.id});
            bool selected = (idx == state_.selected_chat_index);

            // Chat row
            std::string icon = chat.is_channel ? "CHA" : (chat.is_group ? "GRP" : "USR");
            std::string pin = chat.is_pinned ? "P " : "  ";
            
            auto title_line = hbox({
                text(chat.title) | bold | xflex,
                state_.is_extera_supporter(chat.id) ? text(" ➤") | color(Color::Palette256(exteraGram::badge_color(state_, chat.id))) : text(""),
            });

            auto row = vbox({
                hbox({
                    text(pin) | color(Color::Palette256(theme.chatlist_pinned)),
                    title_line | flex,
                    chat.unread_count > 0 ? text("[" + std::to_string(chat.unread_count) + "]") | color(Color::Palette256(theme.chatlist_unread)) : text(""),
                }),
                text(" " + chat.last_message) | dim | size(WIDTH, LESS_THAN, 28)
            }) | size(WIDTH, EQUAL, 30);

            if (selected) row = row | bgcolor(Color::Palette256(theme.chatlist_selected_bg));
            else row = row | bgcolor(Color::Palette256(theme.chatlist_bg));

            items.push_back(row);
            items.push_back(separator() | dim);
            
            rendered++;
            idx++;
        }

        items.push_back(filler() | flex | bgcolor(Color::Palette256(theme.chatlist_bg)));
        
        // Footer help
        items.push_back(
            vbox({
                separator() | dim,
                hbox({ text(" ↑/↓ Nav") | dim, filler(), text("Home/End ") | dim }),
            }) | size(WIDTH, EQUAL, 30) | bgcolor(Color::Palette256(theme.chatlist_bg))
        );

        return vbox(std::move(items)) | size(WIDTH, EQUAL, 30) | flex | reflect(box_);
    });

    return CatchEvent(renderer, [this, search_input](Event event) {
        auto handle_nav = [&](int new_idx) {
            std::lock_guard<std::mutex> lock(state_.mtx);
            if (state_.chats.empty()) return true;
            state_.selected_chat_index = std::clamp(new_idx, 0, (int)state_.chats.size() - 1);
            state_.selected_chat_id = state_.chats[state_.selected_chat_index].id;
            if (on_select_) on_select_(state_.selected_chat_id);
            return true;
        };

        if (event.is_mouse()) {
            if (!box_.Contain(event.mouse().x, event.mouse().y)) return false;
            if (event.mouse().button == Mouse::Left && event.mouse().motion == Mouse::Released) {
                int local_y = event.mouse().y - box_.y_min - (searching_ ? 3 : 2);
                int clicked_idx = local_y / 3;
                if (clicked_idx >= 0 && clicked_idx < (int)visible_chat_ids_.size()) {
                    return handle_nav(visible_chat_ids_[clicked_idx].first);
                }
            }
            if (event.mouse().button == Mouse::WheelUp) return handle_nav(state_.selected_chat_index - 1);
            if (event.mouse().button == Mouse::WheelDown) return handle_nav(state_.selected_chat_index + 1);
            return true;
        }

        if (searching_) {
            if (event == Event::Escape) { searching_ = false; return true; }
            if (event == Event::Return) { searching_ = false; return handle_nav(state_.selected_chat_index); }
            return false;
        }

        if (event == Event::ArrowDown) return handle_nav(state_.selected_chat_index + 1);
        if (event == Event::ArrowUp) return handle_nav(state_.selected_chat_index - 1);
        if (event == Event::PageDown) return handle_nav(state_.selected_chat_index + 5);
        if (event == Event::PageUp) return handle_nav(state_.selected_chat_index - 5);
        if (event == Event::Home) return handle_nav(0);
        if (event == Event::End) return handle_nav(state_.chats.size() - 1);
        if (event == Event::Character('/')) { searching_ = true; search_input->TakeFocus(); return true; }
        if (event == Event::Return) return handle_nav(state_.selected_chat_index);

        return false;
    });
}

} // namespace tgcli::ui
