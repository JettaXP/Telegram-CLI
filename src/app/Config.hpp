#pragma once
// ── Telegram CLI — Configuration Manager ────────────────────────────────────
#include <string>
#include <filesystem>

namespace tgcli {

struct Theme {
    // Status bar
    int status_fg = 255;   // white
    int status_bg = 236;   // dark grey
    int status_dot = 76;   // green dot

    // Chat list
    int chatlist_bg = 234;
    int chatlist_fg = 252;
    int chatlist_selected_bg = 238;
    int chatlist_selected_fg = 255;
    int chatlist_unread = 208;   // orange badge
    int chatlist_group = 81;     // cyan for groups
    int chatlist_channel = 69;   // blue for channels
    int chatlist_bot = 220;      // yellow for bots
    int chatlist_pinned = 203;   // red pin

    // Chat view
    int chatview_bg = 235;
    int chatview_fg = 252;
    int chatview_sender = 117;   // light blue
    int chatview_timestamp = 242; // grey
    int chatview_reply_bar = 69;
    int chatview_reaction = 220;

    // Input bar
    int input_bg = 236;
    int input_fg = 255;
    int input_placeholder = 240;
    int input_border = 60;

    // Badges
    int badge_supporter = 81;    // cyan checkmark
    int badge_developer = 220;   // gold checkmark
    int badge_premium = 141;     // purple star

    // General
    int border_color = 240;
    int accent = 69;             // main accent blue
};

struct Config {
    int api_id = 0;
    std::string api_hash;
    std::string phone_number;
    std::string account_name = "default";
    std::string theme_name = "dark";
    std::string tdlib_data_dir;
    Theme theme;

    // Paths
    std::filesystem::path config_dir;
    std::filesystem::path config_file;
    std::filesystem::path session_dir;

    // Load/save
    void load();
    void save() const;
    void ensure_dirs() const;
    void apply_theme(const std::string& name);

    // Singleton access
    static Config& instance();

private:
    Config();
};

} // namespace tgcli
