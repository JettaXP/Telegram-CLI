// ── Telegram CLI — Stars Panel Implementation ───────────────────────────────
#include "StarsPanel.hpp"
#include "../app/Config.hpp"
#include <ctime>

using namespace ftxui;

namespace tgcli::ui {

StarsPanel::StarsPanel(AppState& state) : state_(state) {}

Component StarsPanel::component() {
    return Renderer([this]() {
        std::lock_guard<std::mutex> lock(state_.mtx);
        auto& theme = Config::instance().theme;

        // Balance display
        auto balance = text(" " + std::to_string(state_.stars_balance) + " Stars")
            | bold | color(Color::Palette256(220)); // gold

        // Transaction list
        Elements txn_items;
        for (const auto& txn : state_.star_transactions) {
            std::string dir = (txn.amount > 0) ? "+" : "";
            auto amount_color = (txn.amount > 0)
                ? Color::Palette256(76)    // green
                : Color::Palette256(196);  // red

            // Format date
            time_t t = static_cast<time_t>(txn.date);
            struct tm* tm_info = localtime(&t);
            char date_buf[11];
            strftime(date_buf, sizeof(date_buf), "%m/%d %H:%M", tm_info);

            txn_items.push_back(hbox({
                text(" " + std::string(date_buf)) | dim,
                text("  "),
                text(dir + std::to_string(txn.amount)) | color(amount_color) | bold,
                text("  "),
                text(txn.partner_name) | flex,
            }));
        }

        if (txn_items.empty()) {
            txn_items.push_back(text("  No transactions yet") | dim);
        }

        return vbox({
            text(" Stars") | bold | color(Color::Palette256(theme.accent)),
            separator(),
            text(""),
            hbox({text("  Balance: "), balance}),
            text(""),
            separator(),
            text("  Recent Transactions") | bold,
            text(""),
            vbox(std::move(txn_items)) | yframe | flex,
            separator(),
            text("  :stars close  |  :buy  Purchase Stars") | dim,
        }) | size(WIDTH, EQUAL, 30)
           | bgcolor(Color::Palette256(theme.chatlist_bg))
           | border
           | color(Color::Palette256(theme.border_color));
    });
}

} // namespace tgcli::ui
