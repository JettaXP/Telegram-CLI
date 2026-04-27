#pragma once
#include <ftxui/component/component.hpp>
#include "../app/State.hpp"
namespace tgcli::ui {
class GiftsPanel {
public:
    GiftsPanel(AppState& state);
    ftxui::Component component();
private:
    AppState& state_;
    int selected_ = 0;
    std::string filter_ = "all"; // "all", "gifts", "nfts"
};
} // namespace tgcli::ui
