#include "tui_app.h"

#include <array>
#include <chrono>
#include <thread>
#include <utility>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>

#include "ui/star_abyss_app.h"
#include "ui/ui_panels.h"

using namespace ftxui;

StarAbyssApp::StarAbyssApp(ScreenInteractive& screen) : screen_(screen) {
    menuButtons_[0] = Button(&menuLabels_[0], [&] { engine_.resumeRun(); SyncUi(); }, AccentButton(kHull));
    menuButtons_[1] = Button(&menuLabels_[1], [&] { engine_.startAnotherRun(); SyncUi(); }, AccentButton(kGold));
    menuButtons_[2] = Button(&menuLabels_[2], [&] { OpenSaveDialog(SaveDialogMode::Load); SyncUi(); }, AccentButton(kGold));
    menuButtons_[3] = Button(&menuLabels_[3], [&] {
        infoReturnsToRun_ = false;
        engine_.showLeaderboard("data/leaderboard.txt");
        SyncUi();
    }, AccentButton(kHull));
    menuButtons_[4] = Button(&menuLabels_[4], [&] {
        infoReturnsToRun_ = false;
        engine_.showHelp();
        SyncUi();
    }, AccentButton(kHull));
    menuButtons_[5] = Button(&menuLabels_[5], [&] {
        language_ = language_ == UiLanguage::English ? UiLanguage::Chinese : UiLanguage::English;
        SyncUi();
    }, AccentButton(kGold));
    menuButtons_[6] = Button(&menuLabels_[6], [&] { screen_.ExitLoopClosure()(); }, AccentButton(kEnemy));

    for (size_t i = 0; i < mapButtons_.size(); ++i) {
        mapButtons_[i] = Button(&mapLabels_[i], [&, i] { engine_.chooseNode(i); AutoSave(); SyncUi(); }, AccentButton(kHull));
    }
    for (size_t i = 0; i < encounterButtons_.size(); ++i) {
        encounterButtons_[i] = Button(&encounterLabels_[i], [&, i] { engine_.resolveEncounter(i); AutoSave(); SyncUi(); }, AccentButton(kGold));
    }
    for (size_t i = 0; i < rewardButtons_.size(); ++i) {
        rewardButtons_[i] = Button(&rewardLabels_[i], [&, i] { engine_.chooseReward(static_cast<int>(i)); AutoSave(); SyncUi(); }, AccentButton(kGold));
    }
    for (size_t i = 0; i < battleCardButtons_.size(); ++i) {
        battleCardButtons_[i] = Button(&battleCardLabels_[i], [&, i] { engine_.playCard(i); AutoSave(); SyncUi(); }, AccentButton(kHull));
    }

    difficultyToggle_ = Toggle(&difficultyEntries_, &difficultyIndex_);
    shipToggle_ = Toggle(&shipEntries_, &shipIndex_);
    startRunButton_ = Button(&startRunLabel_, [&] {
        const std::array<std::string, 3> ships = {"vanguard", "drift_fox", "iron_bastion"};
        engine_.startNewRun(difficultyIndex_, ships[static_cast<size_t>(shipIndex_)]);
        AutoSave();
        SyncUi();
    }, AccentButton(kGold));
    setupBackButton_ = Button(&setupBackLabel_, [&] { engine_.showMainMenu(); SyncUi(); }, AccentButton(kEnemy));

    mapSaveButton_ = Button(&mapSaveLabel_, [&] { OpenSaveDialog(SaveDialogMode::Save); SyncUi(); }, AccentButton(kGold));
    mapMenuButton_ = Button(&mapMenuLabel_, [&] { engine_.returnToMenu(); SyncUi(); }, AccentButton(kEnemy));
    encounterSaveButton_ = Button(&encounterSaveLabel_, [&] { OpenSaveDialog(SaveDialogMode::Save); SyncUi(); }, AccentButton(kGold));
    encounterMenuButton_ = Button(&encounterMenuLabel_, [&] { engine_.returnToMenu(); SyncUi(); }, AccentButton(kEnemy));
    battleEndTurnButton_ = Button(&battleEndTurnLabel_, [&] { engine_.endTurn(); AutoSave(); SyncUi(); }, AccentButton(kGold));
    battleSaveButton_ = Button(&battleSaveLabel_, [&] { OpenSaveDialog(SaveDialogMode::Save); SyncUi(); }, AccentButton(kGold));
    battleMenuButton_ = Button(&battleMenuLabel_, [&] { engine_.returnToMenu(); SyncUi(); }, AccentButton(kEnemy));
    rewardSkipButton_ = Button(&rewardSkipLabel_, [&] { engine_.chooseReward(-1); AutoSave(); SyncUi(); }, AccentButton(kEnemy));
    rewardMenuButton_ = Button(&rewardMenuLabel_, [&] { engine_.returnToMenu(); SyncUi(); }, AccentButton(kEnemy));
    infoBackButton_ = Button(&infoBackLabel_, [&] { BackFromInfo(); }, AccentButton(kHull));
    summaryMenuButton_ = Button(&summaryMenuLabel_, [&] { engine_.showMainMenu(); SyncUi(); }, AccentButton(kHull));
    summaryNewRunButton_ = Button(&summaryNewRunLabel_, [&] { engine_.startAnotherRun(); SyncUi(); }, AccentButton(kGold));
    summaryExitButton_ = Button(&summaryExitLabel_, [&] { screen_.ExitLoopClosure()(); }, AccentButton(kEnemy));
    saveNameInputComponent_ = Input(&saveNameInput_, "");
    saveEntriesMenu_ = Menu(&saveEntries_, &saveSelected_);
    saveConfirmButton_ = Button(&saveConfirmLabel_, [&] { SaveRun(); SyncUi(); }, AccentButton(kGold));
    loadConfirmButton_ = Button(&loadConfirmLabel_, [&] { LoadRun(); SyncUi(); }, AccentButton(kHull));
    autoSaveButton_ = Button(&autoSaveLabel_, [&] { SaveToSlot("autosave", true); SyncUi(); }, AccentButton(kGold));
    closeDialogButton_ = Button(&closeDialogLabel_, [&] { CloseSaveDialog(); SyncUi(); }, AccentButton(kEnemy));

    mainMenuPage_ = Renderer(Container::Vertical(std::vector<Component>(menuButtons_.begin(), menuButtons_.end())), [&] { return RenderMainMenu(); });
    setupPage_ = Renderer(Container::Vertical({difficultyToggle_, shipToggle_, startRunButton_, setupBackButton_}), [&] { return RenderSetup(); });
    mapPage_ = Renderer(Container::Vertical({mapButtons_[0], mapButtons_[1], mapButtons_[2], mapSaveButton_, mapMenuButton_}), [&] { return RenderMap(); });
    encounterPage_ = Renderer(Container::Vertical({encounterButtons_[0], encounterButtons_[1], encounterButtons_[2], encounterButtons_[3], encounterButtons_[4], encounterSaveButton_, encounterMenuButton_}),
                              [&] { return RenderEncounter(); });
    {
        std::vector<Component> battle_components(battleCardButtons_.begin(), battleCardButtons_.end());
        battle_components.push_back(battleEndTurnButton_);
        battle_components.push_back(battleSaveButton_);
        battle_components.push_back(battleMenuButton_);
        battlePage_ = Renderer(Container::Vertical(std::move(battle_components)), [&] { return RenderBattle(); });
    }
    rewardPage_ = Renderer(Container::Vertical({rewardButtons_[0], rewardButtons_[1], rewardButtons_[2], rewardSkipButton_, rewardMenuButton_}),
                           [&] { return RenderReward(); });
    summaryPage_ = Renderer(Container::Vertical({summaryMenuButton_, summaryNewRunButton_, summaryExitButton_}), [&] { return RenderSummary(); });
    infoPage_ = Renderer(Container::Vertical({infoBackButton_}), [&] { return RenderInfo(); });
    saveDialogPage_ = Renderer(Container::Vertical({saveNameInputComponent_, saveEntriesMenu_, saveConfirmButton_, loadConfirmButton_, autoSaveButton_, closeDialogButton_}),
                               [&] { return RenderSaveDialog(); });

    std::vector<Component> page_list = {
        mainMenuPage_, setupPage_, mapPage_, encounterPage_, battlePage_, rewardPage_, summaryPage_, infoPage_,
    };
    pages_ = Container::Tab(std::move(page_list), &activePage_);
    root_ = CatchEvent(Modal(pages_, saveDialogPage_, &saveDialogVisible_), [&](Event event) { return HandleEvent(event); });

    RefreshSaveEntries();
    SyncUi();
    ticker_ = std::thread([this]() {
        while (running_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(90));
            screen_.Post(Event::Custom);
        }
    });
}

StarAbyssApp::~StarAbyssApp() {
    running_ = false;
    if (ticker_.joinable()) {
        ticker_.join();
    }
}

ftxui::Component CreateApp(ftxui::ScreenInteractive& screen) {
    auto app = std::make_shared<StarAbyssApp>(screen);
    return CatchEvent(app->root(), [app](Event) { return false; });
}
