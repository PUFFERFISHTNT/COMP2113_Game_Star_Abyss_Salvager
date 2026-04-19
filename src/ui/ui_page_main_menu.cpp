#include "ui/star_abyss_app.h"

#include "ui/ui_art.h"
#include "ui/ui_panels.h"

#include <ftxui/screen/terminal.hpp>

using namespace ftxui;

// Keeps the landing screen presentation isolated from run/battle UI so the
// menu can evolve independently from gameplay pages.
Element StarAbyssApp::RenderMainMenu() const {
    const int terminal_width = Terminal::Size().dimx;
    const bool compact_layout = terminal_width < 118;
    Elements player_lines = PlayerArt(engine_.state(), ambientTick_);
    Elements enemy_lines = EnemyArt(engine_.state(), ambientTick_);

    Element fleet_panels = compact_layout
                               ? vbox({
                                     window(text(Tr(language_, "Player Flagship", "玩家旗舰")) | color(kHull), vbox(std::move(player_lines))) |
                                         bgcolor(kPanel),
                                     separator(),
                                     window(text(Tr(language_, "Abyss Contact", "深渊接触目标")) | color(kEnemy), vbox(std::move(enemy_lines))) |
                                         bgcolor(kPanel),
                                 })
                               : hbox({
                                     window(text(Tr(language_, "Player Flagship", "玩家旗舰")) | color(kHull), vbox(std::move(player_lines))) |
                                         flex | bgcolor(kPanel),
                                     window(text(Tr(language_, "Abyss Contact", "深渊接触目标")) | color(kEnemy), vbox(std::move(enemy_lines))) |
                                         flex | bgcolor(kPanel),
                                 });

    Element content = vbox({
        text(Tr(language_, "STAR ABYSS SALVAGER", "星渊打捞者")) | bold | color(kHull) | center,
        text(Tr(language_, "Terminal roguelike salvage bridge", "终端肉鸽舰桥")) | color(kGold) | center,
        separatorDouble(),
        fleet_panels,
        separator(),
        window(text(Tr(language_, "Main Menu", "主菜单")) | color(kGold),
               vbox({
                   paragraph(Tr(language_,
                                "Chart a route through collapsing sectors, build a stronger module deck, and bring your ship home from the abyss.",
                                "在崩塌的星区中规划航线，构筑更强的模块卡组，把你的飞船从深渊中带回家。")) |
                       color(Color::White),
                   separator(),
                   menuButtons_[0]->Render(),
                   menuButtons_[1]->Render(),
                   menuButtons_[2]->Render(),
                   menuButtons_[3]->Render(),
                   menuButtons_[4]->Render(),
                   menuLibraryButton_->Render(),
                   menuButtons_[5]->Render(),
                   menuButtons_[6]->Render(),
               })) |
            bgcolor(kPanel),
        text(Tr(language_, "Current language: English", "当前语言：中文")) | color(kHull) | center,
    });

    return ApplyVerticalScrollFrame(content, pageScrollLines_[PageIndex(AppPage::MainMenu)]) | borderDouble | bgcolor(kAbyss);
}
