// ── Telegram CLI — Status Bar Implementation ────────────────────────────────
#include "StatusBar.hpp"
#include "../app/Config.hpp"
#include "../app/ExteraGram.hpp"

using namespace ftxui;

namespace tgcli::ui {

StatusBar::StatusBar(AppState& state) : state_(state) {}

Element StatusBar::render() {
    std::lock_guard<std::mutex> lock(state_.mtx);
    auto& theme = Config::instance().theme;

    // Connection indicator dot
    std::string dot;
    Color dot_color;
    switch (state_.connection) {
        case ConnectionStatus::READY:
            dot = "\xE2\x97\x8F"; // ●
            dot_color = Color::Palette256(theme.status_dot); // green
            break;
        case ConnectionStatus::CONNECTING:
        case ConnectionStatus::UPDATING:
            dot = "\xE2\x97\x8F"; // ●
            dot_color = Color::Palette256(220); // yellow
            break;
        case ConnectionStatus::OFFLINE:
            dot = "\xE2\x97\x8F"; // ●
            dot_color = Color::Palette256(196); // red
            break;
    }

    auto connection_indicator = hbox({
        text("[") | color(Color::Palette256(theme.status_fg)),
        text(dot) | color(dot_color),
        text("] ") | color(Color::Palette256(theme.status_fg)),
        text(state_.connection_text()) | color(Color::Palette256(theme.status_fg)),
    });

    // User name
    std::string user_display = state_.current_user.first_name;
    if (!state_.current_user.last_name.empty()) {
        user_display += " " + state_.current_user.last_name;
    }

    // ExteraGram badge for current user
    std::string badge_sym = ExteraGram::badge_symbol(state_, state_.current_user.id);
    int badge_col = ExteraGram::badge_color(state_, state_.current_user.id);

    auto user_section = hbox({
        text(user_display) | bold | color(Color::Palette256(theme.status_fg)),
    });

    // Add ExteraGram badge if user has one
    if (!badge_sym.empty()) {
        user_section = hbox({
            text(user_display) | bold | color(Color::Palette256(theme.status_fg)),
            text(" " + badge_sym) | color(Color::Palette256(badge_col)),
        });
    }

    // Premium indicator
    if (state_.current_user.is_premium) {
        user_section = hbox({
            user_section,
            text(" \xE2\x98\x85") | color(Color::Palette256(theme.badge_premium)), // ★
        });
    }

    // App name / brand
    auto brand = text(" TGCLI ") | bold | color(Color::Palette256(theme.accent));

    return hbox({
        brand,
        separator(),
        text(" ") ,
        connection_indicator,
        text("  |  "),
        user_section,
        filler(),
    }) | bgcolor(Color::Palette256(theme.status_bg));
}

} // namespace tgcli::ui
