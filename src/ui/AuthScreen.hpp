#pragma once
// ── Telegram CLI — Auth Screen UI ───────────────────────────────────────────
#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include "../app/State.hpp"
#include "../telegram/Auth.hpp"
#include <functional>
#include <vector>
#include <string>

namespace tgcli::ui {

class AuthScreen {
public:
    using OnAuthReady = std::function<void()>;

    AuthScreen(AppState& state, Auth& auth);

    ftxui::Component component();
    void set_on_ready(OnAuthReady cb) { on_ready_ = cb; }

private:
    AppState& state_;
    Auth& auth_;
    OnAuthReady on_ready_;

    std::string input_text_;
    std::string password_text_;
    std::string first_name_;
    std::string last_name_;

    // QR code rendering
    std::vector<std::string> generate_qr_lines(const std::string& data);
    std::string last_qr_link_;
    std::vector<std::string> cached_qr_lines_;
};

} // namespace tgcli::ui
