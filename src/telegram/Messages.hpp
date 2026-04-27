#pragma once
// ── Telegram CLI — Messaging ────────────────────────────────────────────────
#include "Client.hpp"
#include <string>

namespace tgcli {

class Messages {
public:
    Messages(TdClient& client);

    // Load chat list
    void load_chats(int limit = 50);

    // Load message history for a chat
    void load_history(int64_t chat_id, int limit = 30, int64_t from_message_id = 0);

    // Send a text message
    void send_text(int64_t chat_id, const std::string& text,
                   int64_t reply_to_msg_id = 0);

    // Send a file (path on disk)
    void send_file(int64_t chat_id, const std::string& file_path);

    // Edit a message
    void edit_text(int64_t chat_id, int64_t message_id, const std::string& new_text);

    // Delete a message
    void delete_message(int64_t chat_id, int64_t message_id, bool revoke = true);

    // Forward a message
    void forward(int64_t from_chat_id, int64_t to_chat_id, int64_t message_id);

    // Add a reaction
    void add_reaction(int64_t chat_id, int64_t message_id, const std::string& emoji);

    // Mark chat as read
    void mark_read(int64_t chat_id, int64_t last_message_id);

    // Fetch full info for side panel (bio, username, etc)
    void fetch_chat_full_info(int64_t chat_id);

    // Get user display name
    std::string get_user_name(int64_t user_id);

private:
    TdClient& client_;
};

} // namespace tgcli
