// ── Telegram CLI — Chat View Implementation ─────────────────────────────────
#include "ChatView.hpp"
#include "../app/Config.hpp"
#include "../app/ExteraGram.hpp"

#include <ctime>
#include <sstream>
#include <iomanip>

using namespace ftxui;

namespace tgcli::ui {

ChatView::ChatView(AppState& state) : state_(state) {}

std::string ChatView::format_time(int32_t timestamp) {
    if (timestamp == 0) return "";
    time_t t = static_cast<time_t>(timestamp);
    struct tm* tm_info = localtime(&t);
    char buf[6];
    strftime(buf, sizeof(buf), "%H:%M", tm_info);
    return std::string(buf);
}

Element ChatView::render_message(const MessageEntry& msg, bool selected) {
    auto& theme = Config::instance().theme;

    // ── Sender name with badges ─────────────────────────────────────────
    auto sender_elem = text(msg.sender_name)
        | bold
        | color(Color::Palette256(theme.chatview_sender));

    // ExteraGram badge (галочка)
    if (msg.sender_has_extera_badge) {
        std::string badge = ExteraGram::badge_symbol(state_, msg.sender_id);
        int badge_col = ExteraGram::badge_color(state_, msg.sender_id);
        sender_elem = hbox({
            sender_elem,
            text(" " + badge) | color(Color::Palette256(badge_col)),
        });
    }

    // Premium badge
    if (msg.sender_is_premium) {
        sender_elem = hbox({
            sender_elem,
            text(" \xE2\x98\x85") | color(Color::Palette256(theme.badge_premium)), // ★
        });
    }

    // ── Timestamp ───────────────────────────────────────────────────────
    auto time_str = format_time(msg.date);
    auto time_elem = text(time_str)
        | dim
        | color(Color::Palette256(theme.chatview_timestamp));

    // ── Reply indicator ─────────────────────────────────────────────────
    Element reply_elem = text("");
    if (msg.reply_to_message_id != 0) {
        std::string preview = msg.reply_preview.empty() ? "..." : msg.reply_preview;
        if (preview.length() > 40) preview = preview.substr(0, 37) + "...";
        reply_elem = hbox({
            text("\xE2\x94\x82 ") | color(Color::Palette256(theme.chatview_reply_bar)), // │
            text(preview) | dim,
        });
    }

    // ── Message content ─────────────────────────────────────────────────
    Element content_elem;
    if (!msg.media_type.empty()) {
        std::string media_indicator;
        if (msg.media_type == "Photo")   media_indicator = "[Photo]";
        else if (msg.media_type == "Video")   media_indicator = "[Video]";
        else if (msg.media_type == "File")    media_indicator = "[File: " + msg.file_name + "]";
        else if (msg.media_type == "Sticker") media_indicator = "[Sticker " + msg.text + "]";
        else if (msg.media_type == "Voice")   media_indicator = "[Voice Message]";
        else if (msg.media_type == "GIF")     media_indicator = "[GIF]";
        else media_indicator = "[" + msg.media_type + "]";

        auto media_elem = text(media_indicator)
            | color(Color::Palette256(theme.accent));

        if (!msg.media_caption.empty()) {
            content_elem = vbox({
                media_elem,
                text(msg.media_caption) | color(Color::Palette256(theme.chatview_fg)),
            });
        } else {
            content_elem = media_elem;
        }
    } else {
        content_elem = paragraph(msg.text)
            | color(Color::Palette256(theme.chatview_fg));
    }

    // ── Reactions ───────────────────────────────────────────────────────
    Element reactions_elem = text("");
    if (!msg.reactions.empty()) {
        Elements reaction_parts;
        for (const auto& r : msg.reactions) {
            reaction_parts.push_back(
                text(r.emoji + " " + std::to_string(r.count) + " ")
                | color(Color::Palette256(theme.chatview_reaction))
            );
        }
        reactions_elem = hbox(std::move(reaction_parts));
    }

    // ── Edited indicator ────────────────────────────────────────────────
    Element edited_elem = text("");
    if (msg.is_edited) {
        edited_elem = text(" (edited)") | dim;
    }

    // ── Compose message bubble ──────────────────────────────────────────
    auto header = hbox({
        sender_elem,
        filler(),
        edited_elem,
        text(" "),
        time_elem,
    });

    Elements msg_parts;
    if (msg.reply_to_message_id != 0) {
        msg_parts.push_back(reply_elem);
    }
    msg_parts.push_back(header);
    msg_parts.push_back(content_elem);
    if (!msg.reactions.empty()) {
        msg_parts.push_back(reactions_elem);
    }

    auto bubble = vbox(std::move(msg_parts));

    // Outgoing messages get a slight visual distinction
    if (msg.is_outgoing) {
        bubble = hbox({
            filler() | size(WIDTH, EQUAL, 4),
            bubble | flex,
        });
    } else {
        bubble = hbox({
            bubble | flex,
            filler() | size(WIDTH, EQUAL, 4),
        });
    }

    // Selection highlight
    if (selected) {
        bubble = bubble | bgcolor(Color::Palette256(theme.chatlist_selected_bg));
    }

    return bubble;
}

Component ChatView::component() {
    return Renderer([this]() {
        std::lock_guard<std::mutex> lock(state_.mtx);
        auto& theme = Config::instance().theme;

        if (state_.selected_chat_id == 0) {
            // No chat selected
            return vbox({
                filler(),
                text("Select a chat to start messaging") | dim | center,
                text("Use arrows or j/k to navigate") | dim | center,
                filler(),
            }) | flex | bgcolor(Color::Palette256(theme.chatview_bg));
        }

        // Find current chat title
        std::string chat_title;
        for (const auto& chat : state_.chats) {
            if (chat.id == state_.selected_chat_id) {
                chat_title = chat.title;
                break;
            }
        }

        // Chat header
        auto header = hbox({
            text(" "),
            text(chat_title) | bold | color(Color::Palette256(theme.chatview_sender)),
            filler(),
            text(" [i] Info ") | dim,
        }) | bgcolor(Color::Palette256(theme.status_bg));

        // Messages
        Elements msg_elements;
        int total = static_cast<int>(state_.messages.size());
        int view_size = 50;
        int start = std::max(0, total - view_size + state_.scroll_offset);
        int end = std::min(total, start + view_size);

        for (int i = start; i < end; i++) {
            bool selected = (i == selected_msg_index_);
            msg_elements.push_back(render_message(state_.messages[i], selected));
            msg_elements.push_back(text("") | size(HEIGHT, EQUAL, 1)); // spacing
        }

        if (msg_elements.empty()) {
            msg_elements.push_back(
                text("No messages yet") | dim | center
            );
        }

        auto messages_view = vbox(std::move(msg_elements))
            | yframe  // scrollable
            | flex;

        return vbox({
            header,
            separator() | color(Color::Palette256(theme.border_color)),
            messages_view,
        }) | flex;
    }) | CatchEvent([this](Event event) {
        // Handle mouse wheel for messages
        if (event.is_mouse()) {
            if (event.mouse().button == Mouse::WheelUp) {
                std::lock_guard<std::mutex> lock(state_.mtx);
                int total = static_cast<int>(state_.messages.size());
                state_.scroll_offset = std::max(state_.scroll_offset - 5, -total);
                return true;
            }
            if (event.mouse().button == Mouse::WheelDown) {
                std::lock_guard<std::mutex> lock(state_.mtx);
                state_.scroll_offset = std::min(state_.scroll_offset + 5, 0);
                return true;
            }
        }
        
        // Scroll
        if (event == Event::PageUp) {
            std::lock_guard<std::mutex> lock(state_.mtx);
            int total = static_cast<int>(state_.messages.size());
            state_.scroll_offset = std::max(state_.scroll_offset - 10, -total);
            return true;
        }
        if (event == Event::PageDown) {
            std::lock_guard<std::mutex> lock(state_.mtx);
            state_.scroll_offset = std::min(state_.scroll_offset + 10, 0);
            return true;
        }

        // Message actions
        if (event == Event::Character('r')) {
            // Reply
            std::lock_guard<std::mutex> lock(state_.mtx);
            if (!state_.messages.empty()) {
                state_.reply_to_msg_id = state_.messages.back().id;
            }
            if (on_action_) on_action_("reply", state_.reply_to_msg_id);
            return true;
        }
        if (event == Event::Character('d')) {
            // Delete
            if (!state_.messages.empty()) {
                if (on_action_) on_action_("delete", state_.messages.back().id);
            }
            return true;
        }
        if (event == Event::Character('f')) {
            // Forward
            if (!state_.messages.empty()) {
                if (on_action_) on_action_("forward", state_.messages.back().id);
            }
            return true;
        }
        if (event == Event::Character('e')) {
            // Edit
            std::lock_guard<std::mutex> lock(state_.mtx);
            if (!state_.messages.empty() && state_.messages.back().is_outgoing) {
                state_.edit_msg_id = state_.messages.back().id;
                if (on_action_) on_action_("edit", state_.edit_msg_id);
            }
            return true;
        }

        return false;
    });
}

} // namespace tgcli::ui
