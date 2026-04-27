// ── Telegram-CLI — Commands Panel Implementation ────────────────────────────
#include "CommandsPanel.hpp"
#include "../app/Config.hpp"

using namespace ftxui;

namespace tgcli::ui {

CommandsPanel::CommandsPanel(AppState& state) : state_(state) {}

Component CommandsPanel::component() {
    auto run = [this](const std::string& cmd) {
        if (on_command_) {
            on_command_(cmd);
        }
    };

    auto btn_info = Button("Info", [run] { run("info"); });
    auto btn_stars = Button("Stars", [run] { run("stars"); });
    auto btn_gifts = Button("Gifts", [run] { run("gifts"); });
    auto btn_theme_dark = Button("Dark", [run] { run("theme dark"); });
    auto btn_theme_nord = Button("Nord", [run] { run("theme nord"); });
    auto btn_theme_gruvbox = Button("Gruvbox", [run] { run("theme gruvbox"); });
    auto btn_logout = Button("Logout", [run] { run("logout"); });
    auto btn_quit = Button("Quit", [run] { run("quit"); });

    auto buttons = Container::Vertical({
        btn_info,
        btn_stars,
        btn_gifts,
        btn_theme_dark,
        btn_theme_nord,
        btn_theme_gruvbox,
        btn_logout,
        btn_quit,
    });

    return Renderer(buttons, [this, buttons]() {
        std::lock_guard<std::mutex> lock(state_.mtx);
        auto& theme = Config::instance().theme;

        if (!state_.show_commands_panel) {
            return vbox({
                filler(),
            }) | size(WIDTH, EQUAL, 24);
        }

        Elements items;
        items.push_back(hbox({
            text(" Commands") | bold | color(Color::Palette256(theme.accent)),
            filler(),
            text(" ×") | dim,
        }));
        items.push_back(separator() | color(Color::Palette256(theme.border_color)));
        items.push_back(text(""));
        items.push_back(text(" Use Tab / Enter to press buttons") | dim);
        items.push_back(text(""));
        items.push_back(buttons->Render());
        items.push_back(text(""));
        items.push_back(separator() | color(Color::Palette256(theme.border_color)));
        items.push_back(text(" Ctrl+; open, Esc close") | dim);
        items.push_back(text(" Ctrl+V paste photo from clipboard") | dim);
        items.push_back(filler());

        return vbox(std::move(items))
            | size(WIDTH, EQUAL, 24)
            | bgcolor(Color::Palette256(theme.chatlist_bg));
    });
}

} // namespace tgcli::ui
