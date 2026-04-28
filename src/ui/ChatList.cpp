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

        int max_visible = box_.y_max > 0 ? std::max(5, (box_.y_max - box_.y_min) / 3) : 15;
        int start_idx = std::max(0, state_.selected_chat_index - max_visible / 2);

        int idx = 0;
        int rendered = 0;
        visible_chat_ids_.clear();

        for (const auto& chat : state_.chats) {
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

            std::string icon;
            int icon_color = theme.chatlist_fg;
            if (chat.is_channel) icon = "CHA";
            else if (chat.is_group) icon = "GRP";
            else if (chat.is_bot) icon = "BOT";
            else icon = "USR";

            std::string pin = chat.is_pinned ? "P " : "  ";

            auto title_elem = text(chat.title) | bold | color(Color::Palette256(
                    selected ? theme.chatlist_selected_fg : theme.chatlist_fg
                )) | xflex;

            if (state_.is_extera_supporter(chat.id)) {
                title_elem = hbox({
                    title_elem,
                    text(" ➤") | color(Color::Palette256(exteraGram::badge_color(state_, chat.id))),
                });
            }

            auto unread_elem = text("");
            if (chat.unread_count > 0) {
                unread_elem = text("[" + std::to_string(chat.unread_count) + "]")
                    | bold | color(Color::Palette256(theme.chatlist_unread));
            }

            auto row = vbox({
                hbox({
                    text(pin) | color(Color::Palette256(theme.chatlist_pinned)),
                    title_elem | flex,
                    unread_elem,
                }),
                text(" " + chat.last_message) | dim | size(WIDTH, LESS_THAN, 28)
            }) | size(WIDTH, EQUAL, 30);

            if (selected) row = row | bgcolor(Color::Palette256(theme.chatlist_selected_bg));
            else row = row | bgcolor(Color::Palette256(theme.chatlist_bg));

            items.push_back(row);
            items.push_back(separator() | dim | color(Color::Palette256(theme.border_color)));
            
            rendered++;
            idx++;
        }

        items.push_back(filler() | flex | bgcolor(Color::Palette256(theme.chatlist_bg)));

        return vbox(std::move(items))
            | size(WIDTH, EQUAL, 30)
            | flex
            | reflect(box_);
    });

    return CatchEvent(renderer, [this, search_input](Event event) {
        if (event.is_mouse()) {
            if (!box_.Contain(event.mouse().x, event.mouse().y)) return false;
            if (event.mouse().button == Mouse::Left && event.mouse().motion == Mouse::Released) {
                int start_y = searching_ ? 3 : 2; 
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
                if (state_.selected_chat_index < static_cast<int>(state_.chats.size()) - 1) state_.selected_chat_index++;
                return true;
            }
            if (event.mouse().button == Mouse::WheelUp) {
                std::lock_guard<std::mutex> lock(state_.mtx);
                if (state_.selected_chat_index > 0) state_.selected_chat_index--;
                return true;
            }
            return true;
        }

        if (searching_) {
            if (event == Event::Escape) { searching_ = false; return true; }
            if (event == Event::Return) {
                std::lock_guard<std::mutex> lock(state_.mtx);
                if (!state_.chats.empty()) {
                    searching_ = false;
                    state_.selected_chat_id = state_.chats[state_.selected_chat_index].id;
                    if (on_select_) on_select_(state_.selected_chat_id);
                }
                return true;
            }
            return false;
        }

        if (event == Event::ArrowDown || event == Event::F4 || event == Event::Character('j')) {
            std::lock_guard<std::mutex> lock(state_.mtx);
            if (state_.selected_chat_index < static_cast<int>(state_.chats.size()) - 1) {
                state_.selected_chat_index++;
                state_.selected_chat_id = state_.chats[state_.selected_chat_index].id;
                if (on_select_) on_select_(state_.selected_chat_id);
            }
            return true;
        }
        if (event == Event::ArrowUp || event == Event::F3 || event == Event::Character('k')) {
            std::lock_guard<std::mutex> lock(state_.mtx);
            if (state_.selected_chat_index > 0) {
                state_.selected_chat_index--;
                state_.selected_chat_id = state_.chats[state_.selected_chat_index].id;
                if (on_select_) on_select_(state_.selected_chat_id);
            }
            return true;
        }
        if (event == Event::PageDown) {
            std::lock_guard<std::mutex> lock(state_.mtx);
            state_.selected_chat_index = std::min(static_cast<int>(state_.chats.size()) - 1, state_.selected_chat_index + 10);
            state_.selected_chat_id = state_.chats[state_.selected_chat_index].id;
            if (on_select_) on_select_(state_.selected_chat_id);
            return true;
        }
        if (event == Event::PageUp) {
            std::lock_guard<std::mutex> lock(state_.mtx);
            state_.selected_chat_index = std::max(0, state_.selected_chat_index - 10);
            state_.selected_chat_id = state_.chats[state_.selected_chat_index].id;
            if (on_select_) on_select_(state_.selected_chat_id);
            return true;
        }
        if (event == Event::Home) {
            std::lock_guard<std::mutex> lock(state_.mtx);
            state_.selected_chat_index = 0;
            if (!state_.chats.empty()) {
                state_.selected_chat_id = state_.chats[0].id;
                if (on_select_) on_select_(state_.selected_chat_id);
            }
            return true;
        }
        if (event == Event::End) {
            std::lock_guard<std::mutex> lock(state_.mtx);
            if (!state_.chats.empty()) {
                state_.selected_chat_index = state_.chats.size() - 1;
                state_.selected_chat_id = state_.chats.back().id;
                if (on_select_) on_select_(state_.selected_chat_id);
            }
            return true;
        }
        if (event == Event::Character('/')) {
            searching_ = true;
            search_input->TakeFocus();
            return true;
        }
        if (event == Event::Return) {
            std::lock_guard<std::mutex> lock(state_.mtx);
            if (!state_.chats.empty()) {
                state_.selected_chat_id = state_.chats[state_.selected_chat_index].id;
                if (on_select_) on_select_(state_.selected_chat_id);
            }
            return true;
        }
        return false;
    });
}

} // namespace tgcli::ui
