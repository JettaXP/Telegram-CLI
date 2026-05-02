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

#include <algorithm>
#include <thread>

using namespace ftxui;

namespace tgcli {

App::App()
    : config_(Config::instance()),
      screen_(ScreenInteractive::Fullscreen()) {
    config_.load();

    client_ = std::make_unique<TdClient>(state_);
    auth_ = std::make_unique<Auth>(*client_);
    messages_ = std::make_unique<Messages>(*client_);
    stars_ = std::make_unique<Stars>(*client_);
    gifts_ = std::make_unique<Gifts>(*client_);

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

    auth_->fetch_me();
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

    std::thread([this]() {
        exteraGram::fetch_profiles(state_);
    }).detach();

    std::thread([this]() {
        stars_->fetch_balance();
        stars_->fetch_transactions();
    }).detach();
}

void App::on_chat_selected(int64_t chat_id) {
    focus_input_pending_ = false;
    screen_.Post(Event::Custom);

    std::thread([this, chat_id]() {
        {
            std::lock_guard<std::mutex> lock(state_.mtx);
            state_.messages.clear();
            state_.scroll_offset = 0;
            state_.chatview_view_size = 1;
            state_.oldest_loaded_message_id = 0;
            state_.newest_loaded_message_id = 0;
            state_.history_loading = true;
            state_.history_exhausted = false;
            state_.selected_chat_details = ChatFullInfo{};
            state_.selected_chat_details.id = chat_id;
        }

        int loaded_count = messages_->load_history(chat_id, 1000);
        messages_->fetch_chat_full_info(chat_id);

        bool current_chat = false;
        bool can_send = true;
        {
            std::lock_guard<std::mutex> lock(state_.mtx);
            current_chat = (state_.selected_chat_id == chat_id);
            if (!state_.messages.empty()) {
                state_.oldest_loaded_message_id = state_.messages.front().id;
                state_.newest_loaded_message_id = state_.messages.back().id;
            }
            if (loaded_count < 1000) {
                state_.history_exhausted = true;
            }
            state_.history_loading = false;
            state_.scroll_offset = 0;
            can_send = state_.selected_chat_details.can_send_messages;
        }

        if (!current_chat) {
            return;
        }

        if (!state_.messages.empty()) {
            messages_->mark_read(chat_id, state_.messages.back().id);
        }

        focus_input_pending_ = can_send;
        screen_.Post(Event::Custom);
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

    if (chat_id == 0) {
        return;
    }

    if (edit_id != 0) {
        messages_->edit_text(chat_id, edit_id, text);
        return;
    }

    if (text.length() > 6 && text.substr(0, 6) == "/file ") {
        messages_->send_file(chat_id, text.substr(6));
    } else {
        messages_->send_text(chat_id, text, reply_id);
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
    } else if (cmd == "quit" || cmd == "q") {
        screen_.Exit();
    } else if (cmd == "logout") {
        auth_->logout();
        screen_.Exit();
    } else if (cmd.substr(0, 6) == "react ") {
        std::string emoji = cmd.substr(6);
        int64_t chat_id = 0;
        int64_t msg_id = 0;
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
}

void App::run() {
    client_->start();

    ui::AuthScreen auth_screen(state_, *auth_);
    auth_screen.set_on_ready([this]() { on_auth_ready(); });
    auto auth_component = auth_screen.component();

    ui::StatusBar status_bar(state_);

    ui::ChatList chat_list(state_);
    chat_list.set_on_select([this](int64_t id) { on_chat_selected(id); });
    auto chatlist_comp = chat_list.component();

    ui::ChatView chat_view(state_);
    auto chatview_comp = chat_view.component();

    ui::InputBar input_bar(state_);
    input_bar.set_on_send([this](const std::string& t) { on_message_send(t); });
    input_bar.set_on_command([this](const std::string& c) { on_command(c); });

    auto reload_selected_chat = [this]() {
        int64_t chat_id = 0;
        {
            std::lock_guard<std::mutex> lock(state_.mtx);
            chat_id = state_.selected_chat_id;
        }
        if (chat_id == 0) {
            return false;
        }
        {
            std::lock_guard<std::mutex> lock(state_.mtx);
            state_.messages.clear();
            state_.scroll_offset = 0;
            state_.chatview_view_size = 1;
            state_.oldest_loaded_message_id = 0;
            state_.newest_loaded_message_id = 0;
            state_.history_loading = false;
            state_.history_exhausted = false;
        }
        on_chat_selected(chat_id);
        return true;
    };

    auto page_step = [this]() {
        std::lock_guard<std::mutex> lock(state_.mtx);
        return std::max(3, state_.chatview_view_size / 4);
    };

    auto scroll_messages = [this](int delta, bool home, bool end) {
        std::lock_guard<std::mutex> lock(state_.mtx);
        int total = static_cast<int>(state_.messages.size());
        int view_size = std::max(1, state_.chatview_view_size);
        int max_offset = std::max(0, total - view_size);
        if (home) {
            state_.scroll_offset = max_offset;
        } else if (end) {
            state_.scroll_offset = 0;
        } else {
            state_.scroll_offset = std::clamp(state_.scroll_offset + delta, 0, max_offset);
        }
        screen_.Post(Event::Custom);
        return true;
    };

    auto request_older_history = [this]() {
        int64_t chat_id = 0;
        int64_t anchor_message_id = 0;
        {
            std::lock_guard<std::mutex> lock(state_.mtx);
            if (state_.history_loading || state_.history_exhausted || state_.selected_chat_id == 0 || state_.oldest_loaded_message_id == 0) {
                return true;
            }
            chat_id = state_.selected_chat_id;
            anchor_message_id = state_.oldest_loaded_message_id;
            state_.history_loading = true;
        }

        std::thread([this, chat_id, anchor_message_id]() {
            int loaded = messages_->load_history(chat_id, 200, anchor_message_id);

            {
                std::lock_guard<std::mutex> lock(state_.mtx);
                if (state_.selected_chat_id != chat_id) {
                    state_.history_loading = false;
                    return;
                }
                if (!state_.messages.empty()) {
                    state_.oldest_loaded_message_id = state_.messages.front().id;
                    state_.newest_loaded_message_id = state_.messages.back().id;
                }
                if (loaded < 200) {
                    state_.history_exhausted = true;
                }
                if (loaded > 0 && state_.scroll_offset > 0) {
                    state_.scroll_offset += loaded;
                }
                state_.history_loading = false;
            }

            screen_.Post(Event::Custom);
        }).detach();

        return true;
    };

    auto select_chat_at = [this](int new_index) {
        int chat_id = 0;
        {
            std::lock_guard<std::mutex> lock(state_.mtx);
            if (state_.chats.empty()) {
                return true;
            }
            new_index = std::clamp(new_index, 0, static_cast<int>(state_.chats.size()) - 1);
            state_.selected_chat_index = new_index;
            state_.selected_chat_id = state_.chats[new_index].id;
            chat_id = state_.selected_chat_id;
        }
        on_chat_selected(chat_id);
        return true;
    };

    input_bar.set_on_nav([&](ui::InputBar::NavAction action) {
        auto needs_history = [&]() {
            std::lock_guard<std::mutex> lock(state_.mtx);
            int total = static_cast<int>(state_.messages.size());
            int view_size = std::max(1, state_.chatview_view_size);
            int max_offset = std::max(0, total - view_size);
            return state_.scroll_offset >= max_offset &&
                   !state_.history_loading &&
                   !state_.history_exhausted &&
                   state_.selected_chat_id != 0;
        };

        switch (action) {
            case ui::InputBar::NavAction::ArrowUp:
                {
                    int idx = 0;
                    {
                        std::lock_guard<std::mutex> lock(state_.mtx);
                        idx = state_.selected_chat_index - 1;
                    }
                    return select_chat_at(idx);
                }
            case ui::InputBar::NavAction::ArrowDown:
                {
                    int idx = 0;
                    {
                        std::lock_guard<std::mutex> lock(state_.mtx);
                        idx = state_.selected_chat_index + 1;
                    }
                    return select_chat_at(idx);
                }
            case ui::InputBar::NavAction::PageUp:
                if (needs_history()) {
                    return request_older_history();
                }
                return scroll_messages(-page_step(), false, false);
            case ui::InputBar::NavAction::PageDown:
                return scroll_messages(page_step(), false, false);
            case ui::InputBar::NavAction::Home:
                return scroll_messages(0, true, false);
            case ui::InputBar::NavAction::End:
                return scroll_messages(0, false, true);
            case ui::InputBar::NavAction::Reload:
                return reload_selected_chat();
        }
        return false;
    });

    auto input_comp = input_bar.component();
    input_comp_ = input_comp;

    ui::StarsPanel stars_panel(state_);
    auto stars_comp = stars_panel.component();

    ui::GiftsPanel gifts_panel(state_);
    auto gifts_comp = gifts_panel.component();

    ui::InfoPanel info_panel(state_);
    auto info_comp = info_panel.component();

    main_container_ = Container::Horizontal({
        chatlist_comp,
        Container::Vertical({
            chatview_comp,
            input_comp,
        }),
    });

    auto tab_index = std::make_shared<int>(0);
    bool auth_ready_handled = false;

    auto root = Renderer(Container::Tab({auth_component, main_container_}, tab_index.get()),
        [&, tab_index]() {
            AuthState auth_st;
            bool show_stars = false;
            bool show_gifts = false;
            bool show_info = false;
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
            }

            mode_ = UIMode::MAIN;
            *tab_index = 1;

            auto status = status_bar.render();
            Elements main_row;
            main_row.push_back(chatlist_comp->Render() | size(WIDTH, EQUAL, 30));
            main_row.push_back(separator() | color(Color::Palette256(config_.theme.border_color)));
            main_row.push_back(
                vbox({
                    chatview_comp->Render() | flex,
                    input_comp->Render(),
                }) | flex
            );

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

            auto layout = vbox({
                status,
                hbox(std::move(main_row)) | flex,
            });

            if (focus_input_pending_ && input_comp) {
                focus_input_pending_ = false;
                input_comp->TakeFocus();
            }

            return layout;
        }
    ) | CatchEvent([this, &input_bar, input_comp, scroll_messages, request_older_history, reload_selected_chat, page_step](Event event) {
        bool input_focused = input_comp->Focused();

        auto select_chat_by_delta = [&](int delta) {
            int64_t chat_id = 0;
            {
                std::lock_guard<std::mutex> lock(state_.mtx);
                if (state_.chats.empty()) {
                    return true;
                }
                int new_idx = std::clamp(state_.selected_chat_index + delta, 0, static_cast<int>(state_.chats.size()) - 1);
                state_.selected_chat_index = new_idx;
                state_.selected_chat_id = state_.chats[new_idx].id;
                chat_id = state_.selected_chat_id;
            }
            on_chat_selected(chat_id);
            return true;
        };

        if (mode_ == UIMode::MAIN) {
            if (event == Event::ArrowUp) {
                return select_chat_by_delta(-1);
            }
            if (event == Event::ArrowDown) {
                return select_chat_by_delta(1);
            }
            if (event == Event::PageUp) {
                bool load_history = false;
                {
                    std::lock_guard<std::mutex> lock(state_.mtx);
                    int total = static_cast<int>(state_.messages.size());
                    int view_size = std::max(1, state_.chatview_view_size);
                    int max_offset = std::max(0, total - view_size);
                    load_history = (state_.scroll_offset >= max_offset &&
                                    !state_.history_loading &&
                                    !state_.history_exhausted &&
                                    state_.selected_chat_id != 0);
                }
                if (load_history) {
                    return request_older_history();
                }
                return scroll_messages(-page_step(), false, false);
            }
            if (event == Event::PageDown) {
                return scroll_messages(page_step(), false, false);
            }
            if (event == Event::Home) {
                return scroll_messages(0, true, false);
            }
            if (event == Event::End) {
                return scroll_messages(0, false, true);
            }
        }

        {
            bool show_info = false;
            {
                std::lock_guard<std::mutex> lock(state_.mtx);
                show_info = state_.show_info_panel;
            }
            if (show_info && (event == Event::Character('p') || event == Event::Character('P'))) {
                std::lock_guard<std::mutex> lock(state_.mtx);
                for (auto& c : state_.chats) {
                    if (c.id == state_.selected_chat_id) {
                        c.is_pinned = !c.is_pinned;
                        break;
                    }
                }
                std::sort(state_.chats.begin(), state_.chats.end(), [](const ChatEntry& a, const ChatEntry& b) {
                    if (a.is_pinned != b.is_pinned) {
                        return a.is_pinned > b.is_pinned;
                    }
                    return a.order > b.order;
                });
                focus_input_pending_ = false;
                return true;
            }
        }

        if ((event == Event::F2) && mode_ == UIMode::MAIN) {
            on_command("info");
            return true;
        }

        if (event == Event::Character(':') && mode_ == UIMode::MAIN && !input_focused) {
            input_bar.set_text(":");
            input_comp->TakeFocus();
            return true;
        }

        if (event == Event::Special("\x07") && mode_ == UIMode::MAIN) {
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
                focus_input_pending_ = true;
                return true;
            }
        }

        if (event == Event::F3 && mode_ == UIMode::MAIN) {
            return reload_selected_chat();
        }

        if (event == Event::Return && mode_ == UIMode::MAIN && !input_focused) {
            return reload_selected_chat();
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
