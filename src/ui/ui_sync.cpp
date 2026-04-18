#include "ui/star_abyss_app.h"

#include "ui/ui_localization.h"

void StarAbyssApp::SyncUi() {
    const RunState& state = engine_.state();
    const int previous_page = activePage_;
    activePage_ = static_cast<int>(state.page);
    if (previous_page != activePage_) {
        pageScrollLines_[PageIndex(state.page)] = 0;
    }
    difficultyEntries_[0] = Tr(language_, "Easy", "简单");
    difficultyEntries_[1] = Tr(language_, "Normal", "普通");
    difficultyEntries_[2] = Tr(language_, "Hard", "困难");
    shipEntries_[0] = TranslateGameText(language_, "Vanguard");
    shipEntries_[1] = TranslateGameText(language_, "Drift Fox");
    shipEntries_[2] = TranslateGameText(language_, "Iron Bastion");
    startRunLabel_ = Tr(language_, "Launch Run", "开始出航");
    setupBackLabel_ = Tr(language_, "Back", "返回");
    mapSaveLabel_ = Tr(language_, "Save Run", "保存航程");
    mapMenuLabel_ = Tr(language_, "Main Menu", "主菜单");
    encounterSaveLabel_ = Tr(language_, "Save Run", "保存航程");
    encounterMenuLabel_ = Tr(language_, "Main Menu", "主菜单");
    battleEndTurnLabel_ = Tr(language_, "End Turn", "结束回合");
    battleSaveLabel_ = Tr(language_, "Save Run", "保存航程");
    battleMenuLabel_ = Tr(language_, "Main Menu", "主菜单");
    rewardSkipLabel_ = Tr(language_, "Skip Card Reward", "跳过卡牌奖励");
    rewardMenuLabel_ = Tr(language_, "Main Menu", "主菜单");
    infoBackLabel_ = Tr(language_, "Back", "返回");
    summaryMenuLabel_ = Tr(language_, "Main Menu", "主菜单");
    summaryNewRunLabel_ = Tr(language_, "New Run", "新航程");
    summaryExitLabel_ = Tr(language_, "Exit", "退出");
    saveConfirmLabel_ = Tr(language_, "Save", "保存");
    loadConfirmLabel_ = Tr(language_, "Load", "读取");
    autoSaveLabel_ = Tr(language_, "Auto Save", "自动存档");
    closeDialogLabel_ = Tr(language_, "Close", "关闭");

    menuLabels_[0] = state.hasRun ? Tr(language_, "Resume Current Run", "继续当前航程")
                                  : Tr(language_, "Resume Current Run (Unavailable)", "继续当前航程（不可用）");
    menuLabels_[1] = Tr(language_, "Start New Run", "开始新航程");
    menuLabels_[2] = Tr(language_, "Load Save", "读取存档");
    menuLabels_[3] = Tr(language_, "Leaderboard", "排行榜");
    menuLabels_[4] = Tr(language_, "Help", "帮助");
    menuLabels_[5] = Tr(language_, "Language: English", "语言：中文");
    menuLabels_[6] = Tr(language_, "Exit", "退出");

    for (std::string& label : mapLabels_) label = TranslateGameText(language_, "Unavailable");
    for (std::string& label : encounterLabels_) label = TranslateGameText(language_, "Unavailable");
    for (std::string& label : battleCardLabels_) label = TranslateGameText(language_, "Empty Slot");
    for (std::string& label : rewardLabels_) label = TranslateGameText(language_, "Unavailable");

    for (size_t i = 0; i < state.routeOptions.size() && i < mapLabels_.size(); ++i) {
        mapLabels_[i] = TranslateGameText(language_, state.routeOptions[i].name);
    }
    for (size_t i = 0; i < state.encounter.choices.size() && i < encounterLabels_.size(); ++i) {
        encounterLabels_[i] = TranslateGameText(language_, state.encounter.choices[i]);
    }
    const std::vector<CardView> hand = engine_.handCards();
    for (size_t i = 0; i < hand.size() && i < battleCardLabels_.size(); ++i) {
        battleCardLabels_[i] = TranslateGameText(language_, hand[i].def.name);
    }
    const std::vector<CardView> rewards = engine_.rewardCards();
    for (size_t i = 0; i < rewards.size() && i < rewardLabels_.size(); ++i) {
        rewardLabels_[i] = TranslateGameText(language_, rewards[i].def.name);
    }
    if (saveDialogVisible_) {
        RefreshSaveEntries();
    }
}
