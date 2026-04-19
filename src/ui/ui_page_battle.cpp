#include "ui/star_abyss_app.h"

#include <algorithm>
#include <vector>

#include "ui/ui_art.h"
#include "ui/ui_panels.h"

#include <ftxui/screen/terminal.hpp>

using namespace ftxui;

namespace {

int BattleCardsPerRow(int terminal_width, size_t visible_slots) {
    if (visible_slots == 0) {
        return 1;
    }
    if (terminal_width >= 150) {
        return static_cast<int>(std::min<size_t>(5, visible_slots));
    }
    if (terminal_width >= 122) {
        return static_cast<int>(std::min<size_t>(4, visible_slots));
    }
    if (terminal_width >= 94) {
        return static_cast<int>(std::min<size_t>(3, visible_slots));
    }
    if (terminal_width >= 66) {
        return static_cast<int>(std::min<size_t>(2, visible_slots));
    }
    return 1;
}

int BattleCardWidth(int terminal_width, int cards_per_row) {
    const int safe_columns = std::max(1, cards_per_row);
    const int available_width = std::max(24, terminal_width - 8);
    const int computed_width = available_width / safe_columns;
    return std::max(22, std::min(34, computed_width));
}

}  // namespace

// Battle is the densest screen in the project. Splitting it out keeps combat
// layout, card visualization, and action affordances in one dedicated unit.
Element StarAbyssApp::RenderBattle() const {
    const RunState& state = engine_.state();
    const int terminal_width = Terminal::Size().dimx;
    Elements ship_lines = PlayerArt(state, ambientTick_);
    Elements enemy_lines = EnemyArt(state, ambientTick_);
    Elements lane_lines;
    for (const std::string& line : LaneRows(state, ambientTick_, LaneWidthHint(battleLaneRowsBox_))) {
        lane_lines.push_back(text(line) | color(kGold));
    }

    Elements log_lines;
    const size_t log_start = state.battleLog.size() > 8 ? state.battleLog.size() - 8 : 0;
    for (size_t i = log_start; i < state.battleLog.size(); ++i) {
        log_lines.push_back(text(TranslateGameText(language_, state.battleLog[i])) | color(Color::White));
    }
    if (log_lines.empty()) {
        log_lines.push_back(text(TranslateGameText(language_, "Awaiting the first exchange.")) | color(kDim));
    }

    const std::vector<CardView> hand = engine_.handCards();
    const bool inspecting_draw = battlePileInspector_ == BattlePileInspector::Draw;
    const bool inspecting_discard = battlePileInspector_ == BattlePileInspector::Discard;
    const size_t visible_slots = std::max<size_t>(5, std::min(hand.size(), battleCardButtons_.size()));
    const int cards_per_row = BattleCardsPerRow(terminal_width, visible_slots);
    const int card_width = BattleCardWidth(terminal_width, cards_per_row);
    Elements card_rows;
    Elements current_row;
    for (size_t i = 0; i < visible_slots; ++i) {
        const CardDef* card = i < hand.size() ? &hand[i].def : nullptr;
        const bool playable = card != nullptr && engine_.canPlayCard(i);
        std::vector<std::string> facts = card != nullptr ? CardFacts(language_, *card)
                                                         : std::vector<std::string>{TranslateGameText(language_, "No module loaded.")};
        const std::string fact_text = CompactFactText(language_, facts);
        Elements description_lines;
        if (card != nullptr) {
            for (const std::string& line : CardDescriptionLines(language_, *card)) {
                description_lines.push_back(text(line) | color(Color::White));
            }
        } else {
            description_lines.push_back(text(Tr(language_, "Draw cards and route power to fill this slot.", "抽牌并调度能量来填满这个槽位。")) | color(kDim));
        }
        const Color accent = CardAccent(card);
        const Color frame_color = CardFrameColor(card);
        Element panel = vbox({
                            text(BattleKeyLabel(i) + ". " + (card != nullptr ? TranslateGameText(language_, card->name)
                                                                             : TranslateGameText(language_, "Empty Slot"))) |
                                color(card != nullptr ? (playable ? accent : kDim) : kDim) | bold,
                            separator(),
                            battleCardButtons_[i]->Render(),
                            hbox({
                                text(Tr(language_, "Cost ", "费用 ") + std::to_string(card != nullptr ? card->cost : 0)) | color(card != nullptr ? accent : kDim),
                                filler(),
                                text(card != nullptr ? TranslateGameText(language_, card->type) : TranslateGameText(language_, "No card drawn")) |
                                    color(card != nullptr ? (playable ? kHull : kDim) : kDim),
                            }),
                            separator(),
                            vbox(std::move(description_lines)),
                            paragraph(fact_text) | color(card != nullptr ? kDim : Color::RGB(84, 93, 112)),
                        }) |
                        bgcolor(CardBackground(card, playable)) |
                        borderStyled(ROUNDED, frame_color) |
                        size(WIDTH, EQUAL, card_width) |
                        size(HEIGHT, GREATER_THAN, 10);

        current_row.push_back(panel);
        if (static_cast<int>(current_row.size()) == cards_per_row) {
            card_rows.push_back(hbox(std::move(current_row)));
            current_row = {};
        }
    }
    if (!current_row.empty()) {
        card_rows.push_back(hbox(std::move(current_row)));
    }

    Elements command_deck_rows;
    for (size_t row_index = 0; row_index < card_rows.size(); ++row_index) {
        if (row_index > 0) {
            command_deck_rows.push_back(separator());
        }
        command_deck_rows.push_back(std::move(card_rows[row_index]));
    }

    const std::vector<CardView> inspected_pile =
        inspecting_draw ? engine_.drawPileCards() : (inspecting_discard ? engine_.discardPileCards() : std::vector<CardView>{});
    Elements inspected_cards;
    if (!inspected_pile.empty()) {
        for (size_t i = 0; i < inspected_pile.size(); ++i) {
            const CardDef& card = inspected_pile[i].def;
            Elements description_lines;
            for (const std::string& line : CardDescriptionLines(language_, card)) {
                description_lines.push_back(text(line) | color(Color::White));
            }
            if (description_lines.empty()) {
                description_lines.push_back(text(TranslateGameText(language_, card.description)) | color(Color::White));
            }
            const std::vector<std::string> facts = CardFacts(language_, card);
            const std::string fact_text = CompactFactText(language_, facts);
            const std::string pile_position = i == 0
                                                  ? Tr(language_, "Top card", "顶部卡牌")
                                                  : (Tr(language_, "Position ", "位置 ") + std::to_string(i + 1));
            inspected_cards.push_back(
                vbox({
                    text(pile_position + "  |  " + TranslateGameText(language_, card.name)) | color(CardAccent(&card)) | bold,
                    text(Tr(language_, "Cost ", "费用 ") + std::to_string(card.cost) + "   " +
                         Tr(language_, "Type: ", "类型：") + TranslateGameText(language_, card.type)) |
                        color(kHull),
                    vbox(std::move(description_lines)),
                    paragraph(fact_text.empty() ? Tr(language_, "No additional card facts.", "没有额外卡牌信息。") : fact_text) | color(kDim),
                }) |
                borderStyled(ROUNDED, CardFrameColor(&card)) |
                bgcolor(CardBackground(&card, true)));
            if (i + 1 < inspected_pile.size()) {
                inspected_cards.push_back(separator());
            }
        }
    } else if (battlePileInspector_ != BattlePileInspector::None) {
        inspected_cards.push_back(
            text(inspecting_draw ? Tr(language_, "Draw pile empty. Reshuffle will wait for discard cards.", "抽牌堆为空。需要等待弃牌堆重洗。")
                                 : Tr(language_, "Discard pile empty. Played and discarded cards will appear here.", "弃牌堆为空。打出或弃置的卡牌会出现在这里。")) |
            color(kDim));
    }

    Element pile_inspector = text(Tr(language_, "Open a pile to inspect draw order or discarded cards during battle.", "在战斗中展开牌堆即可查看抽牌顺序或弃牌内容。")) |
                             color(kDim) | center;
    if (battlePileInspector_ != BattlePileInspector::None) {
        pile_inspector =
            window(text(inspecting_draw ? Tr(language_, "Draw Pile Inspector", "抽牌堆检视")
                                        : Tr(language_, "Discard Pile Inspector", "弃牌堆检视")) |
                       color(inspecting_draw ? kHull : kEnemy),
                   vbox({
                       text(inspecting_draw
                                ? Tr(language_, "Cards are listed from the next card you will draw downward.", "卡牌按你下一张将要抽到的顺序向下排列。")
                                : Tr(language_, "Cards are listed from the most recently discarded card downward.", "卡牌按最近弃置的顺序向下排列。")) |
                           color(Color::White),
                       separator(),
                       vbox(std::move(inspected_cards)),
                   })) |
            bgcolor(kPanel);
    }

    Element content = vbox({
        text(Tr(language_, "Battle Bridge", "战斗舰桥")) | bold | color(kEnemy) | center,
        text(Tr(language_, "Turn ", "回合 ") + std::to_string(std::max(1, state.battleTurn)) + "  |  " +
             Tr(language_, "Play cards, then press E to end turn", "出牌后按 E 结束回合")) |
            color(kGold) | center,
        separatorDouble(),
        hbox({
            BattleEntityPanel(Tr(language_, "Ship Systems", "舰船系统"),
                              Tr(language_, "Hull", "船体"),
                              state.player.hp,
                              state.player.maxHp,
                              kHull,
                              BattlePlayerSummaryLines(language_, state)) |
                flex,
            BattleEntityPanel(Tr(language_, "Enemy Intent", "敌方意图"),
                              Tr(language_, "Enemy Hull", "敌方船体"),
                              state.enemy.hp,
                              state.enemy.maxHp,
                              kEnemy,
                              BattleEnemySummaryLines(language_, state)) |
                flex,
        }),
        separator(),
        hbox({
            window(text(TranslateGameText(language_, state.player.shipName)) | color(kHull), vbox(std::move(ship_lines))) | flex | bgcolor(kPanel),
            window(text(Tr(language_, "Combat Lane", "战斗航道")) | color(kGold),
                   vbox({
                       vbox(std::move(lane_lines)) | xflex | reflect(battleLaneRowsBox_),
                       separator(),
                       text(state.animation.active
                                ? (Tr(language_, "Animation: ", "动画：") + TranslateGameText(language_, state.animation.label))
                                : Tr(language_, "Animation: idle drift", "动画：待机漂移")) |
                           color(Color::White),
                   })) |
                flex | bgcolor(kPanel),
            window(text(TranslateGameText(language_, state.enemy.name)) | color(kEnemy), vbox(std::move(enemy_lines))) | flex | bgcolor(kPanel),
        }),
        separator(),
        window(text(Tr(language_, "Command Deck", "指令手牌")) | color(kGold),
               vbox(std::move(command_deck_rows))) |
            bgcolor(kPanelAlt),
        separator(),
        hbox({
            ActionPanel(Tr(language_, "Owned Cards", "已拥有卡牌"),
                        Tr(language_, "Open the current deck viewer with full card details.", "打开当前牌组查看器并浏览所有卡牌详情。"),
                        battleOwnedCardsButton_,
                        kHull),
            ActionPanel(inspecting_draw ? Tr(language_, "Draw Pile Open", "抽牌堆已展开") : Tr(language_, "Draw Pile", "抽牌堆"),
                        inspecting_draw ? Tr(language_, "Click again to close. Top card is shown first in the inspector below.", "再次点击即可关闭。下方面板按顶部卡牌优先显示。")
                                        : Tr(language_, "Click to inspect upcoming draw order and pile contents.", "点击查看即将抽到的顺序和牌堆内容。"),
                        battleDrawPileButton_,
                        kHull),
            ActionPanel(inspecting_discard ? Tr(language_, "Discard Pile Open", "弃牌堆已展开") : Tr(language_, "Discard Pile", "弃牌堆"),
                        inspecting_discard ? Tr(language_, "Click again to close. Most recently discarded card is shown first.", "再次点击即可关闭。最近弃置的卡牌会显示在最前。")
                                           : Tr(language_, "Click to inspect discarded cards in most-recent-first order.", "点击查看弃置卡牌，按最近弃置优先显示。"),
                        battleDiscardPileButton_,
                        kEnemy),
        }),
        pile_inspector,
        separator(),
        hbox({
            ActionPanel(Tr(language_, "[E] End Turn", "[E] 结束回合"),
                        Tr(language_, "Resolve your remaining plays and pass to the enemy.", "结算本回合剩余操作，然后交给敌方行动。"),
                        battleEndTurnButton_,
                        kGold),
            ActionPanel(Tr(language_, "[S] Save Run", "[S] 保存航程"),
                        Tr(language_, "Create a manual save or update the autosave slot.", "创建手动存档，或更新自动存档槽。"),
                        battleSaveButton_,
                        kHull),
            ActionPanel(Tr(language_, "[Esc] Main Menu", "[Esc] 主菜单"),
                        Tr(language_, "Leave the battle screen and return to the main menu.", "离开战斗界面并返回主菜单。"),
                        battleMenuButton_,
                        kEnemy),
        }),
        window(text(Tr(language_, "Battle Log", "战斗日志")) | color(kHull), vbox(std::move(log_lines))) | bgcolor(kPanel),
        text(ScrollHintLine(language_)) | color(kDim) | center,
        text(TranslateGameText(language_, state.statusLine)) | color(Color::White),
    });

    return ApplyVerticalScrollFrame(content, pageScrollLines_[PageIndex(AppPage::Battle)]) | borderDouble | bgcolor(kAbyss);
}
