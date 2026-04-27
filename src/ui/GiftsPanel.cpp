// ── Telegram CLI — Gifts Panel Implementation ───────────────────────────────
#include "GiftsPanel.hpp"
#include "../app/Config.hpp"

using namespace ftxui;

namespace tgcli::ui {

GiftsPanel::GiftsPanel(AppState& state) : state_(state) {}

Component GiftsPanel::component() {
    return Renderer([this]() {
        std::lock_guard<std::mutex> lock(state_.mtx);
        auto& theme = Config::instance().theme;

        // Filter tabs
        auto tab_all = text(" All ")
            | (filter_ == "all" ? bold : dim)
            | (filter_ == "all"
                ? bgcolor(Color::Palette256(theme.accent))
                : bgcolor(Color::Palette256(theme.chatlist_bg)));

        auto tab_gifts = text(" Gifts ")
            | (filter_ == "gifts" ? bold : dim)
            | (filter_ == "gifts"
                ? bgcolor(Color::Palette256(theme.accent))
                : bgcolor(Color::Palette256(theme.chatlist_bg)));

        auto tab_nfts = text(" NFTs ")
            | (filter_ == "nfts" ? bold : dim)
            | (filter_ == "nfts"
                ? bgcolor(Color::Palette256(theme.accent))
                : bgcolor(Color::Palette256(theme.chatlist_bg)));

        // Gift items
        Elements gift_items;
        for (size_t i = 0; i < state_.saved_gifts.size(); i++) {
            const auto& gift = state_.saved_gifts[i];

            // Filter
            if (filter_ == "gifts" && gift.is_nft) continue;
            if (filter_ == "nfts" && !gift.is_nft) continue;

            bool sel = (static_cast<int>(i) == selected_);

            // Icon
            std::string icon = gift.is_nft
                ? "\xEF\x8C\xA8"  // nf-mdi-diamond (NFT)
                : "\xEF\x81\xAB"; // nf-fa-gift

            // Rarity color
            int rarity_col = theme.chatview_fg;
            if (gift.rarity == "rare") rarity_col = theme.accent;
            else if (gift.rarity == "unique") rarity_col = theme.badge_developer;

            auto row = hbox({
                text(" " + icon + " ") | color(Color::Palette256(rarity_col)),
                text(gift.title.empty() ? ("Gift #" + std::to_string(gift.id)) : gift.title) | flex,
                text(std::to_string(gift.star_count) + " S ") | dim,
            });

            if (gift.is_nft && gift.serial_number > 0) {
                row = vbox({
                    row,
                    hbox({
                        text("   #" + std::to_string(gift.serial_number)) | dim,
                        text(" "),
                        text(gift.rarity) | color(Color::Palette256(rarity_col)),
                    }),
                });
            }

            if (sel) {
                row = row | bgcolor(Color::Palette256(theme.chatlist_selected_bg));
            }

            gift_items.push_back(row);
            gift_items.push_back(separator() | dim);
        }

        if (gift_items.empty()) {
            gift_items.push_back(text("  No gifts yet") | dim);
        }

        return vbox({
            text(" Gifts & Collectibles") | bold | color(Color::Palette256(theme.accent)),
            separator(),
            hbox({tab_all, text(" "), tab_gifts, text(" "), tab_nfts}),
            separator(),
            vbox(std::move(gift_items)) | yframe | flex,
            separator(),
            text("  :gifts close  |  :send gift  |  :nft convert") | dim,
        }) | size(WIDTH, EQUAL, 30)
           | bgcolor(Color::Palette256(theme.chatlist_bg))
           | border
           | color(Color::Palette256(theme.border_color));
    }) | CatchEvent([this](Event event) {
        if (event == Event::Character('1')) { filter_ = "all"; return true; }
        if (event == Event::Character('2')) { filter_ = "gifts"; return true; }
        if (event == Event::Character('3')) { filter_ = "nfts"; return true; }
        return false;
    });
}

} // namespace tgcli::ui
