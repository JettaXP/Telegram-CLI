// ── Telegram CLI — TDLib Client Implementation ──────────────────────────────
#include "Client.hpp"
#include "../app/Config.hpp"

#include <iostream>
#include <chrono>
#include <algorithm>

namespace tgcli {

TdClient::TdClient(AppState& state) : state_(state) {
    td::ClientManager::execute(td_api::make_object<td_api::setLogVerbosityLevel>(1));
    client_manager_ = std::make_unique<td::ClientManager>();
    client_id_ = client_manager_->create_client_id();
    // Send initial request to start auth flow
    send(td_api::make_object<td_api::getOption>("version"));
}

TdClient::~TdClient() {
    stop();
}

void TdClient::start() {
    running_ = true;
    event_thread_ = std::thread(&TdClient::event_loop, this);
}

void TdClient::stop() {
    running_ = false;
    if (event_thread_.joinable()) {
        event_thread_.join();
    }
}

void TdClient::send(td_api::object_ptr<td_api::Function> request) {
    auto id = next_request_id();
    client_manager_->send(client_id_, id, std::move(request));
}

TdClient::Object TdClient::send_sync(td_api::object_ptr<td_api::Function> request, int timeout_sec) {
    auto id = next_request_id();
    client_manager_->send(client_id_, id, std::move(request));

    // Wait for response with this ID
    std::unique_lock<std::mutex> lock(response_mtx_);
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(timeout_sec);

    while (responses_.find(id) == responses_.end()) {
        if (response_cv_.wait_until(lock, deadline) == std::cv_status::timeout) {
            return nullptr;
        }
    }

    auto result = std::move(responses_[id]);
    responses_.erase(id);
    return result;
}

void TdClient::event_loop() {
    while (running_) {
        auto response = client_manager_->receive(0.5);
        if (response.object) {
            if (response.request_id == 0) {
                // This is an update
                process_update(std::move(response.object));
            } else {
                // Global error intercept for auth hints
                if (response.object->get_id() == td_api::error::ID) {
                    auto& err = static_cast<td_api::error&>(*response.object);
                    {
                        std::lock_guard<std::mutex> st_lock(state_.mtx);
                        if (err.message_ == "PASSWORD_HASH_INVALID") {
                            state_.auth_hint = "Incorrect password! Please try again.";
                        } else {
                            state_.auth_hint = err.message_;
                        }
                    }
                    if (update_callback_) {
                        update_callback_();
                    }
                }

                // Store response if anyone is waiting via send_sync
                std::lock_guard<std::mutex> lock(response_mtx_);
                responses_[response.request_id] = std::move(response.object);
                response_cv_.notify_all();
            }
        }
    }
}

void TdClient::process_update(td_api::object_ptr<td_api::Object> update) {
    if (!update) return;

    td_api::downcast_call(*update, [this](auto& obj) {
        using T = std::decay_t<decltype(obj)>;

        if constexpr (std::is_same_v<T, td_api::updateAuthorizationState>) {
            process_auth_state(std::move(obj.authorization_state_));
        } else if constexpr (std::is_same_v<T, td_api::updateNewMessage>) {
            on_update_new_message(obj);
        } else if constexpr (std::is_same_v<T, td_api::updateNewChat>) {
            on_update_new_chat(obj);
        } else if constexpr (std::is_same_v<T, td_api::updateChatLastMessage>) {
            on_update_chat_last_message(obj);
        } else if constexpr (std::is_same_v<T, td_api::updateChatPosition>) {
            on_update_chat_position(obj);
        } else if constexpr (std::is_same_v<T, td_api::updateChatReadInbox>) {
            on_update_chat_read_inbox(obj);
        } else if constexpr (std::is_same_v<T, td_api::updateUser>) {
            on_update_user(obj);
        } else if constexpr (std::is_same_v<T, td_api::updateConnectionState>) {
            on_update_connection_state(obj);
        }
        // Other updates are silently ignored for now
    });

    if (update_callback_) {
        update_callback_();
    }
}

void TdClient::process_auth_state(td_api::object_ptr<td_api::AuthorizationState> auth_state) {
    if (!auth_state) return;

    std::lock_guard<std::mutex> lock(state_.mtx);

    td_api::downcast_call(*auth_state, [this](auto& state_obj) {
        using T = std::decay_t<decltype(state_obj)>;

        if constexpr (std::is_same_v<T, td_api::authorizationStateWaitTdlibParameters>) {
            auto& cfg = Config::instance();
            auto params = td_api::make_object<td_api::setTdlibParameters>();
            params->use_test_dc_ = false;
            params->database_directory_ = cfg.tdlib_data_dir;
            params->files_directory_ = "";
            params->database_encryption_key_ = "";
            params->use_file_database_ = true;
            params->use_chat_info_database_ = true;
            params->use_message_database_ = true;
            params->use_secret_chats_ = true;
            params->api_id_ = cfg.api_id;
            params->api_hash_ = cfg.api_hash;
            params->system_language_code_ = "en";
            params->device_model_ = "TelegramCLI";
            params->system_version_ = "Linux";
            params->application_version_ = "1.0.0";
            send(std::move(params));
        } else if constexpr (std::is_same_v<T, td_api::authorizationStateWaitPhoneNumber>) {
            if (state_.use_qr_login) {
                // Request QR code authentication instead of phone number
                state_.auth_state = AuthState::WAIT_QR_CODE;
                send(td_api::make_object<td_api::requestQrCodeAuthentication>());
            } else {
                state_.auth_state = AuthState::WAIT_PHONE;
            }
        } else if constexpr (std::is_same_v<T, td_api::authorizationStateWaitOtherDeviceConfirmation>) {
            // QR code login — store the tg:// link for display
            state_.auth_state = AuthState::WAIT_QR_CODE;
            state_.qr_code_link = state_obj.link_;
        } else if constexpr (std::is_same_v<T, td_api::authorizationStateWaitCode>) {
            state_.auth_state = AuthState::WAIT_CODE;
            if (state_obj.code_info_) {
                // Store hint about where code was sent
                state_.auth_hint = "Check your Telegram app for the code";
            }
        } else if constexpr (std::is_same_v<T, td_api::authorizationStateWaitPassword>) {
            state_.auth_state = AuthState::WAIT_PASSWORD;
            state_.auth_hint = state_obj.password_hint_;
        } else if constexpr (std::is_same_v<T, td_api::authorizationStateWaitRegistration>) {
            state_.auth_state = AuthState::WAIT_REGISTRATION;
        } else if constexpr (std::is_same_v<T, td_api::authorizationStateReady>) {
            state_.auth_state = AuthState::READY;
            // Fetch main chat list
            send(td_api::make_object<td_api::loadChats>(td_api::make_object<td_api::chatListMain>(), 100));
        } else if constexpr (std::is_same_v<T, td_api::authorizationStateLoggingOut>) {
            state_.auth_state = AuthState::LOGGING_OUT;
        } else if constexpr (std::is_same_v<T, td_api::authorizationStateClosed>) {
            state_.auth_state = AuthState::CLOSED;
        }
    });
}

// ── Update handlers ─────────────────────────────────────────────────────────

void TdClient::on_update_new_message(td_api::updateNewMessage& update) {
    if (!update.message_) return;
    auto& msg = *update.message_;

    std::lock_guard<std::mutex> lock(state_.mtx);

    // Only add to current chat's message list
    if (msg.chat_id_ == state_.selected_chat_id) {
        MessageEntry entry;
        entry.id = msg.id_;
        entry.chat_id = msg.chat_id_;
        entry.date = msg.date_;
        entry.is_outgoing = msg.is_outgoing_;

        // Extract sender
        if (msg.sender_id_) {
            td_api::downcast_call(*msg.sender_id_, [&entry](auto& sender) {
                using T = std::decay_t<decltype(sender)>;
                if constexpr (std::is_same_v<T, td_api::messageSenderUser>) {
                    entry.sender_id = sender.user_id_;
                } else if constexpr (std::is_same_v<T, td_api::messageSenderChat>) {
                    entry.sender_id = sender.chat_id_;
                }
            });
        }

        // Extract text content
        if (msg.content_) {
            td_api::downcast_call(*msg.content_, [&entry](auto& content) {
                using T = std::decay_t<decltype(content)>;
                if constexpr (std::is_same_v<T, td_api::messageText>) {
                    if (content.text_) entry.text = content.text_->text_;
                } else if constexpr (std::is_same_v<T, td_api::messagePhoto>) {
                    entry.media_type = "Photo";
                    if (content.caption_) entry.media_caption = content.caption_->text_;
                } else if constexpr (std::is_same_v<T, td_api::messageVideo>) {
                    entry.media_type = "Video";
                    if (content.caption_) entry.media_caption = content.caption_->text_;
                } else if constexpr (std::is_same_v<T, td_api::messageDocument>) {
                    entry.media_type = "File";
                    if (content.document_)
                        entry.file_name = content.document_->file_name_;
                    if (content.caption_) entry.media_caption = content.caption_->text_;
                } else if constexpr (std::is_same_v<T, td_api::messageSticker>) {
                    entry.media_type = "Sticker";
                    if (content.sticker_)
                        entry.text = content.sticker_->emoji_;
                } else if constexpr (std::is_same_v<T, td_api::messageVoiceNote>) {
                    entry.media_type = "Voice";
                    if (content.caption_) entry.media_caption = content.caption_->text_;
                } else if constexpr (std::is_same_v<T, td_api::messageAnimation>) {
                    entry.media_type = "GIF";
                    if (content.caption_) entry.media_caption = content.caption_->text_;
                }
                // Additional content types handled as generic
            });
        }

        // Reply info
        if (msg.reply_to_) {
            td_api::downcast_call(*msg.reply_to_, [&entry](auto& reply) {
                using T = std::decay_t<decltype(reply)>;
                if constexpr (std::is_same_v<T, td_api::messageReplyToMessage>) {
                    entry.reply_to_message_id = reply.message_id_;
                }
            });
        }

        // ExteraGram badge check
        entry.sender_has_extera_badge = state_.is_extera_supporter(entry.sender_id);
        entry.sender_extera_status = state_.extera_status(entry.sender_id);

        state_.messages.push_back(std::move(entry));
    }
}

void TdClient::on_update_new_chat(td_api::updateNewChat& update) {
    if (!update.chat_) return;
    std::lock_guard<std::mutex> lock(state_.mtx);
    ChatEntry entry;
    entry.id = update.chat_->id_;
    entry.title = update.chat_->title_;
    entry.unread_count = update.chat_->unread_count_;
    for (const auto& pos : update.chat_->positions_) {
        if (pos && pos->list_ && pos->list_->get_id() == td_api::chatListMain::ID) {
            entry.order = pos->order_;
            entry.is_pinned = pos->is_pinned_;
        }
    }
    
    // Check type
    if (update.chat_->type_) {
        td_api::downcast_call(*update.chat_->type_, [&entry](auto& type) {
            using T = std::decay_t<decltype(type)>;
            if constexpr (std::is_same_v<T, td_api::chatTypePrivate>) {
                entry.is_private = true;
            } else if constexpr (std::is_same_v<T, td_api::chatTypeBasicGroup>) {
                entry.is_group = true;
            } else if constexpr (std::is_same_v<T, td_api::chatTypeSupergroup>) {
                entry.is_group = !type.is_channel_;
                entry.is_channel = type.is_channel_;
            }
        });
    }

    // Replace if exists, otherwise push
    auto it = std::find_if(state_.chats.begin(), state_.chats.end(), [&](const auto& c){ return c.id == entry.id; });
    if (it != state_.chats.end()) {
        *it = std::move(entry);
    } else {
        state_.chats.push_back(std::move(entry));
    }
    
    std::sort(state_.chats.begin(), state_.chats.end(), [](const ChatEntry& a, const ChatEntry& b) {
        if (a.is_pinned != b.is_pinned) return a.is_pinned > b.is_pinned;
        return a.order > b.order;
    });
}

void TdClient::on_update_chat_last_message(td_api::updateChatLastMessage& update) {
    std::lock_guard<std::mutex> lock(state_.mtx);

    for (auto& chat : state_.chats) {
        if (chat.id == update.chat_id_) {
            if (update.last_message_ && update.last_message_->content_) {
                td_api::downcast_call(*update.last_message_->content_, [&chat](auto& content) {
                    using T = std::decay_t<decltype(content)>;
                    if constexpr (std::is_same_v<T, td_api::messageText>) {
                        if (content.text_) chat.last_message = content.text_->text_;
                    } else if constexpr (std::is_same_v<T, td_api::messagePhoto>) {
                        chat.last_message = "[Photo]";
                    } else if constexpr (std::is_same_v<T, td_api::messageVideo>) {
                        chat.last_message = "[Video]";
                    } else if constexpr (std::is_same_v<T, td_api::messageDocument>) {
                        chat.last_message = "[File]";
                    } else if constexpr (std::is_same_v<T, td_api::messageSticker>) {
                        chat.last_message = "[Sticker]";
                    } else if constexpr (std::is_same_v<T, td_api::messageVoiceNote>) {
                        chat.last_message = "[Voice]";
                    } else {
                        chat.last_message = "[Message]";
                    }
                });
                chat.last_date = update.last_message_->date_;
            }
            // Update positions
            for (const auto& pos : update.positions_) {
                if (pos && pos->list_) {
                    if (pos->list_->get_id() == td_api::chatListMain::ID) {
                        chat.order = pos->order_;
                        chat.is_pinned = pos->is_pinned_;
                    }
                }
            }
            break;
        }
    }
}

void TdClient::on_update_chat_position(td_api::updateChatPosition& update) {
    std::lock_guard<std::mutex> lock(state_.mtx);

    if (!update.position_) return;

    for (auto& chat : state_.chats) {
        if (chat.id == update.chat_id_) {
            chat.order = update.position_->order_;
            chat.is_pinned = update.position_->is_pinned_;
            break;
        }
    }
}

void TdClient::on_update_chat_read_inbox(td_api::updateChatReadInbox& update) {
    std::lock_guard<std::mutex> lock(state_.mtx);

    for (auto& chat : state_.chats) {
        if (chat.id == update.chat_id_) {
            chat.unread_count = update.unread_count_;
            break;
        }
    }
}

void TdClient::on_update_user(td_api::updateUser& update) {
    if (!update.user_) return;

    // Check if this is our own user
    std::lock_guard<std::mutex> lock(state_.mtx);
    if (update.user_->id_ == state_.current_user.id || state_.current_user.id == 0) {
        // We'll set current user in Auth after getting getMe
    }
}

void TdClient::on_update_connection_state(td_api::updateConnectionState& update) {
    if (!update.state_) return;

    std::lock_guard<std::mutex> lock(state_.mtx);

    td_api::downcast_call(*update.state_, [this](auto& conn) {
        using T = std::decay_t<decltype(conn)>;
        if constexpr (std::is_same_v<T, td_api::connectionStateWaitingForNetwork>) {
            state_.connection = ConnectionStatus::OFFLINE;
        } else if constexpr (std::is_same_v<T, td_api::connectionStateConnectingToProxy>) {
            state_.connection = ConnectionStatus::CONNECTING;
        } else if constexpr (std::is_same_v<T, td_api::connectionStateConnecting>) {
            state_.connection = ConnectionStatus::CONNECTING;
        } else if constexpr (std::is_same_v<T, td_api::connectionStateUpdating>) {
            state_.connection = ConnectionStatus::UPDATING;
        } else if constexpr (std::is_same_v<T, td_api::connectionStateReady>) {
            state_.connection = ConnectionStatus::READY;
        }
    });
}

} // namespace tgcli
