#include "ui/star_abyss_app.h"

#include "engine/game_dev.h"
#include "ui/ui_localization.h"

void StarAbyssApp::SyncUi() {
    if (engine_.state().page == AppPage::MainMenu &&
        activePage_ != static_cast<int>(AppPage::MainMenu) &&
        devModeUi_ != nullptr && devModeUi_->unlocked()) {
        devModeUi_->exitDeveloperMode();
        return;
    }

    const RunState& state = engine_.state();
    const int previous_page = activePage_;
    activePage_ = static_cast<int>(state.page);
    if (previous_page != activePage_) {
        pageScrollLines_[PageIndex(state.page)] = 0;
    }
    difficultyEntries_[0] = Tr(language_, "Easy", "简单");
    difficultyEntries_[1] = Tr(language_, "Normal", "普通");
    difficultyEntries_[2] = Tr(language_, "Hard", "困难");
    difficultyEntries_[3] = Tr(language_, "Hell", "地狱");
    shipEntries_[0] = TranslateGameText(language_, "Vanguard");
    shipEntries_[1] = TranslateGameText(language_, "Drift Fox");
    shipEntries_[2] = TranslateGameText(language_, "Iron Bastion");
    startRunLabel_ = Tr(language_, "Launch Run", "开始出航");
    setupBackLabel_ = Tr(language_, "Back", "返回");
    mapSaveLabel_ = Tr(language_, "Save Run", "保存航程");
    mapMenuLabel_ = Tr(language_, "Main Menu", "主菜单");
    encounterSaveLabel_ = Tr(language_, "Save Run", "保存航程");
    encounterMenuLabel_ = Tr(language_, "Main Menu", "主菜单");
    encounterOwnedCardsLabel_ = Tr(language_, "[Deck] Owned Cards", "[牌组] 已拥有卡牌");
    battleEndTurnLabel_ = Tr(language_, "End Turn", "结束回合");
    battleSaveLabel_ = Tr(language_, "Save Run", "保存航程");
    battleMenuLabel_ = Tr(language_, "Main Menu", "主菜单");
    battleOwnedCardsLabel_ = Tr(language_, "[Deck] Owned Cards", "[牌组] 已拥有卡牌");
    battleDrawPileLabel_ = Tr(language_, "[Deck] Draw Pile", "[牌库] 抽牌堆");
    battleDiscardPileLabel_ = Tr(language_, "[Scrap] Discard Pile", "[弃牌] 弃牌堆");
    rewardSkipLabel_ = Tr(language_, "Skip Card Reward", "跳过卡牌奖励");
    rewardMenuLabel_ = Tr(language_, "Main Menu", "主菜单");
    rewardOwnedCardsLabel_ = Tr(language_, "[Deck] Owned Cards", "[牌组] 已拥有卡牌");
    infoBackLabel_ = Tr(language_, "Back", "返回");
    summaryMenuLabel_ = Tr(language_, "Main Menu", "主菜单");
    summaryNewRunLabel_ = Tr(language_, "New Run", "新航程");
    summaryExitLabel_ = Tr(language_, "Exit", "退出");
    saveConfirmLabel_ = Tr(language_, "Save", "保存");
    loadConfirmLabel_ = Tr(language_, "Load", "读取");
    autoSaveLabel_ = Tr(language_, "Auto Save", "自动存档");
    closeDialogLabel_ = Tr(language_, "Close", "关闭");
    menuLibraryLabel_ = Tr(language_, "[Archive] Card Library", "[档案] 卡牌图鉴");
    mapOwnedCardsLabel_ = Tr(language_, "[Deck] Owned Cards", "[牌组] 已拥有卡牌");
    cardViewerCloseLabel_ = Tr(language_, "Close", "关闭");

    menuLabels_[0] = state.hasRun ? Tr(language_, "Resume Current Run", "继续当前航程")
                                  : Tr(language_, "Resume Current Run (Unavailable)", "继续当前航程（不可用）");
    menuLabels_[1] = Tr(language_, "Start New Run", "开始新航程");
    menuLabels_[2] = Tr(language_, "Load Save", "读取存档");
    menuLabels_[3] = Tr(language_, "Leaderboard", "排行榜");
    menuLabels_[4] = Tr(language_, "Achievements", "成就");
    menuLabels_[5] = Tr(language_, "Help", "帮助");
    menuLabels_[6] = Tr(language_, "Language: English", "语言：中文");
    menuLabels_[7] = Tr(language_, "Exit", "退出");
    menuDevLabel_ = Tr(language_, "Developer Mode", "开发者选项");
    menuLabels_[4] += " (" + std::to_string(engine_.completedAchievementCount()) + "/" +
                      std::to_string(engine_.totalEventAchievementCount() + engine_.totalEnemyCount()) + ")";

    for (std::string& label : mapLabels_) label = TranslateGameText(language_, "Unavailable");
    for (std::string& label : encounterLabels_) label = TranslateGameText(language_, "Unavailable");
    for (std::string& label : battleCardLabels_) label = TranslateGameText(language_, "Empty Slot");
    for (std::string& label : battleTransformLabels_) label = TranslateGameText(language_, "Unavailable");
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
    const std::vector<std::string> transform_targets = engine_.auxiliaryTransformTargetUnitIds();
    for (size_t i = 0; i < transform_targets.size() && i < battleTransformLabels_.size(); ++i) {
        if (transform_targets[i] == "aux_interceptor") {
            battleTransformLabels_[i] = TranslateGameText(language_, "Interceptor Drone");
        } else if (transform_targets[i] == "aux_guardian") {
            battleTransformLabels_[i] = TranslateGameText(language_, "Guardian Drone");
        } else if (transform_targets[i] == "aux_repair") {
            battleTransformLabels_[i] = TranslateGameText(language_, "Repair Drone");
        } else if (transform_targets[i] == "aux_pulse") {
            battleTransformLabels_[i] = TranslateGameText(language_, "Pulse Drone");
        }
    }
    if (state.page != AppPage::Battle) {
        battlePileInspector_ = BattlePileInspector::None;
    } else {
        battleDrawPileLabel_ += " (" + std::to_string(state.player.drawPile.size()) + ")";
        battleDiscardPileLabel_ += " (" + std::to_string(state.player.discardPile.size()) + ")";
    }
    const int owned_card_total = static_cast<int>(state.player.drawPile.size() + state.player.hand.size() + state.player.discardPile.size());
    mapOwnedCardsLabel_ += " (" + std::to_string(owned_card_total) + ")";
    encounterOwnedCardsLabel_ += " (" + std::to_string(owned_card_total) + ")";
    battleOwnedCardsLabel_ += " (" + std::to_string(owned_card_total) + ")";
    rewardOwnedCardsLabel_ += " (" + std::to_string(owned_card_total) + ")";
    menuLibraryLabel_ += " (" + std::to_string(engine_.obtainedCardCount()) + "/" + std::to_string(engine_.totalCardCount()) + ")";
    const bool owned_cards_page = state.page == AppPage::Map || state.page == AppPage::Encounter ||
                                  state.page == AppPage::Battle || state.page == AppPage::Reward;
    if ((cardViewerMode_ == CardViewerMode::OwnedCards && !owned_cards_page) ||
        ((cardViewerMode_ == CardViewerMode::CardLibrary || cardViewerMode_ == CardViewerMode::Achievements) &&
         state.page != AppPage::MainMenu)) {
        CloseCardViewer();
    }
    const std::vector<CardView> rewards = engine_.rewardCards();
    for (size_t i = 0; i < rewards.size() && i < rewardLabels_.size(); ++i) {
        rewardLabels_[i] = TranslateGameText(language_, rewards[i].def.name);
    }
    if (saveDialogVisible_) {
        RefreshSaveEntries();
    }
}
