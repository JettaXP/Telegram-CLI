#pragma once
// ── Telegram CLI — Main Application ─────────────────────────────────────────
#include "State.hpp"
#include "Config.hpp"
#include "../telegram/Client.hpp"
#include "../telegram/Auth.hpp"
#include "../telegram/Messages.hpp"
#include "../telegram/Stars.hpp"
#include "../telegram/Gifts.hpp"

#include <ftxui/component/screen_interactive.hpp>
#include <memory>

namespace tgcli {

class App {
public:
    App();
    ~App();

    // Run the main application loop
    void run();

private:
    void setup_ui();
    void on_auth_ready();
    void on_chat_selected(int64_t chat_id);
    void on_message_send(const std::string& text);
    void on_command(const std::string& cmd);

    // State & Config
    AppState state_;
    Config& config_;

    // Telegram modules
    std::unique_ptr<TdClient> client_;
    std::unique_ptr<Auth> auth_;
    std::unique_ptr<Messages> messages_;
    std::unique_ptr<Stars> stars_;
    std::unique_ptr<Gifts> gifts_;

    // Screen
    ftxui::ScreenInteractive screen_;
    ftxui::Component main_container_;
    ftxui::Component input_comp_;

    // UI focus helpers
    std::atomic<bool> focus_input_pending_ = false;

    // Current UI mode
    enum class UIMode {
        AUTH,
        MAIN
    };
    UIMode mode_ = UIMode::AUTH;
    bool auth_ready_initialized_ = false;
};

} // namespace tgcli
