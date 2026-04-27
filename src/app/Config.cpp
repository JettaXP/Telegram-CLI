// ── Telegram CLI — Configuration Implementation ─────────────────────────────
#include "Config.hpp"
#include <fstream>
#include <iostream>
#include <cstdlib>

namespace tgcli {

Config::Config() {
    // Default paths
    const char* home = std::getenv("HOME");
    if (!home) home = "/tmp";

    config_dir = std::filesystem::path(home) / ".config" / "tgcli";
    config_file = config_dir / "config.ini";
    session_dir = config_dir / "session";
    tdlib_data_dir = (config_dir / "tdlib_data").string();
}

Config& Config::instance() {
    static Config cfg;
    return cfg;
}

void Config::ensure_dirs() const {
    std::filesystem::create_directories(config_dir);
    std::filesystem::create_directories(session_dir);
    std::filesystem::create_directories(tdlib_data_dir);
}

void Config::load() {
    ensure_dirs();

    if (!std::filesystem::exists(config_file)) {
        // First launch —  use hardcoded defaults, will be saved later
        return;
    }

    std::ifstream in(config_file);
    if (!in.is_open()) return;

    std::string line;
    while (std::getline(in, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;

        auto eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);

        // Trim whitespace
        auto trim = [](std::string& s) {
            size_t start = s.find_first_not_of(" \t\r\n");
            size_t end = s.find_last_not_of(" \t\r\n");
            s = (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
        };
        trim(key);
        trim(val);

        if (key == "api_id") api_id = std::stoi(val);
        else if (key == "api_hash") api_hash = val;
        else if (key == "phone_number") phone_number = val;
        else if (key == "account_name") account_name = val;
        else if (key == "theme") {
            theme_name = val;
            apply_theme(val);
        }
    }
}

void Config::save() const {
    ensure_dirs();

    std::ofstream out(config_file);
    if (!out.is_open()) return;

    out << "# Telegram CLI Configuration\n";
    out << "# Theme options: dark, nord, gruvbox\n\n";
    out << "api_id=" << api_id << "\n";
    out << "api_hash=" << api_hash << "\n";
    if (!phone_number.empty())
        out << "phone_number=" << phone_number << "\n";
    out << "account_name=" << account_name << "\n";
    out << "theme=" << theme_name << "\n";
}

void Config::apply_theme(const std::string& name) {
    if (name == "nord") {
        theme.status_fg = 255;
        theme.status_bg = 0;     // #2E3440
        theme.status_dot = 108;  // nord green
        theme.accent = 110;      // nord blue

        theme.chatlist_bg = 0;
        theme.chatlist_fg = 252;
        theme.chatlist_selected_bg = 237;
        theme.chatlist_selected_fg = 255;
        theme.chatlist_unread = 173; // nord orange
        theme.chatlist_group = 110;
        theme.chatlist_channel = 110;
        theme.chatlist_bot = 222;

        theme.chatview_bg = 0;
        theme.chatview_fg = 252;
        theme.chatview_sender = 110;
        theme.chatview_timestamp = 242;
        theme.chatview_reply_bar = 110;

        theme.input_bg = 237;
        theme.input_fg = 255;
        theme.input_border = 60;

        theme.badge_supporter = 110;
        theme.badge_developer = 222;
        theme.badge_premium = 139;

        theme.border_color = 238;
    } else if (name == "gruvbox") {
        theme.status_fg = 223;   // fg
        theme.status_bg = 235;   // bg0
        theme.status_dot = 142;  // gruvbox green
        theme.accent = 109;     // gruvbox blue

        theme.chatlist_bg = 235;
        theme.chatlist_fg = 223;
        theme.chatlist_selected_bg = 237;
        theme.chatlist_selected_fg = 229;
        theme.chatlist_unread = 208;
        theme.chatlist_group = 109;
        theme.chatlist_channel = 109;
        theme.chatlist_bot = 214;

        theme.chatview_bg = 235;
        theme.chatview_fg = 223;
        theme.chatview_sender = 109;
        theme.chatview_timestamp = 245;
        theme.chatview_reply_bar = 109;

        theme.input_bg = 236;
        theme.input_fg = 223;
        theme.input_border = 241;

        theme.badge_supporter = 109;
        theme.badge_developer = 214;
        theme.badge_premium = 175;

        theme.border_color = 239;
    } else {
        // Default "dark" theme — already set in struct defaults
        theme = Theme{};
    }
}

} // namespace tgcli
