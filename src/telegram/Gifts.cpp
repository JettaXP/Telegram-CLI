// ── Telegram CLI — Gifts & NFT Implementation ──────────────────────────────
#include "Gifts.hpp"

namespace tgcli {

Gifts::Gifts(TdClient& client) : client_(client) {}

void Gifts::fetch_available_gifts() {
    auto result = client_.send_sync(
        td_api::make_object<td_api::getAvailableGifts>()
    );

    if (!result) return;

    // Parse available gifts based on TDLib response type
    // The exact API depends on the TDLib version
    auto& state = client_.state();
    std::lock_guard<std::mutex> lock(state.mtx);

    if (result->get_id() == td_api::availableGifts::ID) {
        auto& gifts = static_cast<td_api::availableGifts&>(*result);
        state.available_gifts.clear();
        for (auto& gift : gifts.gifts_) {
            if (!gift || !gift->gift_) continue;
            GiftEntry entry;
            entry.id = gift->gift_->id_;
            entry.star_count = gift->gift_->star_count_;
            entry.is_nft = false;
            state.available_gifts.push_back(std::move(entry));
        }
    }
}

void Gifts::fetch_saved_gifts() {
    auto result = client_.send_sync(
        td_api::make_object<td_api::getReceivedGifts>(
            "", td_api::make_object<td_api::messageSenderUser>(client_.state().current_user.id),
            0, false, false, false, false, false, false, false, false, false, "", 50
        )
    );

    if (!result) return;

    auto& state = client_.state();
    std::lock_guard<std::mutex> lock(state.mtx);

    if (result->get_id() == td_api::receivedGifts::ID) {
        auto& user_gifts = static_cast<td_api::receivedGifts&>(*result);
        state.saved_gifts.clear();
        for (auto& ug : user_gifts.gifts_) {
            if (!ug || !ug->gift_) continue;
            GiftEntry entry;
            if (ug->gift_->get_id() == td_api::sentGiftRegular::ID) {
                auto& regular = static_cast<td_api::sentGiftRegular&>(*ug->gift_);
                if (!regular.gift_) continue;
                entry.id = regular.gift_->id_;
                entry.star_count = regular.gift_->star_count_;
            } else if (ug->gift_->get_id() == td_api::sentGiftUpgraded::ID) {
                auto& upgraded = static_cast<td_api::sentGiftUpgraded&>(*ug->gift_);
                if (!upgraded.gift_) continue;
                entry.id = upgraded.gift_->id_;
                entry.star_count = 0;
            } else {
                continue;
            }
            entry.date = ug->date_;
            state.saved_gifts.push_back(std::move(entry));
        }
    }
}

void Gifts::send_gift(int64_t user_id, int64_t gift_id) {
    auto text = td_api::make_object<td_api::formattedText>(
        "", std::vector<td_api::object_ptr<td_api::textEntity>>()
    );
    client_.send(td_api::make_object<td_api::sendGift>(
        gift_id,
        td_api::make_object<td_api::messageSenderUser>(user_id),
        std::move(text), false, false
    ));
}

void Gifts::convert_to_nft(int64_t gift_id) {
    // Upgrade a regular gift to a unique NFT collectible
    client_.send(td_api::make_object<td_api::upgradeGift>(
        "", std::to_string(gift_id), false, 0
    ));
}

void Gifts::transfer_nft(int64_t gift_id, int64_t to_user_id) {
    client_.send(td_api::make_object<td_api::transferGift>(
        "", std::to_string(gift_id), td_api::make_object<td_api::messageSenderUser>(to_user_id), 0
    ));
}

} // namespace tgcli
