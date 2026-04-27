// ── Telegram CLI — Auth Screen Implementation ───────────────────────────────
#include "AuthScreen.hpp"
#include "../app/Config.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/component/event.hpp>

#include <qrencode.h>

using namespace ftxui;

namespace tgcli::ui {

AuthScreen::AuthScreen(AppState& state, Auth& auth)
    : state_(state), auth_(auth) {}

// ── QR code generation using libqrencode ────────────────────────────────────
// Uses Unicode half-block characters for compact 2-row-per-line rendering.
// Upper half = top module, lower half = bottom module.
// ▀ = top dark, bottom light  (U+2580)
// ▄ = top light, bottom dark  (U+2584)
// █ = both dark               (U+2588)
//   = both light              (space)
std::vector<std::string> AuthScreen::generate_qr_lines(const std::string& data) {
    std::vector<std::string> lines;
    if (data.empty()) return lines;

    QRcode* qr = QRcode_encodeString(data.c_str(), 0, QR_ECLEVEL_L, QR_MODE_8, 1);
    if (!qr) return lines;

    int w = qr->width;

    // Add 1-module quiet zone on all sides
    auto is_dark = [&](int x, int y) -> bool {
        if (x < 0 || x >= w || y < 0 || y >= w) return false;
        return (qr->data[y * w + x] & 1) != 0;
    };

    // Process 2 rows at a time using half-block characters
    for (int y = -1; y < w + 1; y += 2) {
        std::string line;
        for (int x = -1; x < w + 1; x++) {
            bool top = is_dark(x, y);
            bool bot = is_dark(x, y + 1);

            if (top && bot) {
                line += "\xE2\x96\x88"; // █ full block
            } else if (top && !bot) {
                line += "\xE2\x96\x80"; // ▀ upper half
            } else if (!top && bot) {
                line += "\xE2\x96\x84"; // ▄ lower half
            } else {
                line += " ";
            }
        }
        lines.push_back(line);
    }

    QRcode_free(qr);
    return lines;
}

Component AuthScreen::component() {
    InputOption input_opt;
    input_opt.on_enter = [this] {
        AuthState st;
        { std::lock_guard<std::mutex> lock(state_.mtx); st = state_.auth_state; state_.auth_hint.clear(); }
        if (st == AuthState::WAIT_PHONE && !input_text_.empty()) {
            auth_.send_phone(input_text_);
            input_text_.clear();
        } else if (st == AuthState::WAIT_CODE && !input_text_.empty()) {
            auth_.send_code(input_text_);
            input_text_.clear();
        }
    };
    auto input = Input(&input_text_, "", input_opt);

    InputOption pass_opt;
    pass_opt.password = true;
    pass_opt.on_enter = [this] {
        { std::lock_guard<std::mutex> lock(state_.mtx); state_.auth_hint.clear(); }
        if (!password_text_.empty()) {
            std::string pwd = password_text_;
            pwd.erase(std::remove(pwd.begin(), pwd.end(), '\n'), pwd.end());
            pwd.erase(std::remove(pwd.begin(), pwd.end(), '\r'), pwd.end());
            auth_.send_password(pwd);
            password_text_.clear();
        }
    };
    auto password_input = Input(&password_text_, "", pass_opt) | ftxui::underlined;

    InputOption reg_opt;
    reg_opt.on_enter = [this] {
        { std::lock_guard<std::mutex> lock(state_.mtx); state_.auth_hint.clear(); }
        if (!first_name_.empty()) {
            auth_.send_registration(first_name_, last_name_);
            first_name_.clear();
            last_name_.clear();
        }
    };
    auto first_input = Input(&first_name_, "First name", reg_opt);
    auto last_input = Input(&last_name_, "Last name", reg_opt);

    auto tab_index = std::make_shared<int>(0);
    auto container = Container::Tab({
        input,
        password_input,
        Container::Vertical({first_input, last_input})
    }, tab_index.get());

    return Renderer(container, [this, input, password_input, first_input, last_input, tab_index]() {
        AuthState auth_state;
        std::string hint;
        std::string qr_link;
        {
            std::lock_guard<std::mutex> lock(state_.mtx);
            auth_state = state_.auth_state;
            hint = state_.auth_hint;
            qr_link = state_.qr_code_link;
        }

        switch (auth_state) {
            case AuthState::WAIT_PASSWORD:
                *tab_index = 1;
                break;
            case AuthState::WAIT_REGISTRATION:
                *tab_index = 2;
                break;
            default:
                *tab_index = 0;
                break;
        }
        auto& theme = Config::instance().theme;

        // ── Logo ────────────────────────────────────────────────────────
        auto logo = vbox({
            text("") | center,
            text("  T E L E G R A M  C L I") | bold | center,
            text("") | center,
        }) | color(Color::Palette256(theme.accent));

        Elements content;
        content.push_back(logo);
        content.push_back(separator() | color(Color::Palette256(theme.border_color)));

        switch (auth_state) {
            case AuthState::WAIT_QR_CODE: {
                content.push_back(text(""));
                content.push_back(text(" Scan QR code with your phone") | bold | center);
                content.push_back(text(" Open Telegram > Settings > Devices > Link Desktop") | dim | center);
                content.push_back(text(""));

                if (!qr_link.empty()) {
                    // Generate QR code lines (cache if link unchanged)
                    if (qr_link != last_qr_link_) {
                        last_qr_link_ = qr_link;
                        cached_qr_lines_ = generate_qr_lines(qr_link);
                    }

                    Elements qr_elements;
                    for (const auto& line : cached_qr_lines_) {
                        qr_elements.push_back(text(line) | center);
                    }

                    content.push_back(
                        vbox(std::move(qr_elements))
                            | center
                            | color(Color::Black)
                            | bgcolor(Color::White)
                    );
                    content.push_back(text(""));

                    // Show the actual link as well (for accessibility)
                    content.push_back(text(" " + qr_link) | dim | center);
                } else {
                    content.push_back(text(" Generating QR code...") | dim | center | blink);
                }

                content.push_back(text(""));
                content.push_back(separator() | color(Color::Palette256(theme.border_color)));
                content.push_back(text(" Press [P] to use phone number instead") | dim | center);
                break;
            }

            case AuthState::WAIT_PHONE:
                content.push_back(text(""));
                content.push_back(text(" Enter your phone number:") | bold);
                content.push_back(text(" Include country code, e.g. +1234567890") | dim);
                if (!hint.empty()) {
                    content.push_back(text("  " + hint) | color(Color::Palette256(196)));
                }
                content.push_back(text(""));
                content.push_back(hbox({
                    text("  \xEF\x87\xB2 ") | color(Color::Palette256(theme.accent)), // nf-md-phone
                    input->Render() | flex | underlined,
                }));
                content.push_back(text(""));
                content.push_back(text("Press Enter to continue") | dim | center);
                content.push_back(separator() | color(Color::Palette256(theme.border_color)));
                content.push_back(text("Press [Q] to switch to QR code login") | dim | center);
                break;

            case AuthState::WAIT_CODE:
                content.push_back(text(""));
                content.push_back(text(" Enter verification code:") | bold);
                if (!hint.empty()) {
                    content.push_back(text("  " + hint) | color(Color::Palette256(196)));
                }
                content.push_back(text(""));
                content.push_back(hbox({
                    text("  \xEF\x80\xA3 ") | color(Color::Palette256(theme.accent)), // nf-fa-key
                    input->Render() | flex | underlined,
                }));
                content.push_back(text(""));
                content.push_back(text(" Press Enter to verify") | dim | center);
                content.push_back(separator() | color(Color::Palette256(theme.border_color)));
                content.push_back(text(" Press [ESC] to restart / change number") | dim | center);
                break;

            case AuthState::WAIT_PASSWORD:
                content.push_back(text(""));
                content.push_back(text(" Two-Factor Authentication") | bold | center);
                content.push_back(text(" Enter your 2FA password:") | bold);
                content.push_back(text(""));
                if (!hint.empty()) {
                    content.push_back(text("  Hint: " + hint) | dim | color(Color::Palette256(220)));
                }
                content.push_back(text(""));
                content.push_back(hbox({
                    text("  \xEF\x80\xA3 ") | color(Color::Palette256(196)), // nf-fa-key (red for 2FA)
                    password_input->Render() | flex,
                }));
                content.push_back(text(""));
                content.push_back(text(" Press Enter to authenticate") | dim | center);
                content.push_back(text(""));
                content.push_back(
                    text(" Your password is protected with 2FA") 
                    | dim | center 
                    | color(Color::Palette256(theme.accent))
                );
                content.push_back(separator() | color(Color::Palette256(theme.border_color)));
                content.push_back(text(" Press [ESC] to restart / cancel") | dim | center);
                break;

            case AuthState::WAIT_REGISTRATION:
                content.push_back(text(""));
                content.push_back(text(" Create your account:") | bold);
                if (!hint.empty()) {
                    content.push_back(text("  " + hint) | color(Color::Palette256(196)));
                }
                content.push_back(text(""));
                content.push_back(hbox({text("  First name: "), first_input->Render() | flex | underlined}));
                content.push_back(hbox({text("  Last name:  "), last_input->Render() | flex | underlined}));
                content.push_back(text(""));
                content.push_back(text(" Press Enter to register") | dim | center);
                break;

            case AuthState::READY:
                content.push_back(text(""));
                content.push_back(
                    text(" Authenticated successfully!") 
                    | bold | center 
                    | color(Color::Palette256(76))  // green
                );
                content.push_back(text(" Loading chats...") | dim | center);
                break;

            case AuthState::NONE:
            case AuthState::LOGGING_OUT:
            case AuthState::CLOSED:
            default:
                content.push_back(text(""));
                content.push_back(text(" Connecting to Telegram...") | dim | center);
                content.push_back(text("   ...") | dim | center | blink);
                break;
        }

        return vbox(std::move(content))
            | border
            | color(Color::Palette256(theme.border_color))
            | size(WIDTH, LESS_THAN, 65)
            | size(HEIGHT, LESS_THAN, 35)
            | center;
    }) | CatchEvent([this](Event event) {
        AuthState auth_state;
        {
            std::lock_guard<std::mutex> lock(state_.mtx);
            auth_state = state_.auth_state;
        }

        // ── QR Code screen: P to switch to phone ────────────────────────
        if (auth_state == AuthState::WAIT_QR_CODE) {
            if (event == Event::Character('p') || event == Event::Character('P')) {
                auth_.switch_to_phone_login();
                return true;
            }
            return false;
        }

        // ── Phone screen: Q to switch to QR ─────────────────────────────
        if (auth_state == AuthState::WAIT_PHONE) {
            if (event == Event::Character('q') || event == Event::Character('Q')) {
                std::lock_guard<std::mutex> lock(state_.mtx);
                state_.use_qr_login = true;
                state_.auth_state = AuthState::WAIT_QR_CODE;
                // Re-request QR code auth
                auth_.request_qr_code();
                return true;
            }
        }

        // ── Escape to restart login ─────────────────────────────────────────────
        if (event == Event::Escape) {
            if (auth_state == AuthState::WAIT_CODE || auth_state == AuthState::WAIT_PASSWORD || auth_state == AuthState::WAIT_REGISTRATION) {
                auth_.logout(); // This tells TDLib to abort login and restart
                return true;
            }
        }

        if (event == Event::Return) {
            switch (auth_state) {
                case AuthState::WAIT_PHONE:
                case AuthState::WAIT_CODE:
                case AuthState::WAIT_PASSWORD:
                case AuthState::WAIT_REGISTRATION:
                    // Handled by InputOption::on_enter
                    return false;

                case AuthState::READY:
                    if (on_ready_) on_ready_();
                    return true;

                default:
                    return false;
            }
        }
        return false;
    });
}

} // namespace tgcli::ui
