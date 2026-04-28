// ── Telegram CLI — Main Application Implementation ──────────────────────────
#include "App.hpp"
#include "exteraGram.hpp"

#include "../ui/AuthScreen.hpp"
#include "../ui/StatusBar.hpp"
#include "../ui/ChatList.hpp"
#include "../ui/ChatView.hpp"
#include "../ui/InputBar.hpp"
#include "../ui/StarsPanel.hpp"
#include "../ui/GiftsPanel.hpp"
#include "../ui/InfoPanel.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

#include <thread>
#include <iostream>

using namespace ftxui;

namespace tgcli {

App::App()
    : config_(Config::instance()),
      screen_(ScreenInteractive::Fullscreen()) {

    // Load configuration
    config_.load();

    // Initialize TDLib client
    client_ = std::make_unique<TdClient>(state_);
    auth_ = std::make_unique<Auth>(*client_);
    messages_ = std::make_unique<Messages>(*client_);
    stars_ = std::make_unique<Stars>(*client_);
    gifts_ = std::make_unique<Gifts>(*client_);

    // Set update callback to refresh UI
    client_->set_update_callback([this]() {
        screen_.Post(Event::Custom);
    });
}

App::~App() {
    if (client_) {
        client_->stop();
    }
}

void App::on_auth_ready() {
    if (auth_ready_initialized_) {
        return;
    }
    auth_ready_initialized_ = true;
    mode_ = UIMode::MAIN;

    // Fetch user info
    auth_->fetch_me();

    // Load chats
    messages_->load_chats(50);

    {
        std::lock_guard<std::mutex> lock(state_.mtx);
        if (!state_.chats.empty()) {
            state_.selected_chat_index = 0;
            state_.selected_chat_id = state_.chats.front().id;
        }
    }

    int64_t chat_id = 0;
    {
        std::lock_guard<std::mutex> lock(state_.mtx);
        chat_id = state_.selected_chat_id;
    }
    if (chat_id != 0) {
        on_chat_selected(chat_id);
    }

    // Fetch profiles
    std::thread([this]() {
        exteraGram::fetch_profiles(state_);
    }).detach();

    // Fetch Stars balance
    std::thread([this]() {
        stars_->fetch_balance();
        stars_->fetch_transactions();
    }).detach();
}

void App::on_chat_selected(int64_t chat_id) {
    // Load history
    std::thread([this, chat_id]() {
        {
            std::lock_guard<std::mutex> lock(state_.mtx);
            state_.messages.clear();
            state_.scroll_offset = 0;
        }

        messages_->load_history(chat_id, 30);
        messages_->fetch_chat_full_info(chat_id);

        // Mark as read
        std::lock_guard<std::mutex> lock(state_.mtx);
        if (!state_.messages.empty()) {
            messages_->mark_read(chat_id, state_.messages.back().id);
        }

        // Refresh UI
        screen_.Post(Event::Custom);

        // Focus input bar so user can type immediately
        if (input_comp_) {
            input_comp_->TakeFocus();
        }
    }).detach();
}

void App::on_message_send(const std::string& text) {
    int64_t chat_id;
    int64_t reply_id;
    int64_t edit_id;

    {
        std::lock_guard<std::mutex> lock(state_.mtx);
        chat_id = state_.selected_chat_id;
        reply_id = state_.reply_to_msg_id;
        edit_id = state_.edit_msg_id;
        state_.reply_to_msg_id = 0;
        state_.edit_msg_id = 0;
    }

    if (chat_id == 0) return;

    if (edit_id != 0) {
        messages_->edit_text(chat_id, edit_id, text);
    } else {
        // Check if it's a file send command
        if (text.length() > 6 && text.substr(0, 6) == "/file ") {
            messages_->send_file(chat_id, text.substr(6));
        } else {
            messages_->send_text(chat_id, text, reply_id);
        }
    }
}

void App::on_command(const std::string& cmd) {
    if (cmd == "stars") {
        std::lock_guard<std::mutex> lock(state_.mtx);
        state_.show_stars_panel = !state_.show_stars_panel;
        state_.show_gifts_panel = false;
    } else if (cmd == "gifts") {
        std::lock_guard<std::mutex> lock(state_.mtx);
        state_.show_gifts_panel = !state_.show_gifts_panel;
        state_.show_stars_panel = false;
    } else if (cmd == "info" || cmd == "i") {
        std::lock_guard<std::mutex> lock(state_.mtx);
        state_.show_info_panel = !state_.show_info_panel;
    } else if (cmd == "stars close") {
        std::lock_guard<std::mutex> lock(state_.mtx);
        state_.show_stars_panel = false;
    } else if (cmd == "gifts close") {
        std::lock_guard<std::mutex> lock(state_.mtx);
        state_.show_gifts_panel = false;
    } else if (cmd == "theme dark") {
        config_.apply_theme("dark");
        config_.theme_name = "dark";
        config_.save();
    } else if (cmd == "theme nord") {
        config_.apply_theme("nord");
        config_.theme_name = "nord";
        config_.save();
    } else if (cmd == "theme gruvbox") {
        config_.apply_theme("gruvbox");
        config_.theme_name = "gruvbox";
        config_.save();
    } else if (cmd == "settings" || cmd == "config") {
        // Just show info how to open config
        // In a real app this would open a settings panel
    } else if (cmd == "quit" || cmd == "q") {
        screen_.Exit();
    } else if (cmd == "logout") {
        auth_->logout();
        screen_.Exit();
    } else if (cmd == "buy") {
        // Show purchase URL (can't open browser from terminal, just display)
        // This is informational
    } else if (cmd.substr(0, 6) == "react ") {
        // :react <emoji>
        std::string emoji = cmd.substr(6);
        int64_t chat_id, msg_id;
        {
            std::lock_guard<std::mutex> lock(state_.mtx);
            chat_id = state_.selected_chat_id;
            msg_id = state_.messages.empty() ? 0 : state_.messages.back().id;
        }
        if (chat_id != 0 && msg_id != 0) {
            messages_->add_reaction(chat_id, msg_id, emoji);
        }
    }
}

void App::setup_ui() {
    // No setup needed — everything is done in run()
}

void App::run() {
    // Start TDLib event loop
    client_->start();

    // ── Auth Screen ─────────────────────────────────────────────────────
    ui::AuthScreen auth_screen(state_, *auth_);
    auth_screen.set_on_ready([this]() { on_auth_ready(); });
    auto auth_component = auth_screen.component();

    // ── Main UI Components ──────────────────────────────────────────────
    ui::StatusBar status_bar(state_);

    ui::ChatList chat_list(state_);
    chat_list.set_on_select([this](int64_t id) { on_chat_selected(id); });
    auto chatlist_comp = chat_list.component();

    ui::ChatView chat_view(state_);
    auto chatview_comp = chat_view.component();

    ui::InputBar input_bar(state_);
    input_bar.set_on_send([this](const std::string& t) { on_message_send(t); });
    input_bar.set_on_command([this](const std::string& c) { on_command(c); });
    auto input_comp = input_bar.component();
    input_comp_ = input_comp;

    ui::StarsPanel stars_panel(state_);
    auto stars_comp = stars_panel.component();

    ui::GiftsPanel gifts_panel(state_);
    auto gifts_comp = gifts_panel.component();

    ui::InfoPanel info_panel(state_);
    auto info_comp = info_panel.component();

    // ── Focus container for main view ───────────────────────────────────
    main_container_ = Container::Horizontal({
        chatlist_comp,
        Container::Vertical({
            chatview_comp,
            input_comp,
        }),
    });

    // ── Root component ──────────────────────────────────────────────────
    auto tab_index = std::make_shared<int>(0);
    bool auth_ready_handled = false;

    auto root = Renderer(Container::Tab({auth_component, main_container_}, tab_index.get()),
        [&, tab_index]() {
            // Check auth state
            AuthState auth_st;
            bool show_stars, show_gifts, show_info;
            {
                std::lock_guard<std::mutex> lock(state_.mtx);
                auth_st = state_.auth_state;
                show_stars = state_.show_stars_panel;
                show_gifts = state_.show_gifts_panel;
                show_info = state_.show_info_panel;
            }

            if (auth_st == AuthState::READY && mode_ == UIMode::AUTH && !auth_ready_handled) {
                auth_ready_handled = true;
                mode_ = UIMode::MAIN;
                *tab_index = 1;
                on_auth_ready();
            }

            if (auth_st != AuthState::READY && mode_ == UIMode::AUTH) {
                *tab_index = 0;
                return auth_component->Render();
            } else {
                mode_ = UIMode::MAIN;
                *tab_index = 1;
            }

            // ── Main layout ─────────────────────────────────────────────
            auto status = status_bar.render();

            // Layout
            Elements main_row;
            main_row.push_back(chatlist_comp->Render() | size(WIDTH, EQUAL, 30));
            main_row.push_back(separator() | color(Color::Palette256(config_.theme.border_color)));

            // Center: chat view + input
            main_row.push_back(
                vbox({
                    chatview_comp->Render() | flex,
                    input_comp->Render(),
                }) | flex
            );

            // Right: optional panels
            if (show_stars) {
                main_row.push_back(separator() | color(Color::Palette256(config_.theme.border_color)));
                main_row.push_back(stars_comp->Render());
            } else if (show_gifts) {
                main_row.push_back(separator() | color(Color::Palette256(config_.theme.border_color)));
                main_row.push_back(gifts_comp->Render());
            } else if (show_info) {
                main_row.push_back(separator() | color(Color::Palette256(config_.theme.border_color)));
                main_row.push_back(info_comp->Render());
            }

            return vbox({
                status,
                hbox(std::move(main_row)) | flex,
            });
        }
    ) | CatchEvent([this, &input_bar, input_comp](Event event) {
        // Check if any interactive component has focus
        // In simple terms: if the input bar is focused, we don't handle single-letter hotkeys
        bool input_focused = input_comp->Focused();

        // Global hotkeys
        if ((event == Event::F2 ) && mode_ == UIMode::MAIN) {
            on_command("info");
            return true;
        }

        // Colon to enter command mode
        if (event == Event::Character(':') && mode_ == UIMode::MAIN && !input_focused) {
            input_bar.set_text(":");
            input_comp->TakeFocus();
            return true;
        }

        // Ctrl+G as alternative for Ctrl+;
        if (event == Event::Special("\x07") && mode_ == UIMode::MAIN) { // Ctrl+G
            input_bar.set_text(":");
            input_comp->TakeFocus();
            return true;
        }

        if (event == Event::Escape && mode_ == UIMode::MAIN) {
            bool closed_any = false;
            {
                std::lock_guard<std::mutex> lock(state_.mtx);
                if (state_.show_info_panel) {
                    state_.show_info_panel = false;
                    closed_any = true;
                }
                if (state_.show_stars_panel) {
                    state_.show_stars_panel = false;
                    closed_any = true;
                }
                if (state_.show_gifts_panel) {
                    state_.show_gifts_panel = false;
                    closed_any = true;
                }
            }
            if (closed_any) {
                return true;
            }
        }
        
        if (event == Event::Character('\x03')) {
            screen_.Exit();
            return true;
        }
        return false;
    });

    screen_.Loop(root);
}

} // namespace tgcli
