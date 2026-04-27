#pragma once
#include <ftxui/component/component.hpp>
#include "../app/State.hpp"
namespace tgcli::ui {
class InfoPanel {
public:
    InfoPanel(AppState& state);
    ftxui::Component component();
private:
    AppState& state_;
};
} // namespace tgcli::ui
