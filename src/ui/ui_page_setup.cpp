#include "ui/star_abyss_app.h"

#include <string>

#include "ui/ui_panels.h"

#include <ftxui/screen/terminal.hpp>

using namespace ftxui;

namespace {

std::string DifficultyDescription(UiLanguage language, int difficulty_index) {
    switch (difficulty_index) {
    case 0:
        return Tr(language,
                  "Easy: Current baseline difficulty with the existing forgiving balance.",
                  u8"简单：保持当前版本的默认平衡，整体最宽松。");
    case 1:
        return Tr(language,
                  "Normal: Baseline enemy behavior. Elite and boss enemies have a 30% chance each round to prepare and resolve a second intent.",
                  u8"普通：敌人保持基础攻击倾向。精英和 Boss 每回合有 30% 概率额外准备并结算一个第二意图。");
    case 2:
        return Tr(language,
                  "Hard: Enemy HP, max heat, and intent values are increased by 30%. Enemy attack bias is increased. Elite and boss enemies have a 40% chance each round to prepare and resolve a second intent.",
                  u8"困难：敌人的生命、最大热量与意图数值提高 30%。攻击欲望提升。精英和 Boss 每回合有 40% 概率额外准备并结算一个第二意图。");
    default:
        return Tr(language,
                  "Hell: Enemy numbers match Hard. Enemy attack bias matches Hard. All enemy intents are hidden as Unknown. Elite and boss enemies have a 50% chance each round to prepare and resolve a second intent.",
                  u8"地狱：敌人数值与困难相同。攻击欲望与困难保持相同。所有敌方意图都会显示为“未知”。精英和 Boss 每回合有 50% 概率额外准备并结算一个第二意图。");
    }
}

std::string ShipDescription(UiLanguage language, int ship_index) {
    switch (ship_index) {
    case 0:
        return Tr(language,
                  "Vanguard: A reactor assault hull defined by direct-fire weapons, sharp heat spikes, and built-in cooling hardware. Its signature is straightforward pressure, fast thermal buildup, and stable vent control.",
                  "先锋号：反应堆突击舰体，以直射火力、快速升温和内置散热模块为主要特征。它的标志是正面压制、热量峰值明显，以及稳定的控温能力。");
    case 1:
        return Tr(language,
                  "Drift Fox: A light command carrier built around multiple auxiliary bays, rapid launch systems, and synchronized drone relays. Its defining traits are layered support craft, trigger-linked formations, and strong battlefield utility.",
                  "漂移狐：轻型指挥母舰，核心特征是多附属舰机库、快速投放系统和同步无人机链路。它的辨识度在于层叠式支援编队、联动触发结构，以及很强的战场功能性。");
    default:
        return Tr(language,
                  "Iron Bastion: A heavy fortress hull with two combat forms. Defense Form has no universal damage bonus, but it serves as the stable armored state that enables many shield, draw, energy, and exit-defense effects; Overload Form is the redline state that doubles outgoing damage and also doubles incoming damage. Its defining traits are extreme durability, deliberate form switching, and dangerous overload output.",
                  "钢铁堡垒：重型堡垒舰体，拥有两种战斗形态。防御形态没有通用伤害加成，但它是稳定的装甲姿态，会承载大量护盾、抽牌、回能和脱离防御效果；过载形态则是红线输出姿态，会让你的造成伤害翻倍，同时受到伤害也翻倍。它的核心特征是高耐久、明确的形态切换结构，以及高风险高输出的过载状态。");
    }
}

}  // namespace

// Launch setup remains small, but isolating it now avoids mixing configuration
// widgets with the much denser battle/map rendering logic.
Element StarAbyssApp::RenderSetup() const {
    const std::string difficulty_description = DifficultyDescription(language_, difficultyIndex_);
    const std::string ship_description = ShipDescription(language_, shipIndex_);
    const int terminal_width = Terminal::Size().dimx;
    const int note_width = std::max(26, terminal_width - 14);
    Element content = vbox({
        text(Tr(language_, "Launch Setup", "出航设置")) | bold | color(kGold) | center,
        text(Tr(language_, "Pick difficulty and hull profile before diving in.", "在跃入深渊前，先选择难度与舰体。")) | color(kHull) | center,
        separatorDouble(),
        window(text(Tr(language_, "Difficulty", "难度")) | color(kHull), difficultyToggle_->Render()) | bgcolor(kPanel),
        window(text(Tr(language_, "Starter Ship", "初始飞船")) | color(kGold), shipToggle_->Render()) | bgcolor(kPanel),
        separator(),
        window(text(Tr(language_, "Difficulty Notes", "难度说明")) | color(kHull),
               WrappedParagraph(difficulty_description, note_width) | color(Color::White)) | bgcolor(kPanelAlt),
        window(text(Tr(language_, "Ship Notes", "飞船说明")) | color(kGold),
               WrappedParagraph(ship_description, note_width) | color(Color::White)) | bgcolor(kPanelAlt),
        separator(),
        hbox({startRunButton_->Render() | flex, setupBackButton_->Render() | flex}),
    });

    return ApplyVerticalScrollFrame(content, pageScrollLines_[PageIndex(AppPage::Setup)]) | borderDouble | bgcolor(kAbyss);
}
