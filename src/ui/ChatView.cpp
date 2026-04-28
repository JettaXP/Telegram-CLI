// ── Telegram CLI — Chat View Implementation ─────────────────────────────────
#include "ChatView.hpp"
#include "../app/Config.hpp"
#include "../app/exteraGram.hpp"
#include <ftxui/dom/elements.hpp>
#include <algorithm>
#include <ctime>

using namespace ftxui;

namespace tgcli::ui {

ChatView::ChatView(AppState& state) : state_(state) {}

std::string ChatView::format_time(int32_t timestamp) {
    std::time_t t = timestamp;
    char buf[10];
    std::strftime(buf, sizeof(buf), "%H:%M", std::localtime(&t));
    return buf;
}

Element ChatView::render_message(const MessageEntry& msg, bool selected) {
    auto& theme = Config::instance().theme;

    // Sender name
    auto sender_elem = text(msg.sender_name)
        | bold
        | color(Color::Palette256(theme.chatview_sender));

    if (msg.sender_has_extera_badge) {
        std::string badge = exteraGram::badge_symbol(state_, msg.sender_id);
        int badge_col = exteraGram::badge_color(state_, msg.sender_id);
        sender_elem = hbox({
            sender_elem,
            text(" " + badge) | color(Color::Palette256(badge_col)),
        });
    }

    if (msg.sender_is_premium) {
        sender_elem = hbox({
            sender_elem,
            text(" *") | color(Color::Palette256(theme.badge_premium)),
        });
    }

    // Time
    auto time_elem = text(format_time(msg.date)) | dim | color(Color::Palette256(theme.chatview_timestamp));

    // Content
    Element content_elem;
    if (!msg.media_type.empty()) {
        std::string media_indicator = "[" + msg.media_type + "]";
        if (!msg.file_name.empty()) media_indicator += " " + msg.file_name;
        
        auto media_elem = text(media_indicator) | color(Color::Palette256(theme.accent));

        if (!msg.media_caption.empty()) {
            content_elem = vbox({
                media_elem,
                paragraph(msg.media_caption) | color(Color::Palette256(theme.chatview_fg)),
            });
        } else {
            content_elem = media_elem;
        }
    } else {
        content_elem = paragraph(msg.text) | color(Color::Palette256(theme.chatview_fg));
    }

    // Compose
    auto header = hbox({
        sender_elem,
        filler(),
        time_elem,
    });

    auto bubble = vbox({
        header,
        separator() | dim,
        content_elem,
    }) | borderRounded;

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
                text("Select a chat to start messaging") | dim | center,
                text("Arrows / F3 / F4 to navigate") | dim | center,
                filler(),
            }) | flex;
        }

        std::string chat_title;
        for (const auto& chat : state_.chats) {
            if (chat.id == state_.selected_chat_id) { chat_title = chat.title; break; }
        }

        auto header = hbox({
            text(" " + chat_title) | bold | color(Color::Palette256(theme.chatview_sender)),
            filler(),
            text(" [F2] Info ") | dim,
        }) | bgcolor(Color::Palette256(theme.status_bg));

        Elements msg_elements;
        int total = static_cast<int>(state_.messages.size());
        int term_height = box_.y_max > 0 ? (box_.y_max - box_.y_min) : 30;
        int view_size = std::max(5, term_height / 3); 
        
        int start = std::max(0, total - view_size + state_.scroll_offset);
        int end = std::min(total, start + view_size);

        for (int i = start; i < end; i++) {
            msg_elements.push_back(render_message(state_.messages[i], i == selected_msg_index_));
            msg_elements.push_back(text(" "));
        }

        if (msg_elements.empty()) {
            msg_elements.push_back(text("No messages yet") | dim | center);
        }

        return vbox({
            header,
            separator() | color(Color::Palette256(theme.border_color)),
            vbox(std::move(msg_elements)) | yframe | flex,
        }) | flex | reflect(box_);
    }) | CatchEvent([this](Event event) {
        if (event.is_mouse()) {
            if (event.mouse().button == Mouse::WheelUp) {
                std::lock_guard<std::mutex> lock(state_.mtx);
                state_.scroll_offset = std::max(state_.scroll_offset - 3, -((int)state_.messages.size()));
                return true;
            }
            if (event.mouse().button == Mouse::WheelDown) {
                std::lock_guard<std::mutex> lock(state_.mtx);
                state_.scroll_offset = std::min(state_.scroll_offset + 3, 0);
                return true;
            }
        }
        if (event == Event::PageUp) {
            std::lock_guard<std::mutex> lock(state_.mtx);
            state_.scroll_offset = std::max(state_.scroll_offset - 10, -((int)state_.messages.size()));
            return true;
        }
        if (event == Event::PageDown) {
            std::lock_guard<std::mutex> lock(state_.mtx);
            state_.scroll_offset = std::min(state_.scroll_offset + 10, 0);
            return true;
        }
        if (event == Event::Home) {
            std::lock_guard<std::mutex> lock(state_.mtx);
            state_.scroll_offset = -((int)state_.messages.size());
            return true;
        }
        if (event == Event::End) {
            std::lock_guard<std::mutex> lock(state_.mtx);
            state_.scroll_offset = 0;
            return true;
        }
        return false;
    });
}

} // namespace tgcli::ui
