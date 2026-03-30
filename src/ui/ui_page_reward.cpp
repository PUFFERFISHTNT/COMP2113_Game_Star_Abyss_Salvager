#include "ui/star_abyss_app.h"

#include <algorithm>
#include <vector>

#include "ui/ui_panels.h"

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/terminal.hpp>

using namespace ftxui;

namespace {

int RewardCardsPerRow(int terminal_width, size_t visible_slots) {
    if (visible_slots == 0) {
        return 1;
    }
    if (terminal_width >= 140) {
        return static_cast<int>(std::min<size_t>(3, visible_slots));
    }
    if (terminal_width >= 100) {
        return static_cast<int>(std::min<size_t>(2, visible_slots));
    }
    return 1;
}

int RewardCardWidth(int terminal_width, int cards_per_row) {
    const int safe_columns = std::max(1, cards_per_row);
    const int available_width = std::max(28, terminal_width - 12);
    const int computed_width = available_width / safe_columns;
    return std::max(24, std::min(40, computed_width));
}

}  // namespace

// Reward rendering is separated so future reward types can expand without
// touching battle resolution or summary/info pages.
Element StarAbyssApp::RenderReward() const {
    const RunState& state = engine_.state();
    const std::vector<const CardDef*> reward_cards = engine_.rewardCards();
    const int terminal_width = Terminal::Size().dimx;
    const size_t visible_slots = rewardButtons_.size();
    const int cards_per_row = RewardCardsPerRow(terminal_width, visible_slots);
    const int card_width = RewardCardWidth(terminal_width, cards_per_row);
    Elements reward_rows;
    Elements current_row;
    for (size_t i = 0; i < rewardButtons_.size(); ++i) {
        const CardDef* card = i < reward_cards.size() ? reward_cards[i] : nullptr;
        std::vector<std::string> fact_lines;
        if (card != nullptr) {
            fact_lines = CardFacts(language_, *card);
        } else {
            fact_lines.push_back(TranslateGameText(language_, "No salvage option."));
        }
        const std::string fact_text = CompactFactText(language_, fact_lines);
        Elements description_lines;
        if (card != nullptr) {
            for (const std::string& line : CardDescriptionLines(language_, *card)) {
                description_lines.push_back(text(line) | color(Color::White));
            }
        } else {
            description_lines.push_back(text(TranslateGameText(language_, "Unavailable")) | color(Color::White));
        }
        Element card_panel = window(text(std::to_string(i + 1) + ". " +
                                         (card != nullptr ? (Tr(language_, "[Card Reward] ", "[卡牌奖励] ") + rewardLabels_[i]) : rewardLabels_[i])) |
                                    color(kGold),
                                    vbox({
                                        rewardButtons_[i]->Render(),
                                        separator(),
                                        text(card != nullptr
                                                 ? (Tr(language_, "Cost ", "费用 ") + std::to_string(card->cost) + "   " +
                                                    Tr(language_, "Type: ", "类型：") + TranslateGameText(language_, card->type))
                                                 : Tr(language_, "No card offered in this slot.", "这个槽位没有可选卡牌。")) |
                                            color(card != nullptr ? kHull : kDim),
                                        separator(),
                                        vbox(std::move(description_lines)),
                                        paragraph(fact_text) | color(kDim),
                                    })) |
                             bgcolor(kPanel) |
                             size(WIDTH, EQUAL, card_width) |
                             size(HEIGHT, GREATER_THAN, 10);
        current_row.push_back(card_panel);
        if (static_cast<int>(current_row.size()) == cards_per_row) {
            reward_rows.push_back(hbox(std::move(current_row)));
            current_row = {};
        }
    }
    if (!current_row.empty()) {
        reward_rows.push_back(hbox(std::move(current_row)));
    }

    Elements cards;
    for (size_t row_index = 0; row_index < reward_rows.size(); ++row_index) {
        if (row_index > 0) {
            cards.push_back(separator());
        }
        cards.push_back(std::move(reward_rows[row_index]));
    }

    std::vector<std::string> relic_lines;
    if (!state.reward.relicId.empty()) {
        relic_lines.push_back(TranslateGameText(language_, engine_.relicNameForId(state.reward.relicId)));
        relic_lines.push_back(TranslateGameText(language_, engine_.relicDescriptionForId(state.reward.relicId)));
    } else {
        relic_lines.push_back(Tr(language_, "This reward contains cards only.", "本次奖励只有卡牌，没有遗物。"));
    }

    Element content = vbox({
        text(TranslateGameText(language_, state.reward.banner)) | bold | color(kGold) | center,
        text(state.reward.relicId.empty()
                 ? Tr(language_, "Reward type: Card reward", "奖励类型：卡牌奖励")
                 : Tr(language_, "Reward type: Card reward + relic reward", "奖励类型：卡牌奖励 + 遗物奖励")) |
            color(Color::White) | center,
        text(Tr(language_, "Credits gained: ", "获得金币：") + std::to_string(state.reward.scrap)) | color(kHull) | center,
        text(Tr(language_, "Current credits: ", "当前金币：") + std::to_string(state.player.scrap)) | color(kGold) | center,
        separatorDouble(),
        hbox({vbox(std::move(cards)) | flex, InfoPanel(language_, Tr(language_, "Relic Reward", "遗物奖励"), relic_lines, kEnemy) | flex}),
        separator(),
        text(ScrollHintLine(language_)) | color(kDim) | center,
        separator(),
        hbox({rewardSkipButton_->Render() | flex, rewardMenuButton_->Render() | flex}),
        text(TranslateGameText(language_, state.statusLine)) | color(Color::White),
    });

    return content | vscroll_indicator | yframe | borderDouble | bgcolor(kAbyss);
}
