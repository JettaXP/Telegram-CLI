// ── Telegram CLI — Info Panel Implementation ────────────────────────────────
#include "InfoPanel.hpp"
#include "../app/Config.hpp"
#include "../app/exteraGram.hpp"

using namespace ftxui;

namespace tgcli::ui {

InfoPanel::InfoPanel(AppState& state) : state_(state) {}

Component InfoPanel::component() {
    return Renderer([this]() {
        std::lock_guard<std::mutex> lock(state_.mtx);
        auto& theme = Config::instance().theme;

        if (state_.selected_chat_id == 0) {
            return vbox({
                hbox({
                    text(" Settings") | bold | color(Color::Palette256(theme.accent)),
                    filler(),
                    text(" ×") | dim,
                }),
                separator() | color(Color::Palette256(theme.border_color)),
                text(""),
                text("  Themes:") | bold,
                text("   :theme dark"),
                text("   :theme nord"),
                text("   :theme gruvbox"),
                text(""),
                text("  Config:") | bold,
                paragraph("   ~/.config/tgcli/config.ini") | dim,
                filler(),
                text("  [F2] Close panel") | dim,
            }) | size(WIDTH, EQUAL, 24);
        }

        const ChatEntry* chat = nullptr;
        for (const auto& c : state_.chats) {
            if (c.id == state_.selected_chat_id) {
                chat = &c;
                break;
            }
        }

        if (!chat) {
            return vbox({
                hbox({
                    text(" Info") | bold,
                    filler(),
                    text(" ×") | dim,
                }),
                text("  Loading...") | dim,
                filler(),
            }) | size(WIDTH, EQUAL, 24);
        }

        Elements info_items;
        info_items.push_back(hbox({
            text(" Info") | bold | color(Color::Palette256(theme.accent)),
            filler(),
            text(" ×") | dim,
        }));
        info_items.push_back(separator() | color(Color::Palette256(theme.border_color)));

        std::string type_label;
        if (chat->is_channel) type_label = "Channel";
        else if (chat->is_group) type_label = "Group";
        else if (chat->is_bot) type_label = "Bot";
        else type_label = "Private Chat";

        info_items.push_back(text(""));
        info_items.push_back(text("  " + chat->title) | bold);
        info_items.push_back(text("  " + type_label) | dim);

        info_items.push_back(text(""));
        info_items.push_back(hbox({
            text("  ID: ") | dim,
            text(std::to_string(chat->id)) | color(Color::Palette256(theme.chatview_sender)),
        }));

        if (!state_.selected_chat_details.username.empty()) {
            info_items.push_back(hbox({
                text("  User: ") | dim,
                text("@" + state_.selected_chat_details.username) | color(Color::Palette256(theme.accent)),
            }));
        }

        if (!state_.selected_chat_details.phone.empty()) {
            info_items.push_back(hbox({
                text("  Phone: ") | dim,
                text("+" + state_.selected_chat_details.phone) | color(Color::Palette256(theme.chatlist_fg)),
            }));
        }

        if (!state_.selected_chat_details.bio.empty()) {
            info_items.push_back(text(""));
            info_items.push_back(text("  Bio:") | dim);
            info_items.push_back(paragraph("   " + state_.selected_chat_details.bio));
        }

        if (!state_.selected_chat_details.description.empty()) {
            info_items.push_back(text(""));
            info_items.push_back(text("  About:") | dim);
            info_items.push_back(paragraph("   " + state_.selected_chat_details.description));
        }

        if (chat->is_private) {
            int64_t user_id = chat->id;
            if (exteraGram::has_badge(state_, user_id)) {
                std::string badge = exteraGram::badge_symbol(state_, user_id);
                int badge_col = exteraGram::badge_color(state_, user_id);
                std::string status = state_.extera_status(user_id);
                if (status == "SUPPORTER") status = "Supporter";
                else if (status == "DEVELOPER") status = "Developer";

                info_items.push_back(text(""));
                info_items.push_back(
                    hbox({
                        text("  " + badge + " ") | color(Color::Palette256(badge_col)),
                        text("exteraGram " + status) | color(Color::Palette256(badge_col)),
                    })
                );
            }
        }

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

        info_items.push_back(text("  Actions:") | bold);
        info_items.push_back(text("   :stars  View Stars") | dim);
        info_items.push_back(text("   :gifts  View Gifts") | dim);
        info_items.push_back(text("   :mute   Mute Chat") | dim);
        info_items.push_back(text("   Press 'p' to Pin/Unpin in this panel") | dim);
        info_items.push_back(text("   F2 / Esc Close panel") | dim);

        info_items.push_back(text(""));
        // Show pinned status
        info_items.push_back(hbox({
            text("  Pinned: ") | dim,
            text(chat->is_pinned ? "Yes" : "No") | color(Color::Palette256(theme.chatlist_pinned)),
        }));

        info_items.push_back(filler());

        return vbox(std::move(info_items))
            | size(WIDTH, EQUAL, 24);
    }) | CatchEvent([this](Event event) {
        // Toggle pin/unpin when in info panel with 'p' or 'P'
        if (event == Event::Character('p') || event == Event::Character('P')) {
            std::lock_guard<std::mutex> lock(state_.mtx);
            for (auto& c : state_.chats) {
                if (c.id == state_.selected_chat_id) {
                    c.is_pinned = !c.is_pinned;
                    break;
                }
            }
            // Re-sort pinned chats first
            std::sort(state_.chats.begin(), state_.chats.end(), [](const ChatEntry& a, const ChatEntry& b){
                if (a.is_pinned != b.is_pinned) return a.is_pinned > b.is_pinned;
                return a.order > b.order;
            });
            return true;
        }
        return false;
    });
}


} // namespace tgcli::ui
