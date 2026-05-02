// ── Telegram CLI — Chat List Implementation ─────────────────────────────────
#include "ChatList.hpp"
#include "../app/Config.hpp"
#include "../app/exteraGram.hpp"
#include <algorithm>

using namespace ftxui;

namespace tgcli::ui {

namespace {

std::string normalize_text(std::string text) {
    for (char& ch : text) {
        unsigned char c = static_cast<unsigned char>(ch);
        if (c < 0x20 && ch != ' ') {
            ch = ' ';
        }
    }
    return text;
}

std::string ellipsize_utf8(const std::string& input, size_t max_chars) {
    if (max_chars == 0) {
        return "";
    }

    std::string text = normalize_text(input);
    size_t char_count = 0;
    size_t byte_pos = 0;

    while (byte_pos < text.size() && char_count < max_chars) {
        unsigned char c = static_cast<unsigned char>(text[byte_pos]);
        size_t char_len = 1;
        if ((c & 0x80) == 0x00) {
            char_len = 1;
        } else if ((c & 0xE0) == 0xC0) {
            char_len = 2;
        } else if ((c & 0xF0) == 0xE0) {
            char_len = 3;
        } else if ((c & 0xF8) == 0xF0) {
            char_len = 4;
        }

        if (byte_pos + char_len > text.size()) {
            break;
        }

        byte_pos += char_len;
        ++char_count;
    }

    if (byte_pos >= text.size()) {
        return text;
    }

    if (max_chars <= 3) {
        return text.substr(0, byte_pos);
    }
    return text.substr(0, byte_pos) + "...";
}

}  // namespace

ChatList::ChatList(AppState& state) : state_(state) {}

Component ChatList::component() {
    auto search_input = Input(&search_text_, "Search...");
    auto search_maybe = Maybe(search_input, &searching_);

    auto renderer = Renderer(search_maybe, [this, search_input] {
        std::lock_guard<std::mutex> lock(state_.mtx);
        auto& theme = Config::instance().theme;

        Elements items;

        items.push_back(
            hbox({
                text("  Chats") | bold | color(Color::Palette256(theme.accent)),
                filler(),
                text(std::to_string(state_.chats.size())) | dim,
                text(" "),
            }) | bgcolor(Color::Palette256(theme.chatlist_bg))
        );

        if (searching_) {
            items.push_back(
                hbox({
                    text(" / ") | color(Color::Palette256(theme.accent)),
                    search_input->Render() | flex,
                })
            );
        }

        items.push_back(separator() | color(Color::Palette256(theme.border_color)));

        int height = box_.y_max - box_.y_min;
        int max_visible = std::max(5, (height - 5) / 3);
        int start_idx = std::max(0, state_.selected_chat_index - max_visible / 2);

        int idx = 0;
        int rendered_count = 0;
        visible_chat_ids_.clear();

        for (const auto& chat : state_.chats) {
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

            if (idx < start_idx) {
                idx++;
                continue;
            }
            if (rendered_count >= max_visible) {
                break;
            }

            visible_chat_ids_.push_back({idx, chat.id});
            bool selected = (idx == state_.selected_chat_index);

            std::string pin = chat.is_pinned ? "📌 " : "  ";
            auto title_line = hbox({
                text(ellipsize_utf8(chat.title, 18)) | bold,
                filler(),
                state_.is_extera_supporter(chat.id) ? text(" ➤") | color(Color::Palette256(exteraGram::badge_color(state_, chat.id))) : text(""),
            });

            std::string preview = chat.last_message.empty() ? "No messages" : chat.last_message;
            auto row = vbox({
                hbox({
                    text(pin) | color(Color::Palette256(theme.chatlist_pinned)),
                    title_line | flex,
                    chat.unread_count > 0 ? text("[" + std::to_string(chat.unread_count) + "]") | color(Color::Palette256(theme.chatlist_unread)) : text(""),
                }),
                text(" " + ellipsize_utf8(preview, 24)) | dim,
            }) | size(WIDTH, EQUAL, 30);

            if (selected) {
                row = row | bgcolor(Color::Palette256(theme.chatlist_selected_bg));
            } else {
                row = row | bgcolor(Color::Palette256(theme.chatlist_bg));
            }

            items.push_back(row);
            items.push_back(separator() | dim);

            rendered_count++;
            idx++;
        }

        items.push_back(filler() | flex | bgcolor(Color::Palette256(theme.chatlist_bg)));

        items.push_back(
            vbox({
                separator() | dim,
                hbox({ text(" ↑/↓ Nav") | dim, filler(), text("PgUp/Dn Scroll ") | dim }),
            }) | bgcolor(Color::Palette256(theme.chatlist_bg))
        );

        return vbox(std::move(items)) | size(WIDTH, EQUAL, 30) | reflect(box_);
    });

    return CatchEvent(renderer, [this, search_input](Event event) {
        auto handle_nav = [&](int new_idx) {
            std::lock_guard<std::mutex> lock(state_.mtx);
            if (state_.chats.empty()) {
                return true;
            }
            state_.selected_chat_index = std::clamp(new_idx, 0, static_cast<int>(state_.chats.size()) - 1);
            state_.selected_chat_id = state_.chats[state_.selected_chat_index].id;
            if (on_select_) {
                on_select_(state_.selected_chat_id);
            }
            return true;
        };

        if (event.is_mouse()) {
            if (!box_.Contain(event.mouse().x, event.mouse().y)) {
                return false;
            }
            if (event.mouse().button == Mouse::Left && event.mouse().motion == Mouse::Released) {
                int local_y = event.mouse().y - box_.y_min - (searching_ ? 3 : 2);
                int clicked_visual_idx = local_y / 3;
                if (clicked_visual_idx >= 0 && clicked_visual_idx < static_cast<int>(visible_chat_ids_.size())) {
                    return handle_nav(visible_chat_ids_[clicked_visual_idx].first);
                }
                return true;
            }
            if (event.mouse().button == Mouse::WheelUp) {
                return handle_nav(state_.selected_chat_index - 1);
            }
            if (event.mouse().button == Mouse::WheelDown) {
                return handle_nav(state_.selected_chat_index + 1);
            }
            return true;
        }

        if (searching_) {
            if (event == Event::Escape) {
                searching_ = false;
                return true;
            }
            if (event == Event::Return) {
                searching_ = false;
                return handle_nav(state_.selected_chat_index);
            }
            return false;
        }

        if (event == Event::ArrowDown) return handle_nav(state_.selected_chat_index + 1);
        if (event == Event::ArrowUp) return handle_nav(state_.selected_chat_index - 1);
        if (event == Event::PageDown) return handle_nav(state_.selected_chat_index + 5);
        if (event == Event::PageUp) return handle_nav(state_.selected_chat_index - 5);
        if (event == Event::Home) return handle_nav(0);
        if (event == Event::End) return handle_nav(static_cast<int>(state_.chats.size()) - 1);
        if (event == Event::Character('/')) {
            searching_ = true;
            search_input->TakeFocus();
            return true;
        }
        if (event == Event::Return) return handle_nav(state_.selected_chat_index);

        return false;
    });
}

} // namespace tgcli::ui
