#pragma once
// ── Telegram CLI — TDLib Client Wrapper ─────────────────────────────────────
#include <td/telegram/Client.h>
#include <td/telegram/td_api.h>
#include <td/telegram/td_api.hpp>
#include "../app/State.hpp"

#include <functional>
#include <map>
#include <mutex>
#include <atomic>
#include <thread>
#include <memory>
#include <condition_variable>

namespace td_api = td::td_api;

namespace tgcli {

class TdClient {
public:
    using Object = td_api::object_ptr<td_api::Object>;
    using UpdateCallback = std::function<void()>;

    TdClient(AppState& state);
    ~TdClient();

    // Start the event loop thread
    void start();
    void stop();

    // Send a TDLib request (fire-and-forget)
    void send(td_api::object_ptr<td_api::Function> request);

    // Send a request and wait for response
    Object send_sync(td_api::object_ptr<td_api::Function> request, int timeout_sec = 10);

    // Register a callback for UI refresh
    void set_update_callback(UpdateCallback cb) { update_callback_ = cb; }

    // Reference to shared state
    AppState& state() { return state_; }

private:
    void event_loop();
    void process_update(td_api::object_ptr<td_api::Object> update);
    void process_auth_state(td_api::object_ptr<td_api::AuthorizationState> auth_state);

    // Handle specific update types
    void on_update_new_message(td_api::updateNewMessage& update);
    void on_update_chat_last_message(td_api::updateChatLastMessage& update);
    void on_update_chat_position(td_api::updateChatPosition& update);
    void on_update_chat_read_inbox(td_api::updateChatReadInbox& update);
    void on_update_user(td_api::updateUser& update);
    void on_update_connection_state(td_api::updateConnectionState& update);

    // TDLib client
    std::unique_ptr<td::ClientManager> client_manager_;
    int32_t client_id_ = 0;

    // State
    AppState& state_;

    // Event loop
    std::thread event_thread_;
    std::atomic<bool> running_{false};

    // Request tracking
    std::uint64_t request_id_{0};
    std::mutex response_mtx_;
    std::condition_variable response_cv_;
    std::map<std::uint64_t, Object> responses_;

    // UI callback
    UpdateCallback update_callback_;

    std::uint64_t next_request_id() { return ++request_id_; }
};

} // namespace tgcli
