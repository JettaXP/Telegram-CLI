#pragma once
// ── Telegram CLI — Authentication ───────────────────────────────────────────
#include "Client.hpp"

namespace tgcli {

class Auth {
public:
    Auth(TdClient& client);

    // Send phone number for login (switches from QR to phone auth)
    void send_phone(const std::string& phone);

    // Send verification code
    void send_code(const std::string& code);

    // Send 2FA password
    void send_password(const std::string& password);

    // Send registration info (first name, last name)
    void send_registration(const std::string& first_name, const std::string& last_name);

    // Switch to phone login mode (cancels QR)
    void switch_to_phone_login();

    // Request QR code login
    void request_qr_code();

    // Apply TDLib parameters from config
    void send_tdlib_parameters();

    // Fetch current user info after auth
    void fetch_me();

    // Log out
    void logout();

private:
    TdClient& client_;
};

} // namespace tgcli
