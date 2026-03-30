#include "ui/ui_panels.h"

#include <algorithm>

#include <ftxui/component/component_options.hpp>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

const Color kAbyss = Color::RGB(5, 10, 24);
const Color kPanel = Color::RGB(16, 30, 56);
const Color kPanelAlt = Color::RGB(11, 21, 42);
const Color kHull = Color::RGB(83, 223, 255);
const Color kShield = Color::RGB(124, 178, 255);
const Color kHeat = Color::RGB(255, 111, 74);
const Color kGold = Color::RGB(246, 196, 91);
const Color kEnemy = Color::RGB(255, 129, 136);
const Color kDim = Color::RGB(104, 118, 140);

namespace {

std::string MetricText(const std::string& label, int value, int maxValue) {
    return label + " " + std::to_string(value) + "/" + std::to_string(maxValue);
}

std::string TrimAsciiSpaces(const std::string& value) {
    size_t begin = 0;
    while (begin < value.size() && value[begin] == ' ') {
        ++begin;
    }
    size_t end = value.size();
    while (end > begin && value[end - 1] == ' ') {
        --end;
    }
    return value.substr(begin, end - begin);
}

std::vector<std::string> SplitDisplayTextByPunctuation(const std::string& text) {
    static const std::vector<std::string> separators = {", ", ",", ".", "; ", ";", "，", "。", "；"};

    std::vector<std::string> lines;
    size_t cursor = 0;
    while (cursor < text.size()) {
        size_t next_pos = std::string::npos;
        size_t next_len = 0;
        for (const std::string& separator : separators) {
            const size_t pos = text.find(separator, cursor);
            if (pos == std::string::npos) {
                continue;
            }
            if (next_pos == std::string::npos || pos < next_pos) {
                next_pos = pos;
                next_len = separator.size();
            }
        }

        if (next_pos == std::string::npos) {
            const std::string tail = TrimAsciiSpaces(text.substr(cursor));
            if (!tail.empty()) {
                lines.push_back(tail);
            }
            break;
        }

        const std::string line = TrimAsciiSpaces(text.substr(cursor, next_pos - cursor + next_len));
        if (!line.empty()) {
            lines.push_back(line);
        }
        cursor = next_pos + next_len;
    }

    if (lines.empty() && !text.empty()) {
        lines.push_back(text);
    }
    return lines;
}

std::string JoinClauses(UiLanguage language, const std::vector<std::string>& clauses) {
    if (clauses.empty()) {
        return "";
    }
    const std::string separator = language == UiLanguage::Chinese ? "；" : "; ";
    std::string result;
    for (size_t i = 0; i < clauses.size(); ++i) {
        if (i > 0) {
            result += separator;
        }
        result += clauses[i];
    }
    result += language == UiLanguage::Chinese ? "。" : ".";
    return result;
}

std::string EnergyPips(int energy, int max_energy) {
    std::string out = "Energy ";
    for (int i = 0; i < max_energy; ++i) {
        out += (i < energy) ? "[]" : "..";
        if (i + 1 < max_energy) {
            out += " ";
        }
    }
    if (energy > max_energy) {
        out += " +" + std::to_string(energy - max_energy);
    }
    return out;
}

std::string IntentSummary(UiLanguage language, const RunState& state) {
    const std::string label = TranslateGameText(language, state.enemy.intentLabel);
    if (state.enemy.intentKind == "attack") {
        const int value = state.enemy.weakenTurns > 0 ? std::max(0, state.enemy.intentValue - 2) : state.enemy.intentValue;
        const std::string suffix = state.enemy.weakenTurns > 0 ? Tr(language, " (weakened)", "（已削弱）") : "";
        return label + " - " + Tr(language, "Attack ", "攻击 ") + std::to_string(value) + suffix;
    }
    if (state.enemy.intentKind == "shield") {
        if (state.enemy.shieldLockTurns > 0) {
            return label + " - " + Tr(language, "Shield blocked by jammer", "护盾已被干扰封锁");
        }
        return label + " - " + Tr(language, "Gain ", "获得 ") + std::to_string(state.enemy.intentValue) + Tr(language, " shield", " 点护盾");
    }
    if (state.enemy.intentKind == "heat") {
        return label + " - " + Tr(language, "Inflict ", "施加 ") + std::to_string(state.enemy.intentValue) + Tr(language, " heat", " 点热量");
    }
    if (state.enemy.intentKind == "emp") {
        return label + " - " + Tr(language, "Next turn -", "下回合 -") + std::to_string(state.enemy.intentValue) +
               Tr(language, " energy", " 能量");
    }
    if (state.enemy.intentKind == "charge") {
        return label + " - " + Tr(language, "Charge and gain ", "蓄力并获得 ") + std::to_string(state.enemy.intentValue) +
               Tr(language, " shield", " 点护盾");
    }
    return label;
}

}  // namespace

ButtonOption AccentButton(Color accent) {
    auto option = ButtonOption::Animated();
    option.transform = [accent](const EntryState& state) {
        Element label = text(" " + state.label + " ") | center;
        if (state.active) {
            label = label | color(Color::Black) | bgcolor(accent) | bold;
        } else if (state.focused) {
            label = label | color(Color::White) | bgcolor(Color::RGB(33, 58, 96)) | bold;
        } else {
            label = label | color(accent);
        }
        return label | border;
    };
    return option;
}

Element GaugePanel(const std::string& label, int value, int maxValue, Color gauge_color, Color text_color) {
    const int safe_max = std::max(1, maxValue);
    const float ratio = static_cast<float>(std::max(0, value)) / static_cast<float>(safe_max);
    return window(text(label) | color(text_color),
                  vbox({
                      text(MetricText(label, value, maxValue)) | color(Color::White),
                      gauge(ratio) | color(gauge_color),
                  })) |
           bgcolor(kPanel);
}

Element InfoPanel(UiLanguage language, const std::string& title, const std::vector<std::string>& lines, Color accent) {
    Elements content;
    for (const std::string& line : lines) {
        content.push_back(paragraph(line) | color(Color::White));
    }
    if (content.empty()) {
        content.push_back(text(TranslateGameText(language, "No signal.")) | color(kDim));
    }
    return window(text(title) | color(accent) | bold, vbox(std::move(content))) | bgcolor(kPanelAlt);
}

std::string BattleKeyLabel(size_t index) {
    return index == 9 ? "0" : std::to_string(index + 1);
}

std::string CardDescriptionText(UiLanguage language, const CardDef& card) {
    std::vector<std::string> clauses;
    if (card.shieldToDamage) {
        clauses.push_back(Tr(language, "Deal damage equal to your current shield, then lose that shield",
                             "造成等同于当前护盾值的伤害，然后失去这些护盾"));
    } else if (card.damage > 0) {
        std::string damage_clause;
        if (card.multiHit > 1) {
            damage_clause = Tr(language, "Deal ", "造成 ") + std::to_string(card.damage) +
                            Tr(language, " damage ", " 点伤害，共 ") + std::to_string(card.multiHit) +
                            Tr(language, " times", " 次");
        } else {
            damage_clause = Tr(language, "Deal ", "造成 ") + std::to_string(card.damage) + Tr(language, " damage", " 点伤害");
        }
        if (card.comboBonus > 0) {
            damage_clause += Tr(language, ", gain +", "；若本回合已打出其他攻击牌，则额外 +") + std::to_string(card.comboBonus) +
                             Tr(language, " damage if another attack was played this turn", " 点伤害");
        }
        clauses.push_back(damage_clause);
    }
    if (card.ignoreShield) clauses.push_back(Tr(language, "Ignore enemy shield", "无视敌方护盾"));
    if (card.shield > 0) clauses.push_back(Tr(language, "Gain ", "获得 ") + std::to_string(card.shield) + Tr(language, " shield", " 点护盾"));
    if (card.heal > 0) clauses.push_back(Tr(language, "Recover ", "恢复 ") + std::to_string(card.heal) + Tr(language, " HP", " 点生命"));
    if (card.draw > 0) clauses.push_back(Tr(language, "Draw ", "抽 ") + std::to_string(card.draw) + Tr(language, " cards", " 张牌"));
    if (card.energy > 0) clauses.push_back(Tr(language, "Gain ", "获得 ") + std::to_string(card.energy) + Tr(language, " energy", " 点能量"));
    if (card.nextTurnEnergy > 0) clauses.push_back(Tr(language, "Next turn, gain ", "下回合获得 ") + std::to_string(card.nextTurnEnergy) +
                                                   Tr(language, " extra energy", " 点额外能量"));
    if (card.heat > 0) clauses.push_back(Tr(language, "Gain ", "增加 ") + std::to_string(card.heat) + Tr(language, " heat", " 点热量"));
    if (card.heat < 0) clauses.push_back(Tr(language, "Reduce ", "降低 ") + std::to_string(-card.heat) + Tr(language, " heat", " 点热量"));
    if (card.selfDamage > 0) clauses.push_back(Tr(language, "Lose ", "失去 ") + std::to_string(card.selfDamage) + Tr(language, " HP", " 点生命"));
    if (card.weakenTurns > 0) clauses.push_back(Tr(language, "Enemy's next ", "敌方接下来 ") + std::to_string(card.weakenTurns) +
                                                Tr(language, " attacks lose 2 damage", " 次攻击各减少 2 点伤害"));
    if (card.removeEnemyShield > 0) clauses.push_back(Tr(language, "Remove enemy shield", "移除敌方护盾"));
    if (card.shieldLockTurns > 0) clauses.push_back(Tr(language, "Enemy cannot gain shield for ", "敌方 ") + std::to_string(card.shieldLockTurns) +
                                                    Tr(language, " turns", " 回合内无法获得护盾"));
    if (card.attackMultiplier > 1) clauses.push_back(Tr(language, "Your next attack deals x", "你的下一张攻击牌伤害变为 x") + std::to_string(card.attackMultiplier));
    if (card.halveHeat) clauses.push_back(Tr(language, "Halve current heat", "将当前热量减半"));
    if (card.clearHeat) clauses.push_back(Tr(language, "Remove all heat", "清空全部热量"));
    if (card.salvageBonus > 0) clauses.push_back(Tr(language, "If you win this battle, gain +", "若本场战斗获胜，额外获得 +") +
                                                 std::to_string(card.salvageBonus) + Tr(language, " credits", " 金币"));
    if (card.randomEffect) clauses.push_back(Tr(language, "Trigger one strong but unpredictable effect", "触发一个强力但不可预测的效果"));

    const std::string generated = JoinClauses(language, clauses);
    return generated.empty() ? TranslateGameText(language, card.description) : generated;
}

std::vector<std::string> CardDescriptionLines(UiLanguage language, const CardDef& card) {
    return SplitDisplayTextByPunctuation(TranslateGameText(language, card.description));
}

std::string CompactFactText(UiLanguage language, const std::vector<std::string>& facts) {
    if (facts.empty()) {
        return "";
    }
    const std::string separator = language == UiLanguage::Chinese ? "｜" : " | ";
    std::string result;
    for (size_t i = 0; i < facts.size(); ++i) {
        if (i > 0) {
            result += separator;
        }
        result += facts[i];
    }
    return result;
}

std::string ScrollHintLine(UiLanguage language) {
    return Tr(language,
              "Scroll: mouse wheel, PgUp/PgDn, Ctrl+Up/Ctrl+Down, or [ and ].",
              "滚动：鼠标滚轮、PgUp/PgDn、Ctrl+上/下，或 [ 和 ]。");
}

Color CardAccent(const CardDef* card) {
    if (card == nullptr) {
        return kDim;
    }
    if (card->type == "Attack") {
        return Color::RGB(255, 120, 120);
    }
    if (card->type == "Defense") {
        return kShield;
    }
    if (card->type == "Control") {
        return kGold;
    }
    if (card->type == "Risk") {
        return Color::RGB(255, 152, 82);
    }
    return kHull;
}

Color CardBackground(const CardDef* card, bool playable) {
    if (card == nullptr) {
        return kPanelAlt;
    }
    if (!playable) {
        return Color::RGB(12, 21, 38);
    }
    if (card->type == "Attack") {
        return Color::RGB(48, 18, 28);
    }
    if (card->type == "Defense") {
        return Color::RGB(18, 28, 58);
    }
    if (card->type == "Control") {
        return Color::RGB(49, 37, 15);
    }
    if (card->type == "Risk") {
        return Color::RGB(54, 29, 12);
    }
    return Color::RGB(13, 34, 49);
}

std::vector<std::string> CardFacts(UiLanguage language, const CardDef& card) {
    std::vector<std::string> facts;
    if (card.damage > 0) {
        std::string line = Tr(language, "DMG ", "伤害 ") + std::to_string(card.damage);
        if (card.multiHit > 1) {
            line += Tr(language, " x", " x") + std::to_string(card.multiHit);
        }
        if (card.comboBonus > 0) {
            line += Tr(language, ", combo +", "，连击 +") + std::to_string(card.comboBonus);
        }
        facts.push_back(line);
    }
    if (card.shield > 0) facts.push_back(Tr(language, "Shield +", "护盾 +") + std::to_string(card.shield));
    if (card.heal > 0) facts.push_back(Tr(language, "Heal +", "治疗 +") + std::to_string(card.heal));
    if (card.draw > 0) facts.push_back(Tr(language, "Draw +", "抽牌 +") + std::to_string(card.draw));
    if (card.energy > 0) facts.push_back(Tr(language, "Energy +", "能量 +") + std::to_string(card.energy));
    if (card.nextTurnEnergy > 0) facts.push_back(Tr(language, "Next turn +", "下回合 +") + std::to_string(card.nextTurnEnergy) + Tr(language, " energy", " 能量"));
    if (card.heat > 0) facts.push_back(Tr(language, "Heat +", "热量 +") + std::to_string(card.heat));
    else if (card.heat < 0) facts.push_back(Tr(language, "Heat ", "热量 ") + std::to_string(card.heat));
    if (card.selfDamage > 0) facts.push_back(Tr(language, "Self dmg ", "自伤 ") + std::to_string(card.selfDamage));
    if (card.ignoreShield) facts.push_back(Tr(language, "Ignores shield", "无视护盾"));
    if (card.weakenTurns > 0) facts.push_back(Tr(language, "Weaken x", "削弱 x") + std::to_string(card.weakenTurns));
    if (card.shieldLockTurns > 0) facts.push_back(Tr(language, "Jam shield x", "护盾封锁 x") + std::to_string(card.shieldLockTurns));
    if (card.shieldToDamage) facts.push_back(Tr(language, "Convert own shield", "转化自身护盾"));
    if (card.attackMultiplier > 1) facts.push_back(Tr(language, "Next attack x", "下张攻击 x") + std::to_string(card.attackMultiplier));
    if (card.halveHeat) facts.push_back(Tr(language, "Halve heat", "热量减半"));
    if (card.clearHeat) facts.push_back(Tr(language, "Clear heat", "清空热量"));
    if (card.salvageBonus > 0) facts.push_back(Tr(language, "Battle credits +", "战斗金币 +") + std::to_string(card.salvageBonus));
    if (card.randomEffect) facts.push_back(Tr(language, "Volatile effect", "不稳定效果"));
    return facts;
}

std::vector<std::string> PlayerStatusLines(UiLanguage language, const RunState& state) {
    std::vector<std::string> lines = {
        Tr(language, "Hull ", "船体 ") + std::to_string(state.player.hp) + "/" + std::to_string(state.player.maxHp),
        Tr(language, "Heat ", "热量 ") + std::to_string(state.player.heat) + "/" + std::to_string(state.player.maxHeat),
        Tr(language, "Credits ", "金币 ") + std::to_string(state.player.scrap),
        Tr(language, "Deck ", "卡组 ") + std::to_string(state.player.drawPile.size() + state.player.hand.size() + state.player.discardPile.size()) +
            Tr(language, " cards", " 张"),
    };
    if (state.player.nextAttackMultiplier > 1) lines.push_back(Tr(language, "Target lock ready: next attack x", "目标锁定就绪：下一张攻击 x") + std::to_string(state.player.nextAttackMultiplier) + "。");
    if (state.player.salvageBonus > 0) lines.push_back(Tr(language, "Reward bonus +", "奖励金币加成 +") + std::to_string(state.player.salvageBonus) + Tr(language, " credits.", " 金币。"));
    if (state.player.openingEnergyBonus > 0) lines.push_back(Tr(language, "Next battle +", "下场战斗 +") + std::to_string(state.player.openingEnergyBonus) + Tr(language, " opening energy.", " 初始能量。"));
    if (state.player.openingShieldBonus > 0) lines.push_back(Tr(language, "Next battle +", "下场战斗 +") + std::to_string(state.player.openingShieldBonus) + Tr(language, " opening shield.", " 初始护盾。"));
    return lines;
}

std::vector<std::string> BattlePlayerSummaryLines(UiLanguage language, const RunState& state) {
    std::vector<std::string> lines = {
        Tr(language, "Energy ", "能量 ") + EnergyPips(state.player.energy, state.player.maxEnergy).substr(7),
        Tr(language, "Shield ", "护盾 ") + std::to_string(state.player.shield) + "   " + Tr(language, "Heat ", "热量 ") +
            std::to_string(state.player.heat) + "/" + std::to_string(state.player.maxHeat),
        Tr(language, "Hand ", "手牌 ") + std::to_string(state.player.hand.size()) + "   " +
            Tr(language, "Draw ", "抽牌堆 ") + std::to_string(state.player.drawPile.size()) + "   " +
            Tr(language, "Discard ", "弃牌堆 ") + std::to_string(state.player.discardPile.size()),
        Tr(language, "Credits ", "金币 ") + std::to_string(state.player.scrap),
    };
    if (state.player.nextTurnEnergy > 0) {
        lines.push_back(Tr(language, "Buffered next turn energy +", "下回合储能 +") + std::to_string(state.player.nextTurnEnergy));
    }
    if (state.player.nextAttackMultiplier > 1) {
        lines.push_back(Tr(language, "Target lock ready: next attack x", "目标锁定就绪：下一张攻击 x") + std::to_string(state.player.nextAttackMultiplier));
    }
    if (state.player.salvageBonus > 0) {
        lines.push_back(Tr(language, "Reward bonus +", "奖励金币加成 +") + std::to_string(state.player.salvageBonus) +
                        Tr(language, " credits", " 金币"));
    }
    return lines;
}

std::vector<std::string> BattleEnemySummaryLines(UiLanguage language, const RunState& state) {
    std::vector<std::string> lines = {
        Tr(language, "Intent: ", "意图：") + IntentSummary(language, state),
        Tr(language, "Shield ", "护盾 ") + std::to_string(state.enemy.shield),
    };
    if (state.enemy.weakenTurns > 0) lines.push_back(Tr(language, "Weakened attacks remaining: ", "剩余削弱攻击次数：") + std::to_string(state.enemy.weakenTurns));
    if (state.enemy.shieldLockTurns > 0) lines.push_back(Tr(language, "Shield jam remaining: ", "剩余护盾封锁回合：") + std::to_string(state.enemy.shieldLockTurns));
    return lines;
}

Element BattleEntityPanel(const std::string& title,
                          const std::string& hull_label,
                          int hp,
                          int max_hp,
                          Color accent,
                          const std::vector<std::string>& lines) {
    Elements content = {
        text(MetricText(hull_label, hp, max_hp)) | color(Color::White),
        gauge(static_cast<float>(std::max(0, hp)) / static_cast<float>(std::max(1, max_hp))) | color(accent),
        separator(),
    };
    for (const std::string& line : lines) {
        content.push_back(paragraph(line) | color(Color::White));
    }
    return window(text(title) | color(accent) | bold, vbox(std::move(content))) | bgcolor(kPanel);
}

Element ActionPanel(const std::string& title, const std::string& hint, const Component& button, Color accent) {
    return window(text(title) | color(accent) | bold,
                  vbox({
                      button->Render(),
                      separator(),
                      paragraph(hint) | color(kDim),
                  })) |
           flex | bgcolor(kPanelAlt);
}

std::vector<std::string> RelicLines(UiLanguage language, const GameEngine& engine, const RunState& state) {
    std::vector<std::string> lines;
    for (const std::string& relic_id : state.player.relics) {
        lines.push_back(TranslateGameText(language, engine.relicNameForId(relic_id)) + ": " +
                        TranslateGameText(language, engine.relicDescriptionForId(relic_id)));
    }
    if (lines.empty()) lines.push_back(TranslateGameText(language, "No relics installed."));
    return lines;
}
