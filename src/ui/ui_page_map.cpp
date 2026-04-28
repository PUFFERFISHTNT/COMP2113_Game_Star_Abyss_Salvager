#include "ui/star_abyss_app.h"

#include <algorithm>

#include "ui/ui_panels.h"

#include <ftxui/screen/terminal.hpp>

using namespace ftxui;

// The map page groups route choice, run status, and installed relics. Keeping
// it separate makes future node types or route layouts straightforward to add.
// what it does: Renders the route map page with route options, stats, and relics.
// what the inputs are: None.
// what the outputs are: Returns the map page element (ftxui::Element).
Element StarAbyssApp::RenderMap() const {
    const RunState& state = engine_.state();
    const int terminal_width = Terminal::Size().dimx;
    const bool compact_layout = terminal_width < 118;
    const int deck_size = static_cast<int>(state.player.drawPile.size() + state.player.hand.size() + state.player.discardPile.size());
    Elements options;
    for (size_t i = 0; i < mapButtons_.size(); ++i) {
        const bool active = i < state.routeOptions.size();
        const std::string detail = active ? TranslateGameText(language_, state.routeOptions[i].detail)
                                          : Tr(language_, "No route available.", "没有可选航线。");
        const std::string danger = active ? (Tr(language_, "Danger ", "危险度 ") + std::to_string(state.routeOptions[i].danger))
                                          : Tr(language_, "Offline", "离线");
        options.push_back(window(text(std::to_string(i + 1) + ". " + mapLabels_[i]) | color(kGold),
                                 vbox({
                                     mapButtons_[i]->Render(),
                                     separator(),
                                     paragraph(detail) | color(Color::White),
                                     text(danger) | color(kEnemy),
                                 })) |
                          bgcolor(kPanel));
    }

    Element stats_section = compact_layout
                                ? vbox({
                                      GaugePanel(Tr(language_, "Hull", "船体"), state.player.hp, state.player.maxHp, kHull, kHull),
                                      GaugePanel(Tr(language_, "Heat", "热量"), state.player.heat, state.player.maxHeat, kHeat, kHeat),
                                      window(text(Tr(language_, "Run Stats", "本轮状态")) | color(kGold),
                                             vbox({
                                                 text(Tr(language_, "Credits ", "金币 ") + std::to_string(state.player.scrap)) | color(Color::White),
                                                 text(Tr(language_, "Deck ", "卡组 ") + std::to_string(deck_size) + Tr(language_, " cards", " 张")) | color(Color::White),
                                                 text(Tr(language_, "Relics ", "遗物 ") + std::to_string(state.player.relics.size())) | color(Color::White),
                                             })) |
                                          bgcolor(kPanel),
                                  })
                                : hbox({
                                      GaugePanel(Tr(language_, "Hull", "船体"), state.player.hp, state.player.maxHp, kHull, kHull) | flex,
                                      GaugePanel(Tr(language_, "Heat", "热量"), state.player.heat, state.player.maxHeat, kHeat, kHeat) | flex,
                                      window(text(Tr(language_, "Run Stats", "本轮状态")) | color(kGold),
                                             vbox({
                                                 text(Tr(language_, "Credits ", "金币 ") + std::to_string(state.player.scrap)) | color(Color::White),
                                                 text(Tr(language_, "Deck ", "卡组 ") + std::to_string(deck_size) + Tr(language_, " cards", " 张")) | color(Color::White),
                                                 text(Tr(language_, "Relics ", "遗物 ") + std::to_string(state.player.relics.size())) | color(Color::White),
                                             })) |
                                          bgcolor(kPanel) | flex,
                                  });

    Element body_section = compact_layout
                               ? vbox({
                                     vbox(std::move(options)),
                                     separator(),
                                     InfoPanel(language_, Tr(language_, "Installed Relics", "已安装遗物"), RelicLines(language_, engine_, state), kHull),
                                 })
                               : hbox({
                                     vbox(std::move(options)) | flex,
                                     InfoPanel(language_, Tr(language_, "Installed Relics", "已安装遗物"), RelicLines(language_, engine_, state), kHull) | flex,
                                 });

    Element content = vbox({
        text(Tr(language_, "Sector ", "星区 ") + std::to_string(state.sector) + "  |  " +
             Tr(language_, "Route ", "航段 ") + std::to_string(std::min(state.depth + 1, state.nodesPerSector + 1))) |
            bold | color(kHull) | center,
        stats_section,
        separatorDouble(),
        body_section,
        separator(),
        text(TranslateGameText(language_, state.statusLine)) | color(kGold),
        hbox({mapOwnedCardsButton_->Render() | flex, mapSaveButton_->Render() | flex, mapMenuButton_->Render() | flex}),
    });

    return ApplyVerticalScrollFrame(content, pageScrollLines_[PageIndex(AppPage::Map)]) | borderDouble | bgcolor(kAbyss);
}
