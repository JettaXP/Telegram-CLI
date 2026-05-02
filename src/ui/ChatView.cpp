// ── Telegram CLI — Chat View Implementation ─────────────────────────────────
#include "ChatView.hpp"
#include "../app/Config.hpp"
#include "../app/exteraGram.hpp"
#include <ftxui/dom/elements.hpp>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>

using namespace ftxui;

namespace tgcli::ui {

ChatView::ChatView(AppState& state) : state_(state) {}

std::string ChatView::format_time(int32_t timestamp) {
    std::time_t t = timestamp;
    char buf[10];
    std::strftime(buf, sizeof(buf), "%H:%M", std::localtime(&t));
    return buf;
}

std::string ChatView::format_date(int32_t timestamp) {
    std::time_t t = timestamp;
    char buf[16];
    std::strftime(buf, sizeof(buf), "%d %b", std::localtime(&t));
    return buf;
}

Element ChatView::render_message(const MessageEntry& msg, bool selected) {
    auto& theme = Config::instance().theme;

    auto sender_elem = text(msg.sender_name) | bold | color(Color::Palette256(theme.chatview_sender));
    if (msg.sender_has_extera_badge) {
        sender_elem = hbox({ sender_elem, text(" ➤") | color(Color::Palette256(exteraGram::badge_color(state_, msg.sender_id))) });
    }

    Element content_elem;
    if (!msg.media_type.empty()) {
        std::string media_text = "[" + msg.media_type + "]";
        if (!msg.file_name.empty()) {
            media_text += " " + msg.file_name;
        }

        auto media_elem = text(media_text) | color(Color::Palette256(theme.accent));
        if (!msg.media_caption.empty()) {
            content_elem = vbox({ media_elem, paragraph(msg.media_caption) | flex });
        } else {
            content_elem = media_elem;
        }
    } else {
        content_elem = paragraph(msg.text) | flex;
    }

    auto bubble = vbox({
        hbox({ sender_elem, filler(), text(format_time(msg.date)) | dim }),
        separator() | dim,
        content_elem,
    }) | borderRounded | color(Color::Palette256(theme.chatview_fg));

    if (msg.is_outgoing) {
        bubble = hbox({ filler() | size(WIDTH, EQUAL, 8), bubble | flex });
    } else {
        bubble = hbox({ bubble | flex, filler() | size(WIDTH, EQUAL, 8) });
    }

    if (selected) {
        bubble = bubble | color(Color::Palette256(theme.accent));
    }

    return bubble;
}

Component ChatView::component() {
    return Renderer([this]() {
        std::lock_guard<std::mutex> lock(state_.mtx);
        auto& theme = Config::instance().theme;

        if (state_.selected_chat_id == 0) {
            return vbox({
                filler(),
                text("Select a chat to start messaging") | center | dim,
                text("Arrows / F3 to navigate") | center | dim,
                filler(),
            }) | flex;
        }

        std::string title = "Chat";
        for (const auto& c : state_.chats) {
            if (c.id == state_.selected_chat_id) {
                title = c.title;
                break;
            }
        }

        Elements msg_elements;
        int total = static_cast<int>(state_.messages.size());
        int height = box_.y_max - box_.y_min;
        int header_h = 3;
        int footer_h = 2;
        int view_size = std::max(1, height - header_h - footer_h);
        int max_start = std::max(0, total - view_size);
        state_.chatview_view_size = view_size;

        int max_offset = std::max(0, total - view_size);
        if (state_.scroll_offset <= 0) {
            state_.scroll_offset = 0;
        } else {
            state_.scroll_offset = std::clamp(state_.scroll_offset, 1, max_offset);
        }

        int start = (state_.scroll_offset == 0)
            ? max_offset
            : std::max(0, total - view_size - state_.scroll_offset);
        int end = std::min(total, start + view_size);

        auto header = hbox({
            text(" " + title) | bold,
            filler(),
            text("[F2] Info ") | dim,
        }) | bgcolor(Color::Palette256(theme.status_bg));

        for (int i = start; i < end; i++) {
            msg_elements.push_back(render_message(state_.messages[i], i == selected_msg_index_));
            msg_elements.push_back(text(" "));
        }

        if (msg_elements.empty()) {
            msg_elements.push_back(text("No messages yet") | dim | center);
        }

        auto footer = hbox({
            text(" [PgUp/PgDn] Scroll") | dim,
            filler(),
            text(" [Home/End] ") | dim,
            text("▼ Bottom") | bold | color(Color::Palette256(theme.accent)),
            text(" "),
        }) | bgcolor(Color::Palette256(theme.status_bg));

        return vbox({
            header,
            separator() | color(Color::Palette256(theme.border_color)),
            vbox(std::move(msg_elements)) | yframe | flex,
            footer,
        }) | flex | reflect(box_);
    }) | CatchEvent([this](Event event) {
        std::lock_guard<std::mutex> lock(state_.mtx);
        int total = static_cast<int>(state_.messages.size());
        int height = box_.y_max - box_.y_min;
        int header_h = 3;
        int footer_h = 2;
        int view_size = std::max(1, height - header_h - footer_h);
        int max_offset = std::max(0, total - view_size);
        state_.chatview_view_size = view_size;

        if (event.is_mouse()) {
            if (!box_.Contain(event.mouse().x, event.mouse().y)) {
                return false;
            }

            if (event.mouse().button == Mouse::WheelUp) {
                if (state_.scroll_offset == 0) {
                    state_.scroll_offset = 1;
                }
                state_.scroll_offset = std::min(state_.scroll_offset + 2, max_offset);
                return true;
            }
            if (event.mouse().button == Mouse::WheelDown) {
                state_.scroll_offset = std::max(state_.scroll_offset - 2, 0);
                return true;
            }
            if (event.mouse().button == Mouse::Left && event.mouse().motion == Mouse::Released) {
                if (event.mouse().y >= box_.y_max - footer_h - 1) {
                    state_.scroll_offset = 0;
                    return true;
                }
            }
        }

        if (event == Event::PageUp) {
            if (state_.scroll_offset == 0) {
                state_.scroll_offset = 1;
            }
            state_.scroll_offset = std::min(state_.scroll_offset + std::max(1, view_size / 4), max_offset);
            return true;
        }
        if (event == Event::PageDown) {
            state_.scroll_offset = std::max(state_.scroll_offset - std::max(1, view_size / 4), 0);
            return true;
        }
        if (event == Event::Home) {
            state_.scroll_offset = max_offset;
            return true;
        }
        if (event == Event::End) {
            state_.scroll_offset = 0;
            return true;
        }

        return false;
    });
}

} // namespace tgcli::ui
