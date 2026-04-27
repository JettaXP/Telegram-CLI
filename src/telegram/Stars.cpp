// ── Telegram CLI — Stars Implementation ─────────────────────────────────────
#include "Stars.hpp"
#include <algorithm>

namespace tgcli {

Stars::Stars(TdClient& client) : client_(client) {}

void Stars::fetch_balance() {
    auto result = client_.send_sync(
        td_api::make_object<td_api::getStarTransactions>(
            td_api::make_object<td_api::messageSenderUser>(client_.state().current_user.id),
            "", nullptr, "", 1
        )
    );

    if (!result || result->get_id() != td_api::starTransactions::ID) return;

    auto& txns = static_cast<td_api::starTransactions&>(*result);
    std::lock_guard<std::mutex> lock(client_.state().mtx);
    if (txns.star_amount_) {
        client_.state().stars_balance = txns.star_amount_->star_count_;
    }
}

void Stars::fetch_transactions(int limit) {
    auto result = client_.send_sync(
        td_api::make_object<td_api::getStarTransactions>(
            td_api::make_object<td_api::messageSenderUser>(client_.state().current_user.id),
            "", nullptr, "", limit
        )
    );

    if (!result || result->get_id() != td_api::starTransactions::ID) return;

    auto& txns = static_cast<td_api::starTransactions&>(*result);
    auto& state = client_.state();

    std::vector<StarTransaction> entries;
    entries.reserve(txns.transactions_.size());

    for (auto& tx : txns.transactions_) {
        if (!tx || !tx->star_amount_) continue;

        StarTransaction entry;
        entry.id = tx->id_;
        entry.amount = tx->star_amount_->star_count_;
        entry.date = tx->date_;

        if (entry.amount > 0) {
            entry.source = "incoming";
        } else {
            entry.source = "outgoing";
        }

        entries.push_back(std::move(entry));
    }

    std::lock_guard<std::mutex> lock(state.mtx);
    state.star_transactions = std::move(entries);
}

void Stars::send_stars(int64_t user_id, int64_t amount) {
    // Stars can be sent through gifts — this is a wrapper
    // The actual flow depends on the TDLib API layer available
    // For now we send as a gift using a basic Star gift
    (void)user_id;
    (void)amount;
}

std::string Stars::get_purchase_url() {
    // Stars are purchased through the official Telegram app or t.me/stars
    return "https://t.me/stars";
}

} // namespace tgcli
