#include "ui/star_abyss_app.h"

#include "ui/ui_panels.h"

using namespace ftxui;

// Generic information popups share one renderer so help, leaderboard, and save
// feedback all keep the same presentation contract.
Element StarAbyssApp::RenderInfo() const {
    const RunState& state = engine_.state();
    Elements lines;
    for (const std::string& line : state.info.lines) {
        lines.push_back(paragraph(TranslateGameText(language_, line)) | color(Color::White));
    }

    Element content = vbox({
        text(TranslateGameText(language_, state.info.title)) | bold | color(kHull) | center,
        separatorDouble(),
        window(text(Tr(language_, "Transmission", "讯息窗口")) | color(kGold), vbox(std::move(lines))) | bgcolor(kPanel),
        separator(),
        text(TranslateGameText(language_, state.info.footer)) | color(kGold),
        infoBackButton_->Render(),
    });

    return content | vscroll_indicator | yframe | borderDouble | bgcolor(kAbyss);
}
