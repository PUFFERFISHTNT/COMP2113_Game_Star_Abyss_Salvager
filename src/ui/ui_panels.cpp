#include "ui/ui_panels.h"

#include <algorithm>
#include <sstream>

#include <ftxui/component/component_options.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/string.hpp>

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

std::vector<std::string> WrapGlyphLines(const std::string& input, int width_hint) {
    const int safe_width = std::max(1, width_hint);
    std::vector<std::string> wrapped;
    std::stringstream stream(input);
    std::string raw_line;

    while (std::getline(stream, raw_line, '\n')) {
        if (raw_line.empty()) {
            wrapped.push_back("");
            continue;
        }

        std::string current_line;
        int current_width = 0;
        for (const std::string& glyph : Utf8ToGlyphs(raw_line)) {
            const int glyph_width = std::max(0, string_width(glyph));
            if (current_width > 0 && current_width + glyph_width > safe_width) {
                wrapped.push_back(current_line);
                current_line.clear();
                current_width = 0;
                if (glyph == " ") {
                    continue;
                }
            }
            current_line += glyph;
            current_width += glyph_width;
        }

        if (!current_line.empty()) {
            wrapped.push_back(current_line);
        }
    }

    if (wrapped.empty()) {
        wrapped.push_back("");
    }
    return wrapped;
}

std::string IntentSummary(UiLanguage language, const RunState& state, bool secondary = false) {
    if (state.difficulty >= 3) {
        return Tr(language, "Unknown", "未知");
    }

    const std::string& raw_kind = secondary ? state.enemy.secondaryIntentKind : state.enemy.intentKind;
    const std::string& raw_label = secondary ? state.enemy.secondaryIntentLabel : state.enemy.intentLabel;
    const int raw_value = secondary ? state.enemy.secondaryIntentValue : state.enemy.intentValue;
    const std::string label = TranslateGameText(language, raw_label);
    const auto enemy_overheat_stage = [&state]() {
        if (state.enemy.heat < state.enemy.maxHeat) {
            return 0;
        }
        return std::max(1, std::min(3, state.enemy.overheatRounds));
    };
    const auto charge_bonus_for_intent = [&state, secondary, &raw_kind]() {
        if (raw_kind != "attack") {
            return 0;
        }
        if (!secondary) {
            return std::max(0, state.enemy.nextAttackBonusDamage);
        }
        return state.enemy.intentKind == "attack" ? 0 : std::max(0, state.enemy.nextAttackBonusDamage);
    };
    const auto effective_enemy_attack = [&state, &enemy_overheat_stage, raw_value, &charge_bonus_for_intent]() {
        int value = raw_value;
        value += charge_bonus_for_intent();
        if (state.enemy.weakenTurns > 0) {
            value = std::max(0, value - 2);
        }
        if (enemy_overheat_stage() >= 1) {
            value = (value * 3) / 4;
        }
        return std::max(0, value);
    };
    if (raw_kind == "attack") {
        const int value = effective_enemy_attack();
        std::string suffix;
        if (state.enemy.weakenTurns > 0) {
            suffix += Tr(language, " (weakened)", "（已削弱）");
        }
        if (enemy_overheat_stage() >= 1) {
            suffix += Tr(language, " (overheated)", "（过热）");
        }
        return label + " - " + Tr(language, "Attack ", "攻击 ") + std::to_string(value) + suffix;
    }
    if (raw_kind == "shield") {
        if (state.enemy.shieldLockTurns > 0) {
            return label + " - " + Tr(language, "Shield blocked by jammer", "护盾已被干扰封锁");
        }
        return label + " - " + Tr(language, "Gain ", "获得 ") + std::to_string(raw_value) + Tr(language, " shield", " 点护盾");
    }
    if (raw_kind == "heat") {
        return label + " - " + Tr(language, "Inflict ", "施加 ") + std::to_string(raw_value) + Tr(language, " heat", " 点热量");
    }
    if (raw_kind == "emp") {
        return label + " - " + Tr(language, "Next turn -", "下回合 -") + std::to_string(raw_value) +
               Tr(language, " energy", " 能量");
    }
    if (raw_kind == "charge") {
        return label + " - " + Tr(language, "Charge next attack +", "为下次攻击蓄力 +") + std::to_string(raw_value) +
               Tr(language, " damage", " 点伤害");
    }
    if (raw_kind == "cool") {
        return label + " - " + Tr(language, "Reduce own heat ", "自身热量 -") + std::to_string(raw_value);
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

Element WrappedParagraph(const std::string& text_content, int width_hint) {
    Elements lines;
    for (const std::string& line : WrapGlyphLines(text_content, width_hint)) {
        lines.push_back(text(line));
    }
    return vbox(std::move(lines));
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
    const std::string authored_description =
        language == UiLanguage::English && !card.descriptionEn.empty() ? card.descriptionEn : card.description;
    if (!authored_description.empty()) {
        return authored_description;
    }

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
    if (card.enemyHeat > 0) clauses.push_back(Tr(language, "Add ", "使敌方增加 ") + std::to_string(card.enemyHeat) + Tr(language, " heat to the enemy", " 点热量"));
    if (card.enemyHeat < 0) clauses.push_back(Tr(language, "Reduce enemy heat by ", "使敌方降低 ") + std::to_string(-card.enemyHeat) + Tr(language, "", " 点热量"));
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
    if (card.summonDroneCount > 0) {
        const bool repair_drone = card.name.find("维修") != std::string::npos ||
                                  card.nameEn.find("Repair") != std::string::npos;
        std::string clause = Tr(language, "Deploy ", "部署 ") + std::to_string(card.summonDroneCount) +
                             Tr(language, " auxiliary drone(s)", " 架附属舰");
        if (card.summonDroneAttack > 0 || card.summonDroneShield > 0) {
            clause += Tr(language, " (", "（") + Tr(language, "ATK ", "攻击 ") +
                      std::to_string(card.summonDroneAttack) + ", ";
            clause += repair_drone ? Tr(language, "REP ", "修复 ") : Tr(language, "SHD ", "护盾 ");
            clause += std::to_string(card.summonDroneShield) + Tr(language, " each)", "）");
        }
        if (card.summonDroneDuration > 0) {
            clause += Tr(language, ", duration ", "，持续 ") + std::to_string(card.summonDroneDuration) +
                      Tr(language, " turns", " 回合");
        }
        clauses.push_back(clause);
    }
    if (card.buffDroneAttack != 0 || card.buffDroneShield != 0) {
        clauses.push_back(Tr(language, "All active auxiliary ships gain ", "所有在场附属舰获得 ") +
                          Tr(language, "ATK ", "攻击 ") + std::to_string(card.buffDroneAttack) + " / " +
                          Tr(language, "SHD ", "护盾 ") + std::to_string(card.buffDroneShield));
    }
    if (card.auxImmediateTriggerTimes > 0) {
        clauses.push_back(Tr(language, "All active auxiliary ships trigger immediately x",
                             "所有在场附属舰立即触发 x") + std::to_string(card.auxImmediateTriggerTimes));
    }
    if (card.auxEndTurnExtraTriggerTimes > 0) {
        clauses.push_back(Tr(language, "Auxiliary end-turn triggers +",
                             "附属舰回合结束额外触发 +") + std::to_string(card.auxEndTurnExtraTriggerTimes));
    }
    if (card.auxEffectPercentBonus > 0) {
        clauses.push_back(Tr(language, "Auxiliary effect output +", "附属舰效果提升 +") +
                          std::to_string(card.auxEffectPercentBonus) + "%");
    }
    if (card.auxDestroyCount > 0) {
        clauses.push_back(Tr(language, "Dismantle ", "拆解 ") + std::to_string(card.auxDestroyCount) +
                          Tr(language, " auxiliary ships for immediate rewards x", " 个附属舰并按即时收益倍率 x") +
                          std::to_string(card.auxDestroyRewardMultiplierPercent) + "%");
    }
    if (card.firstSummonExtraCopies > 0 && card.firstSummonExtraScope > 0) {
        const std::string scope = card.firstSummonExtraScope >= 2 ? Tr(language, "each turn", "每回合")
                                                                   : Tr(language, "once this battle", "本场战斗首次");
        clauses.push_back(Tr(language, "First summon protocol (", "首次召唤协议（") + scope + Tr(language, "): +", "）：额外 +") +
                          std::to_string(card.firstSummonExtraCopies) + Tr(language, " same-type copy", " 个同类型复制"));
    }
    if (card.summonCardPlayedDrawThisTurn > 0) {
        clauses.push_back(Tr(language, "This turn, whenever you play a summon card, draw ",
                             "本回合每打出 1 张召唤附属舰牌，抽 ") +
                          std::to_string(card.summonCardPlayedDrawThisTurn) + Tr(language, " card(s)", " 张牌"));
    }
    if (card.formChangesThisTurnThreshold > 0 && card.formChangesThisTurnBonusDamage > 0) {
        clauses.push_back(Tr(language, "If you changed form at least ", "若你本回合改变形态至少 ") +
                          std::to_string(card.formChangesThisTurnThreshold) +
                          Tr(language, " time(s), gain +", " 次，额外造成 +") +
                          std::to_string(card.formChangesThisTurnBonusDamage) + Tr(language, " damage", " 点伤害"));
    }
    if (card.firstEnterHighHeatEnergyThisBattle > 0 || card.firstEnterHighHeatDrawThisBattle > 0) {
        clauses.push_back(Tr(language, "First time entering high heat this battle: ", "本场战斗首次进入高热量：") +
                          Tr(language, "+", "+") + std::to_string(card.firstEnterHighHeatEnergyThisBattle) +
                          Tr(language, " energy, draw ", " 能量，抽 ") + std::to_string(card.firstEnterHighHeatDrawThisBattle) +
                          Tr(language, " card(s)", " 张牌"));
    }
    if (card.eachEnterHighHeatDrawThisBattle > 0) {
        clauses.push_back(Tr(language, "Each later high-heat entry this battle draws ",
                             "本场战斗后续每次进入高热量，抽 ") +
                          std::to_string(card.eachEnterHighHeatDrawThisBattle) + Tr(language, " card(s)", " 张牌"));
    }
    if (card.firstSummonEnemyHeatPerTurnThisBattle > 0) {
        clauses.push_back(Tr(language, "Each turn, your first summon adds enemy heat ",
                             "每回合你的首次召唤使敌方获得热量 ") +
                          std::to_string(card.firstSummonEnemyHeatPerTurnThisBattle) +
                          Tr(language, " (attack summon bonus +", "（攻击型召唤额外 +") +
                          std::to_string(card.firstSummonEnemyHeatAttackBonusPerTurnThisBattle) + ")");
    }
    if (card.enterOverloadEnemyHeatThisBattle > 0 || card.leaveDefenseEnemyHeatThisBattle > 0) {
        clauses.push_back(Tr(language, "This battle: entering Overload adds enemy heat ",
                             "本场战斗：进入过载使敌方热量 +") +
                          std::to_string(card.enterOverloadEnemyHeatThisBattle) +
                          Tr(language, ", leaving Defense adds ", "；离开防御使敌方热量 +") +
                          std::to_string(card.leaveDefenseEnemyHeatThisBattle));
    }
    if (card.formShift != 0) {
        std::string target = card.formShift == 1 ? Tr(language, "Overload", "过载形态")
                             : card.formShift == 2 ? Tr(language, "Defense", "防御形态")
                                                   : Tr(language, "Neutral", "中立形态");
        clauses.push_back(Tr(language, "Shift to ", "切换到 ") + target);
    }
    if (card.formExit) {
        clauses.push_back(Tr(language, "Exit current form", "脱离当前形态"));
    }
    if (card.formToggle) {
        clauses.push_back(Tr(language, "Toggle between Overload and Defense forms",
                             "在过载形态与防御形态之间切换"));
    }
    if (card.overloadIncomingDamageReducePercent > 0) {
        clauses.push_back(Tr(language, "While in Overload this turn, incoming damage modifier -",
                             "本回合处于过载时，受伤修正降低 ") +
                          std::to_string(card.overloadIncomingDamageReducePercent) + "%");
    }
    if (card.blackboxProtocol) {
        clauses.push_back(Tr(language,
                             "Exile 1 card from hand. At the start of each turn, add temporary copies of that card to hand. Copies vanish when leaving hand",
                             "放逐手牌中的 1 张牌。每回合开始将其临时复制加入手牌，复制牌离手即消失"));
    }
    if (card.blackboxCopyCount > 0) {
        clauses.push_back(Tr(language, "Temporary copies each turn: ", "每回合临时复制数量：") +
                          std::to_string(card.blackboxCopyCount));
    }
    if (card.leaveDefenseEnergyBonus > 0 || card.leaveDefenseShieldBonus > 0) {
        clauses.push_back(Tr(language, "When leaving Defense, gain ", "离开防御形态时，获得 ") +
                          Tr(language, "next-turn energy ", "下回合能量 ") + std::to_string(card.leaveDefenseEnergyBonus) +
                          Tr(language, " and shield ", " 与护盾 ") + std::to_string(card.leaveDefenseShieldBonus));
    }
    if (card.randomEffect) clauses.push_back(Tr(language, "Trigger one strong but unpredictable effect", "触发一个强力但不可预测的效果"));

    const std::string generated = JoinClauses(language, clauses);
    return generated.empty() ? TranslateGameText(language, card.description) : generated;
}

std::vector<std::string> CardDescriptionLines(UiLanguage language, const CardDef& card) {
    return SplitDisplayTextByPunctuation(CardDescriptionText(language, card));
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
    if (card->type == "Ability") {
        return Color::RGB(186, 143, 255);
    }
    if (card->type == "Control") {
        return kGold;
    }
    if (card->type == "Risk") {
        return Color::RGB(255, 152, 82);
    }
    return kHull;
}

Color CardFrameColor(const CardDef* card) {
    if (card == nullptr) {
        return Color::RGB(214, 222, 232);
    }
    switch (card->rarity) {
    case CardRarity::Common:
        return Color::RGB(232, 238, 247);
    case CardRarity::Uncommon:
        return Color::RGB(92, 163, 255);
    case CardRarity::Rare:
        return Color::RGB(246, 196, 91);
    }
    return Color::RGB(232, 238, 247);
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
    if (card->type == "Ability") {
        return Color::RGB(34, 20, 55);
    }
    if (card->type == "Control") {
        return Color::RGB(49, 37, 15);
    }
    if (card->type == "Risk") {
        return Color::RGB(54, 29, 12);
    }
    return Color::RGB(13, 34, 49);
}

Element ServiceDeckCardPanel(UiLanguage language,
                             const CardOwnershipEntry& entry,
                             bool selectable,
                             int hotkey,
                             bool direct_select_mode,
                             bool focused_target,
                             bool purge_mode,
                             int card_width,
                             const CardDef* upgrade_preview) {
    // The service grid mirrors the whole deck, but only cards enabled by the
    // current service context should look actionable.
    const CardDef& card = entry.card.def;
    std::vector<std::string> facts = CardFacts(language, card);
    facts.insert(facts.begin(), Tr(language, "Copies x", "持有数量 x") + std::to_string(std::max(1, entry.copies)));
    const std::string fact_text = CompactFactText(language, facts);
    const int text_width = std::max(12, card_width - 4);

    const Color accent = focused_target ? Color::RGB(255, 245, 160)
                                        : (selectable ? (purge_mode ? kEnemy : kGold) : CardAccent(&card));
    const Color frame = focused_target ? Color::RGB(255, 245, 160) : (selectable ? accent : CardFrameColor(&card));
    const Color background = focused_target ? Color::RGB(62, 48, 18)
                                            : (selectable ? CardBackground(&card, true) : Color::RGB(10, 17, 31));
    const Color meta_color = selectable ? kHull : Color::RGB(150, 160, 178);
    const std::string status = focused_target
                                   ? (direct_select_mode
                                          ? (purge_mode ? Tr(language, "Focused target: click or press Enter to purge", "当前选中目标：点击或按回车以移除")
                                                        : Tr(language, "Focused target: click or press Enter to upgrade", "当前选中目标：点击或按回车以升级"))
                                          : (Tr(language, "Focused target: press ", "当前选中目标：按 ") + std::to_string(hotkey) +
                                             (purge_mode ? Tr(language, " to purge", " 以移除") : Tr(language, " to upgrade", " 以升级"))))
                                   : selectable
                                         ? (direct_select_mode
                                                ? (purge_mode ? Tr(language, "Target: hover/click to purge", "目标：悬停/点击以移除")
                                                              : Tr(language, "Target: hover/click to upgrade", "目标：悬停/点击以升级"))
                                                : (Tr(language, "Target: press ", "目标：按 ") + std::to_string(hotkey) +
                                                   (purge_mode ? Tr(language, " to purge", " 以移除") : Tr(language, " to upgrade", " 以升级"))))
                                   : (purge_mode ? Tr(language, "Status: unavailable", "状态：不可移除")
                                                 : Tr(language, "Status: unavailable", "状态：不可升级"));

    Elements body = {
        text(TranslateGameText(language, card.name) + Tr(language, " x", " x") + std::to_string(std::max(1, entry.copies))) |
            color(accent) | bold,
        text(status) | color(focused_target ? Color::RGB(255, 245, 160) : (selectable ? kGold : kDim)),
        separator(),
        text(Tr(language, "Cost ", "费用 ") + std::to_string(card.cost) + "   " +
             Tr(language, "Type: ", "类型：") + TranslateGameText(language, card.type)) |
            color(meta_color),
        separator(),
        WrappedParagraph(CardDescriptionText(language, card), text_width) | color(Color::White),
        WrappedParagraph(fact_text.empty() ? Tr(language, "No additional card facts.", "没有额外卡牌信息。")
                                           : fact_text,
                         text_width) |
            color(selectable ? kDim : Color::RGB(117, 126, 142)),
    };

    if (upgrade_preview != nullptr) {
        body.push_back(separator());
        body.push_back(text(Tr(language, "Upgrade after:", "升级后效果：")) | color(Color::RGB(255, 210, 120)) | bold);
        body.push_back(WrappedParagraph(CardDescriptionText(language, *upgrade_preview), text_width) | color(Color::White));
        const std::string current_facts = CompactFactText(language, CardFacts(language, card));
        const std::string upgraded_facts = CompactFactText(language, CardFacts(language, *upgrade_preview));
        if (upgraded_facts != current_facts && !upgraded_facts.empty()) {
            body.push_back(WrappedParagraph(upgraded_facts, text_width) | color(kGold));
        }
    }

    return vbox(std::move(body)) |
           bgcolor(background) |
           borderStyled(ROUNDED, frame) |
           size(WIDTH, EQUAL, card_width) |
           size(HEIGHT, GREATER_THAN, 10);
}

std::vector<std::string> CardFacts(UiLanguage language, const CardDef& card) {
    std::vector<std::string> facts;
    const bool ability_card = card.type == "Ability";
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
    if (!ability_card && card.shield > 0) facts.push_back(Tr(language, "Shield +", "护盾 +") + std::to_string(card.shield));
    if (card.heal > 0) facts.push_back(Tr(language, "Heal +", "治疗 +") + std::to_string(card.heal));
    if (!ability_card && card.draw > 0) facts.push_back(Tr(language, "Draw +", "抽牌 +") + std::to_string(card.draw));
    if (!ability_card && card.energy > 0) facts.push_back(Tr(language, "Energy +", "能量 +") + std::to_string(card.energy));
    if (card.nextTurnEnergy > 0) facts.push_back(Tr(language, "Next turn +", "下回合 +") + std::to_string(card.nextTurnEnergy) + Tr(language, " energy", " 能量"));
    if (card.heat > 0) facts.push_back(Tr(language, "Heat +", "热量 +") + std::to_string(card.heat));
    else if (card.heat < 0) facts.push_back(Tr(language, "Heat ", "热量 ") + std::to_string(card.heat));
    if (card.enemyHeat > 0) facts.push_back(Tr(language, "Enemy heat +", "敌方热量 +") + std::to_string(card.enemyHeat));
    else if (card.enemyHeat < 0) facts.push_back(Tr(language, "Enemy heat ", "敌方热量 ") + std::to_string(card.enemyHeat));
    if (card.selfDamage > 0) facts.push_back(Tr(language, "Self dmg ", "自伤 ") + std::to_string(card.selfDamage));
    if (card.ignoreShield) facts.push_back(Tr(language, "Ignores shield", "无视护盾"));
    if (card.weakenTurns > 0) facts.push_back(Tr(language, "Weaken x", "削弱 x") + std::to_string(card.weakenTurns));
    if (card.shieldLockTurns > 0) facts.push_back(Tr(language, "Jam shield x", "护盾封锁 x") + std::to_string(card.shieldLockTurns));
    if (card.shieldToDamage) facts.push_back(Tr(language, "Convert own shield", "转化自身护盾"));
    if (card.attackMultiplier > 1) facts.push_back(Tr(language, "Next attack x", "下张攻击 x") + std::to_string(card.attackMultiplier));
    if (card.halveHeat) facts.push_back(Tr(language, "Halve heat", "热量减半"));
    if (card.clearHeat) facts.push_back(Tr(language, "Clear heat", "清空热量"));
    if (card.salvageBonus > 0) facts.push_back(Tr(language, "Battle credits +", "战斗金币 +") + std::to_string(card.salvageBonus));
    if (card.summonDroneCount > 0) facts.push_back(Tr(language, "Summon drones x", "部署附属舰 x") + std::to_string(card.summonDroneCount));
    if (card.summonDroneCount > 0) {
        const bool repair_drone = card.name.find("维修") != std::string::npos ||
                                  card.nameEn.find("Repair") != std::string::npos;
        const bool burner_drone = card.id == "dfh_01";
        const bool thermal_escort = card.id == "dfh_08";
        const bool queen_wasp = card.id == "df_27";
        const bool special_aux_end_turn_payload = thermal_escort || queen_wasp;
        if (!burner_drone && card.summonDroneAttack > 0) {
            facts.push_back(Tr(language, "Drone ATK ", "附属舰攻击 ") + std::to_string(card.summonDroneAttack));
        }
        if (repair_drone) {
            if (card.summonDroneShield > 0) {
                facts.push_back(Tr(language, "Drone REP ", "附属舰修复 ") + std::to_string(card.summonDroneShield));
            }
        } else if (card.summonDroneShield > 0) {
            facts.push_back(Tr(language, "Drone SHD ", "附属舰护盾 ") + std::to_string(card.summonDroneShield));
        }
        if (queen_wasp && card.damage > 0) {
            facts.push_back(Tr(language, "Drone EOT dmg ", "附属舰回合末伤害 ") + std::to_string(card.damage));
        }
        if ((burner_drone || thermal_escort) && card.enemyHeat > 0) {
            facts.push_back(Tr(language, "Drone EOT heat +", "附属舰回合末敌方热量 +") + std::to_string(card.enemyHeat));
        }
        if (special_aux_end_turn_payload && card.auxEndTurnExtraTriggerTimes > 0) {
            facts.push_back(Tr(language, "Other aux EOT x", "其他附属舰在回合末额外触发 x") +
                            std::to_string(card.auxEndTurnExtraTriggerTimes));
        }
    }
    if (card.summonDroneDuration > 0) facts.push_back(Tr(language, "Drone duration ", "附属舰持续 ") + std::to_string(card.summonDroneDuration));
    if (card.buffDroneAttack != 0 || card.buffDroneShield != 0) {
        facts.push_back(Tr(language, "Drone buff A/S ", "附属舰增幅 ") + std::to_string(card.buffDroneAttack) +
                        "/" + std::to_string(card.buffDroneShield));
    }
    if (card.auxImmediateTriggerTimes > 0) {
        facts.push_back(Tr(language, "Aux immediate x", "附属舰立即触发 x") + std::to_string(card.auxImmediateTriggerTimes));
    }
    if (card.auxEndTurnExtraTriggerTimes > 0 && card.id != "df_27" && card.id != "dfh_08") {
        facts.push_back(Tr(language, "Aux end-turn +x", "附属舰回合末 +x") + std::to_string(card.auxEndTurnExtraTriggerTimes));
    }
    if (card.auxEffectPercentBonus > 0) {
        facts.push_back(Tr(language, "Aux effect +", "附属舰效果 +") + std::to_string(card.auxEffectPercentBonus) + "%");
    }
    if (card.auxDestroyCount > 0) {
        facts.push_back(Tr(language, "Dismantle x", "拆解数量 x") + std::to_string(card.auxDestroyCount));
        facts.push_back(Tr(language, "Destroy reward ", "拆解收益倍率 ") + std::to_string(card.auxDestroyRewardMultiplierPercent) + "%");
    }
    if (card.firstSummonExtraCopies > 0 && card.firstSummonExtraScope > 0) {
        const std::string scope = card.firstSummonExtraScope >= 2 ? Tr(language, "turn", "回合")
                                                                   : Tr(language, "battle", "战斗");
        facts.push_back(Tr(language, "First summon +", "首次召唤 +") + std::to_string(card.firstSummonExtraCopies) +
                        Tr(language, " (scope ", "（范围 ") + scope + ")");
    }
    if (card.summonCardPlayedDrawThisTurn > 0) {
        facts.push_back(Tr(language, "Summon draw/turn +", "召唤抽牌/回合 +") + std::to_string(card.summonCardPlayedDrawThisTurn));
    }
    if (card.formChangesThisTurnThreshold > 0 && card.formChangesThisTurnBonusDamage > 0) {
        facts.push_back(Tr(language, "Form changes >= ", "形态变化 >= ") + std::to_string(card.formChangesThisTurnThreshold) +
                        Tr(language, " then DMG +", " 时伤害 +") + std::to_string(card.formChangesThisTurnBonusDamage));
    }
    if (ability_card && card.id == "df_29") {
        facts.push_back(Tr(language, "On aux removal D/E ", "附属舰离场 抽牌/能量 ") +
                        std::to_string(card.draw) + "/" + std::to_string(card.energy));
    }
    if (ability_card && card.id == "ib_28") {
        facts.push_back(Tr(language, "Enter/exit Defense S/E ", "进入/脱离防御 护盾/能量 ") +
                        std::to_string(card.shield) + "/" + std::to_string(card.energy));
    }
    if (ability_card && card.id == "ib_29") {
        facts.push_back(Tr(language, "First Overload/Defense D/E ", "首次过载/防御离场 抽牌/能量 ") +
                        std::to_string(card.draw) + "/" + std::to_string(card.energy));
    }
    if (card.firstEnterHighHeatEnergyThisBattle > 0 || card.firstEnterHighHeatDrawThisBattle > 0) {
        facts.push_back(Tr(language, "First high-heat E/D ", "首入高热量 能量/抽牌 ") +
                        std::to_string(card.firstEnterHighHeatEnergyThisBattle) + "/" +
                        std::to_string(card.firstEnterHighHeatDrawThisBattle));
    }
    if (card.eachEnterHighHeatDrawThisBattle > 0) {
        facts.push_back(Tr(language, "Each high-heat draw ", "每次高热量抽牌 ") +
                        std::to_string(card.eachEnterHighHeatDrawThisBattle));
    }
    if (card.firstSummonEnemyHeatPerTurnThisBattle > 0) {
        facts.push_back(Tr(language, "First summon enemy heat ", "首次召唤敌热 ") +
                        std::to_string(card.firstSummonEnemyHeatPerTurnThisBattle));
        if (card.firstSummonEnemyHeatAttackBonusPerTurnThisBattle > 0) {
            facts.push_back(Tr(language, "Atk summon bonus +", "攻击召唤加成 +") +
                            std::to_string(card.firstSummonEnemyHeatAttackBonusPerTurnThisBattle));
        }
    }
    if (card.enterOverloadEnemyHeatThisBattle > 0 || card.leaveDefenseEnemyHeatThisBattle > 0) {
        facts.push_back(Tr(language, "Overload/Defense enemy heat ", "过载入场/防御离场敌热 ") +
                        std::to_string(card.enterOverloadEnemyHeatThisBattle) + "/" +
                        std::to_string(card.leaveDefenseEnemyHeatThisBattle));
    }
    if (card.formShift != 0) {
        const std::string target = card.formShift == 1 ? Tr(language, "Overload", "过载")
                                    : card.formShift == 2 ? Tr(language, "Defense", "防御")
                                                          : Tr(language, "Neutral", "中立");
        facts.push_back(Tr(language, "Form -> ", "形态 -> ") + target);
    }
    if (card.formExit) facts.push_back(Tr(language, "Exit current form", "脱离当前形态"));
    if (card.formToggle) facts.push_back(Tr(language, "Form toggle", "形态切换"));
    if (card.overloadIncomingDamageReducePercent > 0) {
        facts.push_back(Tr(language, "Overload dmg mod -", "过载受伤修正 -") +
                        std::to_string(card.overloadIncomingDamageReducePercent) + "%");
    }
    if (card.blackboxProtocol) facts.push_back(Tr(language, "Blackbox protocol", "黑箱协议"));
    if (card.blackboxCopyCount > 0) {
        facts.push_back(Tr(language, "Temp copies/turn ", "每回合临时复制 ") + std::to_string(card.blackboxCopyCount));
    }
    if (card.leaveDefenseEnergyBonus > 0 || card.leaveDefenseShieldBonus > 0) {
        facts.push_back(Tr(language, "Exit Defense E/S ", "离开防御奖励 能量/护盾 ") +
                        std::to_string(card.leaveDefenseEnergyBonus) + "/" +
                        std::to_string(card.leaveDefenseShieldBonus));
    }
    if (card.randomEffect) facts.push_back(Tr(language, "Volatile effect", "不稳定效果"));
    return facts;
}

std::vector<std::string> PlayerStatusLines(UiLanguage language, const RunState& state) {
    const bool has_origin = std::find(state.player.relics.begin(), state.player.relics.end(), "origin") !=
                            state.player.relics.end();
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
    const std::string form_name = state.player.form == 1 ? Tr(language, "Overload", "过载")
                                 : state.player.form == 2 ? Tr(language, "Defense", "防御")
                                                          : Tr(language, "Neutral", "中立");
    lines.push_back(Tr(language, "Combat form: ", "战斗形态：") + form_name);
    lines.push_back(Tr(language, "Auxiliary ships: ", "附属舰数量：") + std::to_string(state.auxiliaryShips.size()));
    if (has_origin) {
        lines.push_back(Tr(language,
                           "Origin: +2 energy each turn, deal +25% damage, take 25% less damage.",
                           "起源：每回合 +2 能量，造成伤害 +25%，受到伤害 -25%。"));
    }
    return lines;
}

std::vector<std::string> BattlePlayerSummaryLines(UiLanguage language, const RunState& state) {
    const bool has_origin = std::find(state.player.relics.begin(), state.player.relics.end(), "origin") !=
                            state.player.relics.end();
    std::vector<std::string> lines = {
        Tr(language, "Energy ", "能量 ") + EnergyPips(state.player.energy, state.player.maxEnergy).substr(7),
        Tr(language, "Shield ", "护盾 ") + std::to_string(state.player.shield) + "   " + Tr(language, "Heat ", "热量 ") +
            std::to_string(state.player.heat) + "/" + std::to_string(state.player.maxHeat),
        Tr(language, "Hand ", "手牌 ") + std::to_string(state.player.hand.size()) + "   " +
            Tr(language, "Draw ", "抽牌堆 ") + std::to_string(state.player.drawPile.size()) + "   " +
            Tr(language, "Discard ", "弃牌堆 ") + std::to_string(state.player.discardPile.size()),
        Tr(language, "Credits ", "金币 ") + std::to_string(state.player.scrap),
    };
    const int player_overheat_stage =
        state.player.heat < state.player.maxHeat ? 0 : std::max(1, std::min(3, state.player.overheatRounds));
    if (player_overheat_stage >= 1) {
        lines.push_back(Tr(language, "Signal Fray I: Draw -1", "信号紊流 I：抽牌 -1"));
    }
    if (player_overheat_stage >= 2) {
        lines.push_back(Tr(language, "Weapon Bus Sag II: Outgoing -25%, Incoming +25%",
                           "武器母线压降 II：输出伤害 -25%，受到伤害 +25%"));
    }
    if (player_overheat_stage >= 3) {
        lines.push_back(Tr(language, "Core Bleed III: Lose 5 HP each turn", "核心失血 III：每回合失去 5 点生命"));
    }
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
    const std::string form_name = state.player.form == 1 ? Tr(language, "Overload", "过载")
                                  : state.player.form == 2 ? Tr(language, "Defense", "防御")
                                                           : Tr(language, "Neutral", "中立");
    lines.push_back(Tr(language, "Combat form: ", "战斗形态：") + form_name);
    if (state.player.form == 1) {
        lines.push_back(Tr(language, "Overload effect: deal and take double damage", "过载效果：造成与受到伤害均翻倍"));
    } else if (state.player.form == 2) {
        lines.push_back(Tr(language, "Defense release bonus is defined by cards when exiting the form",
                           "防御形态离开奖励由具体切换卡决定"));
    }
    if (has_origin) {
        lines.push_back(Tr(language,
                           "Origin: +2 energy each turn, deal +25% damage, take 25% less damage.",
                           "起源：每回合 +2 能量，造成伤害 +25%，受到伤害 -25%。"));
    }
    if (!state.player.activeAbilities.empty()) {
        lines.push_back(Tr(language, "Active abilities:", "已启动能力："));
        for (const ActiveAbilityState& ability : state.player.activeAbilities) {
            const std::string name =
                language == UiLanguage::English && !ability.nameEn.empty() ? ability.nameEn : ability.name;
            const std::string description = language == UiLanguage::English && !ability.descriptionEn.empty()
                                                ? ability.descriptionEn
                                                : ability.description;
            lines.push_back(name + ": " + description);
        }
    }
    lines.push_back(Tr(language, "Active auxiliary ships: ", "在场附属舰：") + std::to_string(state.auxiliaryShips.size()));
    return lines;
}

std::vector<std::string> BattleEnemySummaryLines(UiLanguage language, const RunState& state) {
    std::vector<std::string> lines;
    lines.push_back(Tr(language, "Intent: ", "意图：") + IntentSummary(language, state));
    if (!state.enemy.secondaryIntentKind.empty()) {
        lines.push_back(Tr(language, "Second intent: ", "第二意图：") + IntentSummary(language, state, true));
    }
    lines.push_back(Tr(language, "Shield ", "护盾 ") + std::to_string(state.enemy.shield) + "   " +
                    Tr(language, "Heat ", "热量 ") + std::to_string(state.enemy.heat) + "/" + std::to_string(state.enemy.maxHeat));
    if (state.enemy.heat >= state.enemy.maxHeat) {
        const int stage = std::max(1, std::min(3, state.enemy.overheatRounds));
        if (stage == 1) {
            lines.push_back(Tr(language, "Reactor Strain I: Outgoing damage -25%", "反应堆应力 I：输出伤害 -25%"));
        } else if (stage == 2) {
            lines.push_back(Tr(language, "Containment Fracture II: Outgoing -25%, Incoming +25%", "约束层裂解 II：输出 -25%，受到伤害 +25%"));
        } else {
            lines.push_back(Tr(language, "Meltdown Cascade III: Outgoing -25%, Incoming +25%, Burn 5/turn", "熔毁级联 III：输出 -25%，受到伤害 +25%，每回合燃烧 5 点生命"));
        }
    }
    if (state.enemy.coolingCooldownTurns > 0) {
        lines.push_back(Tr(language, "Cooling cooldown: ", "冷却冷却中：") + std::to_string(state.enemy.coolingCooldownTurns));
    }
    if (state.enemy.nextAttackBonusDamage > 0) {
        lines.push_back(Tr(language, "Charged attack bonus: +", "蓄力增伤：+") + std::to_string(state.enemy.nextAttackBonusDamage));
    }
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
