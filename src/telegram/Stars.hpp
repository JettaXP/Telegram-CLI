#pragma once
// ── Telegram CLI — Stars & Payments ─────────────────────────────────────────
#include "Client.hpp"

namespace tgcli {

class Stars {
public:
    Stars(TdClient& client);

    // Get current Stars balance
    void fetch_balance();

    // Get transaction history
    void fetch_transactions(int limit = 20);

    // Send Stars to a user (via gift or direct)
    void send_stars(int64_t user_id, int64_t amount);

    // Get a URL to purchase more Stars
    std::string get_purchase_url();

private:
    TdClient& client_;
};

} // namespace tgcli
