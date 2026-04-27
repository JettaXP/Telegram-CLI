// ── Telegram CLI — ExteraGram API Implementation ────────────────────────────
#include "ExteraGram.hpp"
#include "Config.hpp"
#include <nlohmann/json.hpp>
#include <curl/curl.h>
#include <iostream>

using json = nlohmann::json;

namespace tgcli {

// ── CURL write callback ─────────────────────────────────────────────────────
static size_t curl_write_cb(void* contents, size_t size, size_t nmemb, std::string* out) {
    size_t total = size * nmemb;
    out->append(static_cast<char*>(contents), total);
    return total;
}

std::string ExteraGram::http_get(const std::string& url) {
    std::string response;
    CURL* curl = curl_easy_init();
    if (!curl) return response;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "TelegramCLI/1.0");

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        response.clear();
    }

    curl_easy_cleanup(curl);
    return response;
}

std::map<int64_t, ExteraBadge> ExteraGram::parse_profiles(const std::string& json_data) {
    std::map<int64_t, ExteraBadge> profiles;

    try {
        auto arr = json::parse(json_data);
        if (!arr.is_array()) return profiles;

        for (const auto& item : arr) {
            if (!item.contains("id") || !item.contains("status")) continue;

            ExteraBadge badge;
            int64_t id = item["id"].get<int64_t>();
            badge.status = item["status"].get<std::string>();

            if (item.contains("badge")) {
                const auto& b = item["badge"];
                if (b.contains("documentId"))
                    badge.document_id = b["documentId"].get<int64_t>();
                if (b.contains("text"))
                    badge.text = b["text"].get<std::string>();
            }

            profiles[id] = badge;
        }
    } catch (const json::exception& e) {
        // Silently fail — badge display is non-critical
    }

    return profiles;
}

void ExteraGram::fetch_profiles(AppState& state) {
    std::string response = http_get(API_URL);
    if (response.empty()) return;

    auto profiles = parse_profiles(response);

    std::lock_guard<std::mutex> lock(state.mtx);
    state.extera_profiles = std::move(profiles);
}

bool ExteraGram::has_badge(const AppState& state, int64_t user_id) {
    return state.extera_profiles.count(user_id) > 0;
}

std::string ExteraGram::badge_symbol(const AppState& state, int64_t user_id) {
    auto it = state.extera_profiles.find(user_id);
    if (it == state.extera_profiles.end()) return "";

    // Use standard unicode arrow for supporters
    if (it->second.status == "DEVELOPER") {
        return "\xE2\x9A\x99";  // gear icon ⚙
    } else {
        return "\xE2\x9E\xA4";  // arrow icon ➤
    }
}

int ExteraGram::badge_color(const AppState& state, int64_t user_id) {
    auto it = state.extera_profiles.find(user_id);
    if (it == state.extera_profiles.end()) return 0;

    const auto& theme = Config::instance().theme;
    if (it->second.status == "DEVELOPER") {
        return theme.badge_developer;  // gold
    }
    return theme.badge_supporter;      // cyan
}

} // namespace tgcli
