// ── Telegram CLI — Authentication Implementation ────────────────────────────
#include "Auth.hpp"

namespace tgcli {

Auth::Auth(TdClient& client) : client_(client) {}

void Auth::send_phone(const std::string& phone) {
    auto settings = td_api::make_object<td_api::phoneNumberAuthenticationSettings>();
    auto request = td_api::make_object<td_api::setAuthenticationPhoneNumber>(phone, std::move(settings));
    client_.send(std::move(request));
}

void Auth::send_code(const std::string& code) {
    client_.send(td_api::make_object<td_api::checkAuthenticationCode>(code));
}

void Auth::send_password(const std::string& password) {
    client_.send(td_api::make_object<td_api::checkAuthenticationPassword>(password));
}

void Auth::send_registration(const std::string& first_name, const std::string& last_name) {
    client_.send(td_api::make_object<td_api::registerUser>(first_name, last_name, false));
}

void Auth::switch_to_phone_login() {
    auto& state = client_.state();
    {
        std::lock_guard<std::mutex> lock(state.mtx);
        state.use_qr_login = false;
        state.qr_code_link.clear();
        state.auth_state = AuthState::WAIT_PHONE;
    }
}

void Auth::request_qr_code() {
    client_.send(td_api::make_object<td_api::requestQrCodeAuthentication>());
}

void Auth::fetch_me() {
    auto result = client_.send_sync(td_api::make_object<td_api::getMe>());
    if (!result) return;

    if (result->get_id() == td_api::user::ID) {
        auto& user = static_cast<td_api::user&>(*result);
        auto& state = client_.state();
        std::lock_guard<std::mutex> lock(state.mtx);

        state.current_user.id = user.id_;
        state.current_user.first_name = user.first_name_;
        state.current_user.last_name = user.last_name_;
        if (user.usernames_) {
            if (!user.usernames_->editable_username_.empty()) {
                state.current_user.username = user.usernames_->editable_username_;
            }
        }
        state.current_user.phone = user.phone_number_;
        state.current_user.is_premium = user.is_premium_;
    }
}

void Auth::logout() {
    client_.send(td_api::make_object<td_api::logOut>());
}

} // namespace tgcli
