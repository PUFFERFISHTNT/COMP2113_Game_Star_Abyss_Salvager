#include "tui_app.h"

#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>

#include "ui/star_abyss_app.h"
#include "ui/ui_panels.h"
#include "engine/game_dev.h"

using namespace ftxui;

namespace {

constexpr const char* kLanguagePreferencePath = "save/ui_language.txt";

}  // namespace

StarAbyssApp::StarAbyssApp(ScreenInteractive& screen) : screen_(screen) {
    LoadLanguagePreference();

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
        SaveLanguagePreference();
        SyncUi();
    }, AccentButton(kGold));
    menuButtons_[6] = Button(&menuLabels_[6], [&] { screen_.ExitLoopClosure()(); }, AccentButton(kEnemy));
    menuDevButton_ = Button(&menuDevLabel_, [&] {
        if (!devModeUi_) {
            SyncUi();
            return;
        }
        devModeUi_->open();
        SyncUi();
    }, AccentButton(kEnemy));

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
    for (size_t i = 0; i < battleTransformButtons_.size(); ++i) {
        battleTransformButtons_[i] = Button(&battleTransformLabels_[i], [&, i] {
            if (engine_.selectAuxiliaryTransformTarget(i)) {
                AutoSave();
            }
            SyncUi();
        }, AccentButton(kGold));
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
    encounterOwnedCardsButton_ = Button(&encounterOwnedCardsLabel_, [&] { OpenCardViewer(CardViewerMode::OwnedCards); SyncUi(); }, AccentButton(kHull));
    battleEndTurnButton_ = Button(&battleEndTurnLabel_, [&] { engine_.endTurn(); AutoSave(); SyncUi(); }, AccentButton(kGold));
    battleSaveButton_ = Button(&battleSaveLabel_, [&] { OpenSaveDialog(SaveDialogMode::Save); SyncUi(); }, AccentButton(kGold));
    battleMenuButton_ = Button(&battleMenuLabel_, [&] { engine_.returnToMenu(); SyncUi(); }, AccentButton(kEnemy));
    battleOwnedCardsButton_ = Button(&battleOwnedCardsLabel_, [&] { OpenCardViewer(CardViewerMode::OwnedCards); SyncUi(); }, AccentButton(kHull));
    battleDrawPileButton_ = Button(&battleDrawPileLabel_, [&] { ToggleBattlePileInspector(BattlePileInspector::Draw); SyncUi(); }, AccentButton(kHull));
    battleDiscardPileButton_ = Button(&battleDiscardPileLabel_, [&] { ToggleBattlePileInspector(BattlePileInspector::Discard); SyncUi(); }, AccentButton(kEnemy));
    rewardSkipButton_ = Button(&rewardSkipLabel_, [&] { engine_.chooseReward(-1); AutoSave(); SyncUi(); }, AccentButton(kEnemy));
    rewardMenuButton_ = Button(&rewardMenuLabel_, [&] { engine_.returnToMenu(); SyncUi(); }, AccentButton(kEnemy));
    rewardOwnedCardsButton_ = Button(&rewardOwnedCardsLabel_, [&] { OpenCardViewer(CardViewerMode::OwnedCards); SyncUi(); }, AccentButton(kHull));
    infoBackButton_ = Button(&infoBackLabel_, [&] { BackFromInfo(); }, AccentButton(kHull));
    summaryMenuButton_ = Button(&summaryMenuLabel_, [&] { engine_.showMainMenu(); SyncUi(); }, AccentButton(kHull));
    summaryNewRunButton_ = Button(&summaryNewRunLabel_, [&] { engine_.startAnotherRun(); SyncUi(); }, AccentButton(kGold));
    summaryExitButton_ = Button(&summaryExitLabel_, [&] { screen_.ExitLoopClosure()(); }, AccentButton(kEnemy));
    menuLibraryButton_ = Button(&menuLibraryLabel_, [&] { OpenCardViewer(CardViewerMode::CardLibrary); SyncUi(); }, AccentButton(kHull));
    mapOwnedCardsButton_ = Button(&mapOwnedCardsLabel_, [&] { OpenCardViewer(CardViewerMode::OwnedCards); SyncUi(); }, AccentButton(kHull));
    saveNameInputComponent_ = Input(&saveNameInput_, "");
    saveEntriesMenu_ = Menu(&saveEntries_, &saveSelected_);
    saveConfirmButton_ = Button(&saveConfirmLabel_, [&] { SaveRun(); SyncUi(); }, AccentButton(kGold));
    loadConfirmButton_ = Button(&loadConfirmLabel_, [&] { LoadRun(); SyncUi(); }, AccentButton(kHull));
    autoSaveButton_ = Button(&autoSaveLabel_, [&] { SaveToSlot("autosave", true); SyncUi(); }, AccentButton(kGold));
    closeDialogButton_ = Button(&closeDialogLabel_, [&] { CloseSaveDialog(); SyncUi(); }, AccentButton(kEnemy));
    cardViewerCloseButton_ = Button(&cardViewerCloseLabel_, [&] { CloseCardViewer(); SyncUi(); }, AccentButton(kEnemy));

    mainMenuPage_ = Renderer(Container::Vertical({menuButtons_[0], menuButtons_[1], menuButtons_[2], menuButtons_[3], menuButtons_[4], menuLibraryButton_, menuButtons_[5], menuDevButton_, menuButtons_[6]}),
                             [&] { return RenderMainMenu(); });
    setupPage_ = Renderer(Container::Vertical({difficultyToggle_, shipToggle_, startRunButton_, setupBackButton_}), [&] { return RenderSetup(); });
    mapPage_ = Renderer(Container::Vertical({mapButtons_[0], mapButtons_[1], mapButtons_[2], mapOwnedCardsButton_, mapSaveButton_, mapMenuButton_}), [&] { return RenderMap(); });
    encounterPage_ = Renderer(Container::Vertical({encounterButtons_[0], encounterButtons_[1], encounterButtons_[2], encounterButtons_[3], encounterButtons_[4], encounterButtons_[5], encounterOwnedCardsButton_, encounterSaveButton_, encounterMenuButton_}),
                              [&] { return RenderEncounter(); });
    {
        std::vector<Component> battle_components(battleCardButtons_.begin(), battleCardButtons_.end());
        battle_components.insert(battle_components.end(), battleTransformButtons_.begin(), battleTransformButtons_.end());
        battle_components.push_back(battleOwnedCardsButton_);
        battle_components.push_back(battleDrawPileButton_);
        battle_components.push_back(battleDiscardPileButton_);
        battle_components.push_back(battleEndTurnButton_);
        battle_components.push_back(battleSaveButton_);
        battle_components.push_back(battleMenuButton_);
        battlePage_ = Renderer(Container::Vertical(std::move(battle_components)), [&] { return RenderBattle(); });
    }
    rewardPage_ = Renderer(Container::Vertical({rewardButtons_[0], rewardButtons_[1], rewardButtons_[2], rewardOwnedCardsButton_, rewardSkipButton_, rewardMenuButton_}),
                           [&] { return RenderReward(); });
    summaryPage_ = Renderer(Container::Vertical({summaryMenuButton_, summaryNewRunButton_, summaryExitButton_}), [&] { return RenderSummary(); });
    infoPage_ = Renderer(Container::Vertical({infoBackButton_}), [&] { return RenderInfo(); });
    cardViewerPage_ = Renderer(Container::Vertical({cardViewerCloseButton_}), [&] { return RenderCardViewer(); });
    saveDialogPage_ = Renderer(Container::Vertical({saveNameInputComponent_, saveEntriesMenu_, saveConfirmButton_, loadConfirmButton_, autoSaveButton_, closeDialogButton_}),
                               [&] { return RenderSaveDialog(); });

    std::vector<Component> page_list = {
        mainMenuPage_, setupPage_, mapPage_, encounterPage_, battlePage_, rewardPage_, summaryPage_, infoPage_,
    };
    pages_ = Container::Tab(std::move(page_list), &activePage_);
    devModeUi_ = std::make_shared<dev_mode::DevModeUi>(
        engine_, screen_, language_, devModeVisible_, [this] {
            devModeVisible_ = false;
            cardViewerVisible_ = false;
            cardViewerMode_ = CardViewerMode::None;
            saveDialogVisible_ = false;
            saveDialogMode_ = SaveDialogMode::None;
            battlePileInspector_ = BattlePileInspector::None;
            SyncUi();
        });
    devModePage_ = devModeUi_->component();
    auto modalStack = Modal(
        Modal(Modal(pages_, cardViewerPage_, &cardViewerVisible_), saveDialogPage_, &saveDialogVisible_),
        devModePage_, &devModeVisible_);
    auto devOverlay = devModeUi_->overlay();
    auto withOverlay = Container::Stacked({devOverlay, modalStack});
    root_ = CatchEvent(
        Renderer(withOverlay,
                 [modalStack, devOverlay] {
                     return dbox({modalStack->Render(), devOverlay->Render()});
                 }),
        [&](Event event) { return HandleEvent(event); });

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

void StarAbyssApp::LoadLanguagePreference() {
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::create_directories("save", ec);

    std::ifstream input(kLanguagePreferencePath);
    if (!input) {
        return;
    }

    std::string token;
    input >> token;
    if (token == "zh" || token == "ZH" || token == "chinese" || token == "Chinese") {
        language_ = UiLanguage::Chinese;
    } else if (token == "en" || token == "EN" || token == "english" || token == "English") {
        language_ = UiLanguage::English;
    }
}

void StarAbyssApp::SaveLanguagePreference() const {
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::create_directories("save", ec);

    std::ofstream output(kLanguagePreferencePath, std::ios::trunc);
    if (!output) {
        return;
    }
    output << (language_ == UiLanguage::Chinese ? "zh\n" : "en\n");
}

ftxui::Component CreateApp(ftxui::ScreenInteractive& screen) {
    auto app = std::make_shared<StarAbyssApp>(screen);
    auto layered = Renderer(app->root(), [app] {
        return dbox({
            filler() | bgcolor(kAbyss),
            app->root()->Render(),
        });
    });
    return CatchEvent(layered, [app](Event) { return false; });
}
