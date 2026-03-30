#include "ui/star_abyss_app.h"

#include "ui/ui_panels.h"

#include <ftxui/screen/terminal.hpp>

using namespace ftxui;

// Summary is intentionally isolated from transient battle/map widgets because it
// only renders the final report and post-run actions.
Element StarAbyssApp::RenderSummary() const {
    const RunState& state = engine_.state();
    const int terminal_width = Terminal::Size().dimx;
    const bool compact_layout = terminal_width < 96;
    Elements lines;
    for (const std::string& line : state.info.lines) {
        const std::string translated = TranslateGameText(language_, line);
        if (translated.empty()) {
            lines.push_back(text(""));
        } else {
            lines.push_back(paragraph(translated) | color(Color::White));
        }
    }

    Element action_buttons = compact_layout
                                 ? vbox({
                                       summaryMenuButton_->Render(),
                                       summaryNewRunButton_->Render(),
                                       summaryExitButton_->Render(),
                                   })
                                 : hbox({
                                       summaryMenuButton_->Render() | flex,
                                       summaryNewRunButton_->Render() | flex,
                                       summaryExitButton_->Render() | flex,
                                   });

    Element content = vbox({
        text(TranslateGameText(language_, state.info.title)) | bold | color(kGold) | center,
        separatorDouble(),
        window(text(Tr(language_, "Run Report", "航程报告")) | color(kHull), vbox(std::move(lines))) | bgcolor(kPanel),
        separator(),
        action_buttons,
    });

    return content | vscroll_indicator | yframe | borderDouble | bgcolor(kAbyss);
}
