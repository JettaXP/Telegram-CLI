#pragma once
// ── Telegram CLI — Global Application State ─────────────────────────────────
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <cstdint>
#include <map>

namespace tgcli {

// ── Connection state ────────────────────────────────────────────────────────
enum class ConnectionStatus {
    CONNECTING,
    UPDATING,
    READY,
    OFFLINE
};

// ── ExteraGram badge info ───────────────────────────────────────────────────
struct ExteraBadge {
    std::string status;   // "SUPPORTER", "DEVELOPER"
    int64_t document_id = 0;
    std::string text;     // optional badge text
};

// ── Chat entry ──────────────────────────────────────────────────────────────
struct ChatEntry {
    int64_t id = 0;
    std::string title;
    std::string last_message;
    std::string last_sender;
    int32_t last_date = 0;
    int32_t unread_count = 0;
    bool is_pinned = false;
    bool is_group = false;
    bool is_channel = false;
    bool is_bot = false;
    bool is_private = false;
    int64_t order = 0;
};

// ── Message entry ───────────────────────────────────────────────────────────
struct MessageEntry {
    int64_t id = 0;
    int64_t chat_id = 0;
    int64_t sender_id = 0;
    std::string sender_name;
    std::string text;
    int32_t date = 0;
    bool is_outgoing = false;
    bool is_edited = false;

    // Reply info
    int64_t reply_to_message_id = 0;
    std::string reply_preview;

    // Media indicators
    std::string media_type;   // "", "Photo", "Video", "File", "Sticker", "Voice", "Gift", "Stars"
    std::string media_caption;
    std::string file_name;

    // Reactions
    struct Reaction {
        std::string emoji;
        int32_t count = 0;
    };
    std::vector<Reaction> reactions;

    // User badges
    bool sender_is_premium = false;
    bool sender_has_extera_badge = false;
    std::string sender_extera_status;  // "SUPPORTER" / "DEVELOPER"
};

// ── User info ───────────────────────────────────────────────────────────────
struct UserInfo {
    int64_t id = 0;
    std::string first_name;
    std::string last_name;
    std::string username;
    std::string phone;
    std::string bio;
    bool is_premium = false;
    std::string status;   // "online", "offline", "recently", etc.

    // ExteraGram
    bool has_extera_badge = false;
    ExteraBadge extera_badge;
};

// ── Stars info ──────────────────────────────────────────────────────────────
struct StarTransaction {
    std::string id;
    int64_t amount = 0;
    int32_t date = 0;
    std::string source;   // "incoming", "outgoing", "purchase"
    std::string partner_name;
};

// ── Gift info ───────────────────────────────────────────────────────────────
struct GiftEntry {
    int64_t id = 0;
    std::string title;
    int64_t star_count = 0;
    bool is_nft = false;
    std::string rarity;      // "common", "rare", "unique"
    int32_t serial_number = 0;
    std::string from_user;
    int32_t date = 0;
};

// ── Auth state ──────────────────────────────────────────────────────────────
enum class AuthState {
    NONE,
    WAIT_PHONE,
    WAIT_QR_CODE,
    WAIT_CODE,
    WAIT_PASSWORD,
    WAIT_REGISTRATION,
    READY,
    LOGGING_OUT,
    CLOSED
};

// ── Main state container ────────────────────────────────────────────────────
struct AppState {
    mutable std::mutex mtx;

    // Auth
    AuthState auth_state = AuthState::NONE;
    std::string auth_hint;   // e.g. phone code pattern
    std::string qr_code_link; // tg:// link for QR code login
    bool use_qr_login = true; // prefer QR code login by default

    // Connection
    ConnectionStatus connection = ConnectionStatus::CONNECTING;

    // User
    UserInfo current_user;

    // Chat list
    std::vector<ChatEntry> chats;
    int selected_chat_index = 0;
    int64_t selected_chat_id = 0;

    // Messages for current chat
    std::vector<MessageEntry> messages;
    int scroll_offset = 0;

    // Stars
    int64_t stars_balance = 0;
    std::vector<StarTransaction> star_transactions;

    // Gifts
    std::vector<GiftEntry> available_gifts;
    std::vector<GiftEntry> saved_gifts;

    // ExteraGram profiles cache
    std::map<int64_t, ExteraBadge> extera_profiles;

    // UI state
    bool show_info_panel = false;
    bool show_stars_panel = false;
    bool show_gifts_panel = false;

    // Input
    std::string compose_text;
    int64_t reply_to_msg_id = 0;
    int64_t edit_msg_id = 0;

    // Helpers
    std::string connection_text() const {
        switch (connection) {
            case ConnectionStatus::CONNECTING: return "Connecting";
            case ConnectionStatus::UPDATING:   return "Updating";
            case ConnectionStatus::READY:      return "Connected";
            case ConnectionStatus::OFFLINE:    return "Offline";
        }
        return "Unknown";
    }

    bool is_extera_supporter(int64_t user_id) const {
        auto it = extera_profiles.find(user_id);
        return it != extera_profiles.end();
    }

    std::string extera_status(int64_t user_id) const {
        auto it = extera_profiles.find(user_id);
        if (it != extera_profiles.end()) return it->second.status;
        return "";
    }
};

} // namespace tgcli
