// ── Telegram CLI — Messaging Implementation ─────────────────────────────────
#include "Messages.hpp"
#include "../app/Config.hpp"
#include <algorithm>

namespace tgcli {

Messages::Messages(TdClient& client) : client_(client) {}

void Messages::load_chats(int limit) {
    auto result = client_.send_sync(
        td_api::make_object<td_api::getChats>(nullptr, limit)
    );

    if (!result || result->get_id() != td_api::chats::ID) return;

    auto& chats_obj = static_cast<td_api::chats&>(*result);
    auto& state = client_.state();

    std::vector<ChatEntry> new_chats;
    new_chats.reserve(chats_obj.chat_ids_.size());

    for (int64_t chat_id : chats_obj.chat_ids_) {
        auto chat_result = client_.send_sync(
            td_api::make_object<td_api::getChat>(chat_id)
        );
        if (!chat_result || chat_result->get_id() != td_api::chat::ID) continue;

        auto& chat = static_cast<td_api::chat&>(*chat_result);
        ChatEntry entry;
        entry.id = chat.id_;
        entry.title = chat.title_;
        entry.unread_count = chat.unread_count_;

        // Determine chat type
        if (chat.type_) {
            td_api::downcast_call(*chat.type_, [&entry](auto& type) {
                using T = std::decay_t<decltype(type)>;
                if constexpr (std::is_same_v<T, td_api::chatTypePrivate>) {
                    entry.is_private = true;
                } else if constexpr (std::is_same_v<T, td_api::chatTypeBasicGroup>) {
                    entry.is_group = true;
                } else if constexpr (std::is_same_v<T, td_api::chatTypeSupergroup>) {
                    if (type.is_channel_) {
                        entry.is_channel = true;
                    } else {
                        entry.is_group = true;
                    }
                } else if constexpr (std::is_same_v<T, td_api::chatTypeSecret>) {
                    entry.is_private = true;
                }
            });
        }

        // Extract last message preview
        if (chat.last_message_ && chat.last_message_->content_) {
            entry.last_date = chat.last_message_->date_;
            td_api::downcast_call(*chat.last_message_->content_, [&entry](auto& content) {
                using T = std::decay_t<decltype(content)>;
                if constexpr (std::is_same_v<T, td_api::messageText>) {
                    if (content.text_) {
                        entry.last_message = content.text_->text_;
                        // Truncate for preview
                        if (entry.last_message.length() > 50) {
                            entry.last_message = entry.last_message.substr(0, 47) + "...";
                        }
                    }
                } else if constexpr (std::is_same_v<T, td_api::messagePhoto>) {
                    entry.last_message = "[Photo]";
                } else if constexpr (std::is_same_v<T, td_api::messageVideo>) {
                    entry.last_message = "[Video]";
                } else if constexpr (std::is_same_v<T, td_api::messageDocument>) {
                    entry.last_message = "[File]";
                } else if constexpr (std::is_same_v<T, td_api::messageSticker>) {
                    entry.last_message = "[Sticker]";
                } else if constexpr (std::is_same_v<T, td_api::messageVoiceNote>) {
                    entry.last_message = "[Voice]";
                } else if constexpr (std::is_same_v<T, td_api::messageAnimation>) {
                    entry.last_message = "[GIF]";
                } else {
                    entry.last_message = "[Message]";
                }
            });
        }

        // Positions for ordering
        for (const auto& pos : chat.positions_) {
            if (pos && pos->list_) {
                if (pos->list_->get_id() == td_api::chatListMain::ID) {
                    entry.order = pos->order_;
                    entry.is_pinned = pos->is_pinned_;
                }
            }
        }

        new_chats.push_back(std::move(entry));
    }

    // Sort by order (descending, pinned first)
    std::sort(new_chats.begin(), new_chats.end(), [](const ChatEntry& a, const ChatEntry& b) {
        if (a.is_pinned != b.is_pinned) return a.is_pinned > b.is_pinned;
        return a.order > b.order;
    });

    std::lock_guard<std::mutex> lock(state.mtx);
    state.chats = std::move(new_chats);
}

void Messages::load_history(int64_t chat_id, int limit, int64_t from_message_id) {
    auto result = client_.send_sync(
        td_api::make_object<td_api::getChatHistory>(
            chat_id, from_message_id, 0, limit, false
        )
    );

    if (!result || result->get_id() != td_api::messages::ID) return;

    auto& msgs = static_cast<td_api::messages&>(*result);
    auto& state = client_.state();

    std::vector<MessageEntry> entries;
    entries.reserve(msgs.messages_.size());

    for (auto& msg_ptr : msgs.messages_) {
        if (!msg_ptr) continue;
        auto& msg = *msg_ptr;

        MessageEntry entry;
        entry.id = msg.id_;
        entry.chat_id = msg.chat_id_;
        entry.date = msg.date_;
        entry.is_outgoing = msg.is_outgoing_;

        // Sender
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

        // Resolve sender name
        entry.sender_name = get_user_name(entry.sender_id);

        // Content
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
                    if (content.document_) entry.file_name = content.document_->file_name_;
                    if (content.caption_) entry.media_caption = content.caption_->text_;
                } else if constexpr (std::is_same_v<T, td_api::messageSticker>) {
                    entry.media_type = "Sticker";
                    if (content.sticker_) entry.text = content.sticker_->emoji_;
                } else if constexpr (std::is_same_v<T, td_api::messageVoiceNote>) {
                    entry.media_type = "Voice";
                    if (content.caption_) entry.media_caption = content.caption_->text_;
                } else if constexpr (std::is_same_v<T, td_api::messageAnimation>) {
                    entry.media_type = "GIF";
                    if (content.caption_) entry.media_caption = content.caption_->text_;
                } else {
                    entry.media_type = "Other";
                }
            });
        }

        // Reply
        if (msg.reply_to_) {
            td_api::downcast_call(*msg.reply_to_, [&entry](auto& reply) {
                using T = std::decay_t<decltype(reply)>;
                if constexpr (std::is_same_v<T, td_api::messageReplyToMessage>) {
                    entry.reply_to_message_id = reply.message_id_;
                }
            });
        }

        // exteraGram badge
        entry.sender_has_extera_badge = state.is_extera_supporter(entry.sender_id);
        entry.sender_extera_status = state.extera_status(entry.sender_id);

        entries.push_back(std::move(entry));
    }

    // Reverse to chronological order (TDLib returns newest first)
    std::reverse(entries.begin(), entries.end());

    std::lock_guard<std::mutex> lock(state.mtx);
    if (state.selected_chat_id != chat_id) {
        return;
    }
    if (from_message_id == 0) {
        state.messages = std::move(entries);
    } else {
        // Prepend older messages
        state.messages.insert(state.messages.begin(), entries.begin(), entries.end());
    }
}

void Messages::send_text(int64_t chat_id, const std::string& text, int64_t reply_to_msg_id) {
    auto content = td_api::make_object<td_api::inputMessageText>(
        td_api::make_object<td_api::formattedText>(text, std::vector<td_api::object_ptr<td_api::textEntity>>()),
        nullptr, false
    );

    td_api::object_ptr<td_api::InputMessageReplyTo> reply_to;
    if (reply_to_msg_id != 0) {
        reply_to = td_api::make_object<td_api::inputMessageReplyToMessage>(reply_to_msg_id, nullptr, 0, "");
    }

    client_.send(td_api::make_object<td_api::sendMessage>(
        chat_id, nullptr, std::move(reply_to), nullptr, nullptr, std::move(content)
    ));
}

void Messages::send_file(int64_t chat_id, const std::string& file_path) {
    auto content = td_api::make_object<td_api::inputMessageDocument>(
        td_api::make_object<td_api::inputFileLocal>(file_path),
        nullptr,
        false,
        td_api::make_object<td_api::formattedText>("", std::vector<td_api::object_ptr<td_api::textEntity>>())
    );

    client_.send(td_api::make_object<td_api::sendMessage>(
        chat_id, nullptr, nullptr, nullptr, nullptr, std::move(content)
    ));
}

void Messages::edit_text(int64_t chat_id, int64_t message_id, const std::string& new_text) {
    auto content = td_api::make_object<td_api::inputMessageText>(
        td_api::make_object<td_api::formattedText>(new_text, std::vector<td_api::object_ptr<td_api::textEntity>>()),
        nullptr, false
    );

    client_.send(td_api::make_object<td_api::editMessageText>(
        chat_id, message_id, nullptr, std::move(content)
    ));
}

void Messages::delete_message(int64_t chat_id, int64_t message_id, bool revoke) {
    std::vector<int64_t> msg_ids = {message_id};
    client_.send(td_api::make_object<td_api::deleteMessages>(chat_id, std::move(msg_ids), revoke));
}

void Messages::forward(int64_t from_chat_id, int64_t to_chat_id, int64_t message_id) {
    std::vector<int64_t> msg_ids = {message_id};
    client_.send(td_api::make_object<td_api::forwardMessages>(
        to_chat_id, nullptr, from_chat_id, std::move(msg_ids), nullptr, false, false
    ));
}

void Messages::add_reaction(int64_t chat_id, int64_t message_id, const std::string& emoji) {
    auto reaction = td_api::make_object<td_api::reactionTypeEmoji>(emoji);
    client_.send(td_api::make_object<td_api::addMessageReaction>(
        chat_id, message_id, std::move(reaction), false, false
    ));
}

void Messages::mark_read(int64_t chat_id, int64_t last_message_id) {
    std::vector<int64_t> msg_ids = {last_message_id};
    client_.send(td_api::make_object<td_api::viewMessages>(
        chat_id, std::move(msg_ids), nullptr, true
    ));
}

void Messages::fetch_chat_full_info(int64_t chat_id) {
    auto& state = client_.state();
    
    {
        std::lock_guard<std::mutex> lock(state.mtx);
        state.selected_chat_details = ChatFullInfo{};
        state.selected_chat_details.id = chat_id;
    }

    auto chat_res = client_.send_sync(td_api::make_object<td_api::getChat>(chat_id), 3);
    if (!chat_res || chat_res->get_id() != td_api::chat::ID) return;
    auto& chat = static_cast<td_api::chat&>(*chat_res);

    if (chat.type_->get_id() == td_api::chatTypePrivate::ID) {
        auto& priv = static_cast<td_api::chatTypePrivate&>(*chat.type_);
        int64_t user_id = priv.user_id_;

        auto user_res = client_.send_sync(td_api::make_object<td_api::getUser>(user_id), 3);
        if (user_res && user_res->get_id() == td_api::user::ID) {
            auto& user = static_cast<td_api::user&>(*user_res);
            std::lock_guard<std::mutex> lock(state.mtx);
            if (user.usernames_) state.selected_chat_details.username = user.usernames_->editable_username_;
            state.selected_chat_details.phone = user.phone_number_;
        }

        auto full_res = client_.send_sync(td_api::make_object<td_api::getUserFullInfo>(user_id), 3);
        if (full_res && full_res->get_id() == td_api::userFullInfo::ID) {
            auto& full = static_cast<td_api::userFullInfo&>(*full_res);
            std::lock_guard<std::mutex> lock(state.mtx);
            if (full.bio_) state.selected_chat_details.bio = full.bio_->text_;
        }
    } else if (chat.type_->get_id() == td_api::chatTypeBasicGroup::ID) {
        auto& group = static_cast<td_api::chatTypeBasicGroup&>(*chat.type_);
        auto full_res = client_.send_sync(td_api::make_object<td_api::getBasicGroupFullInfo>(group.basic_group_id_), 3);
        if (full_res && full_res->get_id() == td_api::basicGroupFullInfo::ID) {
            auto& full = static_cast<td_api::basicGroupFullInfo&>(*full_res);
            std::lock_guard<std::mutex> lock(state.mtx);
            state.selected_chat_details.description = full.description_;
        }
    } else if (chat.type_->get_id() == td_api::chatTypeSupergroup::ID) {
        auto& group = static_cast<td_api::chatTypeSupergroup&>(*chat.type_);
        auto full_res = client_.send_sync(td_api::make_object<td_api::getSupergroupFullInfo>(group.supergroup_id_), 3);
        if (full_res && full_res->get_id() == td_api::supergroupFullInfo::ID) {
            auto& full = static_cast<td_api::supergroupFullInfo&>(*full_res);
            std::lock_guard<std::mutex> lock(state.mtx);
            state.selected_chat_details.description = full.description_;
        }
    }

    // Determine whether current user can send messages in this chat (getChatMember)
    try {
        auto my_id = state.current_user.id;
        if (my_id != 0) {
            auto member_res = client_.send_sync(td_api::make_object<td_api::getChatMember>(chat_id, td_api::make_object<td_api::messageSenderUser>(my_id)), 3);
            if (member_res && member_res->get_id() == td_api::chatMember::ID) {
                auto& member = static_cast<td_api::chatMember&>(*member_res);
                bool can_send = true;
                bool is_admin = false;
                if (member.status_) {
                    td_api::downcast_call(*member.status_, [&can_send, &is_admin](auto& status) {
                        using T = std::decay_t<decltype(status)>;
                        if constexpr (std::is_same_v<T, td_api::chatMemberStatusRestricted>) {
                            // Treat restricted as disallowing send (TDLib has fine-grained flags but older versions may not expose them uniformly)
                            can_send = false;
                        } else if constexpr (std::is_same_v<T, td_api::chatMemberStatusAdministrator>) {
                            is_admin = true;
                        } else if constexpr (std::is_same_v<T, td_api::chatMemberStatusCreator>) {
                            is_admin = true;
                        } else if constexpr (std::is_same_v<T, td_api::chatMemberStatusBanned>) {
                            can_send = false;
                        } else if constexpr (std::is_same_v<T, td_api::chatMemberStatusLeft>) {
                            can_send = false;
                        }
                    });
                }
                std::lock_guard<std::mutex> lock(state.mtx);
                state.selected_chat_details.can_send_messages = can_send;
                state.selected_chat_details.user_is_admin = is_admin;
            }
        }
    } catch (...) {
        // Ignore any errors from TDLib sync calls; keep defaults
    }
}


std::string Messages::get_user_name(int64_t user_id) {
    if (user_id <= 0) return "Channel";

    auto result = client_.send_sync(td_api::make_object<td_api::getUser>(user_id), 3);
    if (!result || result->get_id() != td_api::user::ID) return "User";

    auto& user = static_cast<td_api::user&>(*result);
    std::string name = user.first_name_;
    if (!user.last_name_.empty()) {
        name += " " + user.last_name_;
    }
    return name.empty() ? "User" : name;
}

} // namespace tgcli
