#include "ui/star_abyss_app.h"

#include <string>

#include "ui/ui_panels.h"

using namespace ftxui;

namespace {

std::string DifficultyDescription(UiLanguage language, int difficulty_index) {
    switch (difficulty_index) {
    case 0:
        return Tr(language,
                  "Easy: Current baseline difficulty with the existing forgiving balance.",
                  "简单：保持当前版本的默认平衡，整体最宽松。");
    case 1:
        return Tr(language,
                  "Normal: Enemies are more attack-oriented. Elite and boss enemies have a 30% chance each round to prepare and resolve a second intent.",
                  "普通：敌人更偏向攻击。精英和 Boss 每回合有 30% 概率额外准备并结算一个第二意图。");
    case 2:
        return Tr(language,
                  "Hard: Based on Normal. Enemy HP, max heat, and intent values are increased by 30%. Elite and boss enemies have a 40% chance each round to prepare and resolve a second intent.",
                  "困难：基于普通。敌人的生命、最大热量与意图数值提高 30%。精英和 Boss 每回合有 40% 概率额外准备并结算一个第二意图。");
    default:
        return Tr(language,
                  "Hell: Enemy numbers match Hard, but all enemy intents are hidden as Unknown. Elite and boss enemies have a 50% chance each round to prepare and resolve a second intent.",
                  "地狱：敌人数值与困难相同，但所有敌方意图都会显示为“未知”。精英和 Boss 每回合有 50% 概率额外准备并结算一个第二意图。");
    }
}

std::string ShipDescription(UiLanguage language, int ship_index) {
    switch (ship_index) {
    case 0:
        return Tr(language,
                  "Vanguard: Balanced reactor fighter. Strong heat-based offense with steady cooling tools and direct pressure.",
                  "先锋号：均衡型反应堆战舰。擅长围绕热量展开进攻，兼具稳定散热和直接输出。");
    case 1:
        return Tr(language,
                  "Drift Fox: Auxiliary-ship specialist. Builds tempo through drones, trigger chaining, and flexible support patterns.",
                  "漂移狐：附属舰专精。依靠无人机、连锁触发和灵活支援逐步滚起节奏。");
    default:
        return Tr(language,
                  "Iron Bastion: Durable form-shifting hull. Excels at stance switching, defensive conversion, and heavy late bursts.",
                  "钢铁堡垒：高耐久形态切换舰体。擅长姿态转换、防御转化，以及后段重击爆发。");
    }
}

}  // namespace

// Launch setup remains small, but isolating it now avoids mixing configuration
// widgets with the much denser battle/map rendering logic.
Element StarAbyssApp::RenderSetup() const {
    const std::string difficulty_description = DifficultyDescription(language_, difficultyIndex_);
    const std::string ship_description = ShipDescription(language_, shipIndex_);
    Element content = vbox({
        text(Tr(language_, "Launch Setup", "出航设置")) | bold | color(kGold) | center,
        text(Tr(language_, "Pick difficulty and hull profile before diving in.", "在跃入深渊前，先选择难度与舰体。")) | color(kHull) | center,
        separatorDouble(),
        window(text(Tr(language_, "Difficulty", "难度")) | color(kHull), difficultyToggle_->Render()) | bgcolor(kPanel),
        window(text(Tr(language_, "Starter Ship", "初始飞船")) | color(kGold), shipToggle_->Render()) | bgcolor(kPanel),
        separator(),
        window(text(Tr(language_, "Difficulty Notes", "难度说明")) | color(kHull),
               paragraph(difficulty_description) | color(Color::White)) | bgcolor(kPanelAlt),
        window(text(Tr(language_, "Ship Notes", "飞船说明")) | color(kGold),
               paragraph(ship_description) | color(Color::White)) | bgcolor(kPanelAlt),
        separator(),
        hbox({startRunButton_->Render() | flex, setupBackButton_->Render() | flex}),
    });

    return ApplyVerticalScrollFrame(content, pageScrollLines_[PageIndex(AppPage::Setup)]) | borderDouble | bgcolor(kAbyss);
}
