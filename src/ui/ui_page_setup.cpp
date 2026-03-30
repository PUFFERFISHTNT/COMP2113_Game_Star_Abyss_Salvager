#include "ui/star_abyss_app.h"

#include "ui/ui_panels.h"

using namespace ftxui;

// Launch setup remains small, but isolating it now avoids mixing configuration
// widgets with the much denser battle/map rendering logic.
Element StarAbyssApp::RenderSetup() const {
    Element content = vbox({
        text(Tr(language_, "Launch Setup", "出航设置")) | bold | color(kGold) | center,
        text(Tr(language_, "Pick difficulty and hull profile before diving in.", "在跃入深渊前，先选择难度与舰体。")) | color(kHull) | center,
        separatorDouble(),
        window(text(Tr(language_, "Difficulty", "难度")) | color(kHull), difficultyToggle_->Render()) | bgcolor(kPanel),
        window(text(Tr(language_, "Starter Ship", "初始飞船")) | color(kGold), shipToggle_->Render()) | bgcolor(kPanel),
        separator(),
        hbox({startRunButton_->Render() | flex, setupBackButton_->Render() | flex}),
    });

    return content | vscroll_indicator | yframe | borderDouble | bgcolor(kAbyss);
}
