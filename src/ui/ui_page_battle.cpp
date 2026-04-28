#include "ui/star_abyss_app.h"

#include <algorithm>
#include <vector>

#include "ui/ui_art.h"
#include "ui/ui_panels.h"

#include <ftxui/screen/terminal.hpp>

using namespace ftxui;

namespace {

// Chooses how many battle cards fit in each row from terminal width and visible slots and returns it.
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

// Calculates a stable battle card width from terminal width and row count and returns it.
int BattleCardWidth(int terminal_width, int cards_per_row) {
    const int safe_columns = std::max(1, cards_per_row);
    const int available_width = std::max(24, terminal_width - 8);
    const int computed_width = available_width / safe_columns;
    return std::max(22, std::min(34, computed_width));
}

// Builds localized detail lines for the given auxiliary ship and returns them.
std::vector<std::string> AuxiliaryShipDetailLines(UiLanguage language, const AuxiliaryShipState& ship) {
    std::vector<std::string> lines;
    std::vector<std::string> primary;
    if (ship.attack > 0) {
        primary.push_back(Tr(language, "ATK ", "攻击 ") + std::to_string(ship.attack));
    }
    if (ship.shield > 0) {
        primary.push_back(Tr(language, "SHD ", "护盾 ") + std::to_string(ship.shield));
    }
    if (ship.heal > 0) {
        primary.push_back(Tr(language, "REP ", "修复 ") + std::to_string(ship.heal));
    }
    if (!primary.empty()) {
        std::string joined = primary.front();
        for (size_t i = 1; i < primary.size(); ++i) {
            joined += " / " + primary[i];
        }
        lines.push_back(joined);
    }
    if (ship.endTurnExtraOtherTriggers > 0) {
        lines.push_back(Tr(language, "Other Aux EOT x", "其他附属舰在回合末额外触发 x") +
                        std::to_string(ship.endTurnExtraOtherTriggers));
    }
    if (ship.endTurnDamage > 0) {
        lines.push_back(Tr(language, "EOT DMG ", "回合末伤害 ") + std::to_string(ship.endTurnDamage));
    }
    if (ship.endTurnEnemyHeat > 0) {
        lines.push_back(Tr(language, "EOT Heat +", "回合末敌方热量 +") + std::to_string(ship.endTurnEnemyHeat));
    }
    lines.push_back(ship.remainingTurns > 0
                        ? Tr(language, "T ", "回合 ") + std::to_string(ship.remainingTurns)
                        : Tr(language, "T INF", "回合 常驻"));
    return lines;
}

// Renders one auxiliary slot from its index, optional ship, and highlight state and returns the element.
Element AuxiliaryShipSlot(UiLanguage language, int slot_index, const AuxiliaryShipState* ship, bool highlighted) {
    if (ship == nullptr) {
        return vbox({
                   text("[" + std::to_string(slot_index + 1) + "] " + Tr(language, "Empty Slot", "空槽位")) | color(kDim) | bold,
                   text(Tr(language, "Reserve bay for support craft", "附属舰停机位待命")) | color(Color::RGB(108, 120, 142)),
               }) |
               borderStyled(ROUNDED, highlighted ? Color::RGB(242, 190, 77) : Color::RGB(76, 87, 110)) |
               bgcolor(highlighted ? Color::RGB(42, 34, 12) : Color::RGB(10, 17, 31));
    }

    struct VisualStyle {
        std::string icon;
        Color accent;
        Color background;
    };
    auto style_for_ship = [](const AuxiliaryShipState& value) {
        if (value.unitId == "aux_interceptor") {
            return VisualStyle{"^>", Color::RGB(255, 137, 137), Color::RGB(52, 20, 28)};
        }
        if (value.unitId == "aux_guardian") {
            return VisualStyle{"[]", Color::RGB(124, 178, 255), Color::RGB(16, 31, 56)};
        }
        if (value.unitId == "aux_repair") {
            return VisualStyle{"+", Color::RGB(130, 236, 164), Color::RGB(14, 37, 26)};
        }
        if (value.unitId == "aux_pulse") {
            return VisualStyle{"~*", Color::RGB(246, 196, 91), Color::RGB(50, 34, 13)};
        }
        if (value.unitId == "aux_queen_wasp") {
            return VisualStyle{"W*", Color::RGB(255, 186, 76), Color::RGB(58, 31, 10)};
        }
        if (value.unitId == "aux_thermal_escort") {
            return VisualStyle{"T*", Color::RGB(255, 128, 91), Color::RGB(61, 22, 16)};
        }
        if (value.unitId == "aux_burner") {
            return VisualStyle{"B*", Color::RGB(255, 95, 95), Color::RGB(62, 16, 16)};
        }
        return VisualStyle{"<>", Color::RGB(146, 228, 255), Color::RGB(14, 31, 52)};
    };
    const VisualStyle style = style_for_ship(*ship);

    // Keep each slot compact and scannable so the player can confirm support
    // ship composition without opening extra panels.
    Elements content = {
        text("[" + std::to_string(slot_index + 1) + "] " + style.icon + " " +
             TranslateGameText(language, ship->displayName)) |
            color(style.accent) | bold,
    };
    for (const std::string& line : AuxiliaryShipDetailLines(language, *ship)) {
        content.push_back(text(line) | color(Color::White));
    }
    return vbox(std::move(content)) |
           borderStyled(ROUNDED, highlighted ? Color::RGB(242, 190, 77) : style.accent) |
           bgcolor(highlighted ? Color::RGB(66, 49, 16) : style.background);
}

// Renders the auxiliary bay from current state, records slot boxes when requested, and returns the panel.
Element AuxiliaryShipPanel(UiLanguage language,
                           const RunState& state,
                           std::vector<ftxui::Box>* slot_boxes,
                           int selected_slot,
                           bool selection_pending) {
    Elements slots;
    const int slot_count = kMaxAuxiliaryShips;
    if (slot_boxes != nullptr) {
        slot_boxes->assign(static_cast<size_t>(slot_count), {});
    }
    for (int i = 0; i < slot_count; ++i) {
        const AuxiliaryShipState* ship =
            i < static_cast<int>(state.auxiliaryShips.size()) ? &state.auxiliaryShips[static_cast<size_t>(i)] : nullptr;
        const bool highlighted = selection_pending && selected_slot == i;
        Element slot = AuxiliaryShipSlot(language, i, ship, highlighted) | flex;
        if (slot_boxes != nullptr) {
            slot = slot | reflect((*slot_boxes)[static_cast<size_t>(i)]);
        }
        slots.push_back(std::move(slot));
        if (i + 1 < slot_count) {
            slots.push_back(separator());
        }
    }

    return window(text(Tr(language, "Auxiliary Bay", "附属舰机库")) | color(kHull) | bold,
                  vbox({
                      text(Tr(language, "Support slots: ", "支援位：") + std::to_string(slot_count) +
                           Tr(language, ". Newest summons replace the oldest when full.",
                              " 个。满位时新召唤会替换最旧附属舰。")) |
                          color(kDim),
                      text(Tr(language,
                              "Replacement counts as destruction. Replaced or destroyed ships trigger once at +50% Attack / Shield / Repair / Heat.",
                              u8"\u66ff\u6362\u6309\u6467\u6bc1\u5904\u7406\u3002\u88ab\u66ff\u6362\u6216\u88ab\u6467\u6bc1\u7684\u9644\u5c5e\u8230\u4f1a\u4ee5 +50% \u653b\u51fb / \u62a4\u76fe / \u6062\u590d / \u70ed\u91cf\u89e6\u53d1\u4e00\u6b21\u3002")) |
                          color(kDim),
                      separator(),
                      hbox(std::move(slots)),
                  })) |
           bgcolor(kPanel);
}

}  // namespace

// Battle is the densest screen in the project. Splitting it out keeps combat
// layout, card visualization, and action affordances in one dedicated unit.
// Renders the full battle page from current app and engine state and returns the page element.
Element StarAbyssApp::RenderBattle() const {
    const RunState& state = engine_.state();
    const int terminal_width = Terminal::Size().dimx;
    const bool aux_transform_pending = engine_.auxiliaryTransformSelectionPending();
    const bool aux_transform_target_pending = engine_.auxiliaryTransformTargetChoicePending();
    const bool blackbox_selection_pending = state.player.blackboxSelectionPending;
    const std::string battle_hint =
        blackbox_selection_pending
            ? Tr(language_, "Blackbox Protocol: click a hand card to exile as imprint",
                 "黑箱协议：点击一张手牌作为放逐刻印目标")
            : aux_transform_target_pending
                ? Tr(language_, "Wing Rotation: choose a target drone type with 1-4 or click below",
                     "机群轮转：按 1-4 或点击下方选择目标附属舰类型")
                : aux_transform_pending
                    ? Tr(language_, "Wing Rotation: choose an auxiliary ship with 1-3 or click a slot",
                         "机群轮转：按 1-3 或点击槽位选择要转换的附属舰")
            : (Tr(language_, "Turn ", "回合 ") + std::to_string(std::max(1, state.battleTurn)) + "  |  " +
               Tr(language_, "Play cards, then press E to end turn", "出牌后按 E 结束回合"));
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
    const int card_text_width = std::max(12, card_width - 4);
    Elements card_rows;
    Elements current_row;
    for (size_t i = 0; i < visible_slots; ++i) {
        const CardDef* card = i < hand.size() ? &hand[i].def : nullptr;
        const std::string raw_ref = i < hand.size() ? hand[i].ref : "";
        const bool blackbox_target = blackbox_selection_pending && card != nullptr && raw_ref.rfind("tmp|", 0) != 0;
        const bool blackbox_invalid = blackbox_selection_pending && card != nullptr && !blackbox_target;
        const bool playable = card != nullptr && engine_.canPlayCard(i);
        std::vector<std::string> facts = card != nullptr ? CardFacts(language_, *card)
                                                         : std::vector<std::string>{TranslateGameText(language_, "No module loaded.")};
        const std::string fact_text = CompactFactText(language_, facts);
        const std::string description_text =
            card != nullptr ? CardDescriptionText(language_, *card)
                            : Tr(language_, "Draw cards and route power to fill this slot.", "抽牌并调度能量来填满这个槽位。");
        const Color accent = CardAccent(card);
        const Color frame_color = CardFrameColor(card);
        Element panel = vbox({
                            text(BattleKeyLabel(i) + ". " + (card != nullptr ? TranslateGameText(language_, card->name)
                                                                             : TranslateGameText(language_, "Empty Slot"))) |
                                color(card != nullptr ? (playable ? accent : kDim) : kDim) | bold,
                            separator(),
                            battleCardButtons_[i]->Render(),
                            blackbox_selection_pending
                                ? text(blackbox_target
                                           ? Tr(language_, "Select this card as the imprint target",
                                                "选择这张牌作为刻印目标")
                                           : Tr(language_, "Temporary copies cannot be chosen",
                                                "临时复制品不能作为刻印目标")) |
                                      color(blackbox_target ? kGold : kEnemy)
                                : text(""),
                            hbox({
                                text(Tr(language_, "Cost ", "费用 ") + std::to_string(card != nullptr ? card->cost : 0)) | color(card != nullptr ? accent : kDim),
                                filler(),
                                text(card != nullptr ? TranslateGameText(language_, card->type) : TranslateGameText(language_, "No card drawn")) |
                                    color(card != nullptr ? (playable ? kHull : kDim) : kDim),
                            }),
                            separator(),
                            WrappedParagraph(description_text, card_text_width) | color(card != nullptr ? Color::White : kDim),
                            WrappedParagraph(fact_text, card_text_width) | color(card != nullptr ? kDim : Color::RGB(84, 93, 112)),
                        }) |
                        bgcolor(CardBackground(card, playable)) |
                        borderStyled(ROUNDED, blackbox_target ? kGold : (blackbox_invalid ? kEnemy : frame_color)) |
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
                    WrappedParagraph(CardDescriptionText(language_, card), std::max(16, terminal_width - 12)) | color(Color::White),
                    WrappedParagraph(fact_text.empty() ? Tr(language_, "No additional card facts.", "没有额外卡牌信息。") : fact_text,
                                     std::max(16, terminal_width - 12)) |
                        color(kDim),
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
        text(battle_hint) |
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
        blackbox_selection_pending
            ? window(text(Tr(language_, "Blackbox Imprint Selection", "黑箱刻印选择")) | color(kGold) | bold,
                     vbox({
                         paragraph(Tr(language_,
                                      "Choose 1 non-copy card from your hand. It will be exiled for this battle, and a [Copy] will be added to your hand each turn in addition to your normal draw.",
                                      "从你的手牌中选择 1 张非复制牌。它会在本场战斗中被移除，之后每回合开始时，都会额外将一张[复制品]加入你的手牌，不占用你正常的抽 5 张。")) |
                             color(Color::White),
                         separator(),
                         text(Tr(language_,
                                 "Gold frame: selectable target   Red frame: not selectable",
                                 "金框：可选目标   红框：不可选")) |
                             color(kDim),
                     })) |
                bgcolor(kPanel)
            : text(""),
        blackbox_selection_pending ? separator() : text(""),
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
            window(text(TranslateGameText(language_, state.enemy.name)) | color(kEnemy), vbox(std::move(enemy_lines))) |
                flex | bgcolor(kPanel),
        }),
        separator(),
        AuxiliaryShipPanel(language_,
                           state,
                           &battleAuxBoxes_,
                           engine_.auxiliaryTransformSelectedShipIndex(),
                           aux_transform_pending && !aux_transform_target_pending),
        separator(),
        aux_transform_target_pending
            ? window(text(Tr(language_, "Wing Rotation Target", "机群轮转目标")) | color(kGold),
                     hbox({
                         ActionPanel("[1] " + TranslateGameText(language_, battleTransformLabels_[0]),
                                     Tr(language_, "Transform the selected auxiliary ship into this basic type.",
                                        "将选中的附属舰转换成该基础类型。"),
                                     battleTransformButtons_[0],
                                     kHull),
                         ActionPanel("[2] " + TranslateGameText(language_, battleTransformLabels_[1]),
                                     Tr(language_, "Transform the selected auxiliary ship into this basic type.",
                                        "将选中的附属舰转换成该基础类型。"),
                                     battleTransformButtons_[1],
                                     kGold),
                         ActionPanel("[3] " + TranslateGameText(language_, battleTransformLabels_[2]),
                                     Tr(language_, "Transform the selected auxiliary ship into this basic type.",
                                        "将选中的附属舰转换成该基础类型。"),
                                     battleTransformButtons_[2],
                                     kEnemy),
                         ActionPanel("[4] " + TranslateGameText(language_, battleTransformLabels_[3]),
                                     Tr(language_, "Transform the selected auxiliary ship into this basic type.",
                                        "将选中的附属舰转换成该基础类型。"),
                                     battleTransformButtons_[3],
                                     kHull),
                     })) |
                bgcolor(kPanelAlt)
            : (aux_transform_pending
                   ? window(text(Tr(language_, "Wing Rotation", "机群轮转")) | color(kGold),
                            text(Tr(language_,
                                    "Select an occupied auxiliary slot [1-3] or click to choose the ship that will be transformed.",
                                    "按 [1-3] 或点击已占用的附属舰槽位，决定要被转换的目标。")) |
                                color(Color::White)) |
                         bgcolor(kPanelAlt)
                   : text("")),
        aux_transform_pending ? separator() : text(""),
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
