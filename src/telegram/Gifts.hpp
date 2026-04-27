#pragma once
// ── Telegram CLI — Gifts & NFT Collectibles ─────────────────────────────────
#include "Client.hpp"

namespace tgcli {

class Gifts {
public:
    Gifts(TdClient& client);

    // Get available gifts that can be sent
    void fetch_available_gifts();

    // Get owned/saved gifts
    void fetch_saved_gifts();

    // Send a gift to a user
    void send_gift(int64_t user_id, int64_t gift_id);

    // Convert a gift to NFT collectible
    void convert_to_nft(int64_t gift_id);

    // Transfer an NFT to another user
    void transfer_nft(int64_t gift_id, int64_t to_user_id);

private:
    TdClient& client_;
};

} // namespace tgcli
