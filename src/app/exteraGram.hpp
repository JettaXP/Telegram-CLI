#pragma once
// ── Telegram CLI — exteraGram API Integration ───────────────────────────────
// Fetches supporter/developer profiles from https://api.exteragram.app/api/v1/profiles
// and caches them for badge display next to usernames.

#include "State.hpp"
#include <map>
#include <string>
#include <functional>

namespace tgcli {

class exteraGram {
public:
    // Fetch all profiles from the API and populate the state cache
    static void fetch_profiles(AppState& state);

    // Check if a user ID has an exteraGram badge
    static bool has_badge(const AppState& state, int64_t user_id);

    // Get the badge symbol to display (Nerd Font icons)
    // Returns: "" (supporter checkmark), "" (developer wrench), or ""
    static std::string badge_symbol(const AppState& state, int64_t user_id);

    // Get the badge color code (for theme integration)
    static int badge_color(const AppState& state, int64_t user_id);

private:
    // HTTP GET helper using libcurl
    static std::string http_get(const std::string& url);

    // Parse JSON response into ExteraBadge map
    static std::map<int64_t, ExteraBadge> parse_profiles(const std::string& json_data);

    static constexpr const char* API_URL = "https://api.exteragram.app/api/v1/profiles";
};

} // namespace tgcli
