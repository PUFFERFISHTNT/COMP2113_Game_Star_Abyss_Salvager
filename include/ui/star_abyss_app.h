#ifndef STAR_ABYSS_APP_H
#define STAR_ABYSS_APP_H

#include <array>
#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/screen/box.hpp>

#include "tui_game.h"
#include "ui/ui_localization.h"
#include "ui/ui_save_dialog.h"

namespace dev_mode { class DevModeUi; }

class StarAbyssApp {
public:
    explicit StarAbyssApp(ftxui::ScreenInteractive& screen);
    ~StarAbyssApp();

    ftxui::Component root() const { return root_; }

private:
    enum class CardViewerMode {
        None = 0,
        OwnedCards,
        CardLibrary,
    };

    enum class BattlePileInspector {
        None = 0,
        Draw,
        Discard,
    };

    static constexpr size_t kBattleSlots = static_cast<size_t>(kBattleHandLimit);
    static constexpr size_t kMapSlots = 3;
    static constexpr size_t kEncounterSlots = 6;
    static constexpr size_t kRewardSlots = 3;
    static constexpr size_t kPageCount = static_cast<size_t>(AppPage::Info) + 1;
    static constexpr int kWheelScrollLines = 4;
    static constexpr int kPageScrollLines = 14;

    void SyncUi();
    bool HandleEvent(ftxui::Event event);
    bool ScrollActiveView(int delta_lines);
    void SaveRun();
    void LoadRun();
    void BackFromInfo();
    void OpenSaveDialog(SaveDialogMode mode);
    void CloseSaveDialog();
    void RefreshSaveEntries();
    void SaveToSlot(const std::string& slot_name, bool show_feedback);
    void LoadSelectedSave();
    void AutoSave();
    std::string SanitizeSaveName(const std::string& raw_name) const;
    std::string SaveDisplayName(size_t index) const;
    void OpenCardViewer(CardViewerMode mode);
    void CloseCardViewer();
    void ToggleBattlePileInspector(BattlePileInspector target);
    void CloseBattlePileInspector();
    void LoadLanguagePreference();
    void SaveLanguagePreference() const;
    ftxui::Element ApplyVerticalScrollFrame(ftxui::Element content, int scroll_line) const;
    static constexpr size_t PageIndex(AppPage page) { return static_cast<size_t>(page); }

    ftxui::Element RenderMainMenu() const;
    ftxui::Element RenderSetup() const;
    ftxui::Element RenderMap() const;
    ftxui::Element RenderEncounter() const;
    ftxui::Element RenderBattle() const;
    ftxui::Element RenderReward() const;
    ftxui::Element RenderSummary() const;
    ftxui::Element RenderInfo() const;
    ftxui::Element RenderSaveDialog() const;
    ftxui::Element RenderCardViewer() const;

    ftxui::ScreenInteractive& screen_;
    GameEngine engine_;
    std::atomic<bool> running_{true};
    std::thread ticker_;
    int activePage_ = 0;
    int difficultyIndex_ = 1;
    int shipIndex_ = 0;
    int ambientTick_ = 0;
    bool infoReturnsToRun_ = false;
    UiLanguage language_ = UiLanguage::English;
    CardViewerMode cardViewerMode_ = CardViewerMode::None;
    bool cardViewerVisible_ = false;
    BattlePileInspector battlePileInspector_ = BattlePileInspector::None;
    std::array<int, kPageCount> pageScrollLines_{};
    SaveDialogMode saveDialogMode_ = SaveDialogMode::None;
    bool saveDialogVisible_ = false;
    int cardViewerScrollLine_ = 0;
    int saveDialogScrollLine_ = 0;
    int saveSelected_ = 0;
    std::string saveNameInput_ = "save1";
    mutable ftxui::Box battleLaneRowsBox_;
    mutable std::vector<ftxui::Box> battleAuxBoxes_;
    mutable std::vector<ftxui::Box> serviceCardBoxes_;
    mutable std::vector<size_t> serviceCardChoiceIndices_;
    mutable ftxui::Box serviceActionBox_;
    mutable bool serviceActionVisible_ = false;
    // Valid back/skip choice index for the current service screen action box.
    mutable size_t serviceActionChoiceIndex_ = 0;
    mutable bool serviceActionHovered_ = false;
    mutable bool serviceMouseReleaseArmed_ = false;
    mutable std::string serviceMouseScreenId_;
    mutable int serviceFocusedCardIndex_ = 0;
    mutable int serviceHoveredCardIndex_ = -1;
    mutable int serviceCardsPerRow_ = 1;
    std::vector<std::string> saveEntries_;
    std::vector<std::string> saveSlotNames_;
    std::vector<std::string> savePaths_;

    std::vector<std::string> difficultyEntries_ = {"Easy", "Normal", "Hard"};
    std::vector<std::string> shipEntries_ = {"Vanguard", "Drift Fox", "Iron Bastion"};

    std::array<std::string, 7> menuLabels_{};
    std::string menuDevLabel_ = "Developer Mode";
    std::array<std::string, kMapSlots> mapLabels_{};
    std::array<std::string, kEncounterSlots> encounterLabels_{};
    std::array<std::string, kBattleSlots> battleCardLabels_{};
    std::array<std::string, 4> battleTransformLabels_{};
    std::array<std::string, kRewardSlots> rewardLabels_{};
    std::string startRunLabel_ = "Launch Run";
    std::string setupBackLabel_ = "Back";
    std::string mapSaveLabel_ = "Save Run";
    std::string mapMenuLabel_ = "Main Menu";
    std::string encounterSaveLabel_ = "Save Run";
    std::string encounterMenuLabel_ = "Main Menu";
    std::string encounterOwnedCardsLabel_ = "Owned Cards";
    std::string battleEndTurnLabel_ = "End Turn";
    std::string battleSaveLabel_ = "Save Run";
    std::string battleMenuLabel_ = "Main Menu";
    std::string battleOwnedCardsLabel_ = "Owned Cards";
    std::string battleDrawPileLabel_ = "Draw Pile";
    std::string battleDiscardPileLabel_ = "Discard Pile";
    std::string rewardSkipLabel_ = "Skip Card Reward";
    std::string rewardMenuLabel_ = "Main Menu";
    std::string rewardOwnedCardsLabel_ = "Owned Cards";
    std::string infoBackLabel_ = "Back";
    std::string summaryMenuLabel_ = "Main Menu";
    std::string summaryNewRunLabel_ = "New Run";
    std::string summaryExitLabel_ = "Exit";
    std::string saveConfirmLabel_ = "Save";
    std::string loadConfirmLabel_ = "Load";
    std::string autoSaveLabel_ = "Auto Save";
    std::string closeDialogLabel_ = "Close";
    std::string menuLibraryLabel_ = "Card Library";
    std::string mapOwnedCardsLabel_ = "Owned Cards";
    std::string cardViewerCloseLabel_ = "Close";

    std::array<ftxui::Component, 7> menuButtons_{};
    ftxui::Component menuDevButton_;
    std::array<ftxui::Component, kMapSlots> mapButtons_{};
    std::array<ftxui::Component, kEncounterSlots> encounterButtons_{};
    std::array<ftxui::Component, kBattleSlots> battleCardButtons_{};
    std::array<ftxui::Component, 4> battleTransformButtons_{};
    std::array<ftxui::Component, kRewardSlots> rewardButtons_{};

    ftxui::Component difficultyToggle_;
    ftxui::Component shipToggle_;
    ftxui::Component startRunButton_;
    ftxui::Component setupBackButton_;
    ftxui::Component mapSaveButton_;
    ftxui::Component mapMenuButton_;
    ftxui::Component encounterSaveButton_;
    ftxui::Component encounterMenuButton_;
    ftxui::Component battleEndTurnButton_;
    ftxui::Component battleSaveButton_;
    ftxui::Component battleMenuButton_;
    ftxui::Component battleOwnedCardsButton_;
    ftxui::Component battleDrawPileButton_;
    ftxui::Component battleDiscardPileButton_;
    ftxui::Component rewardSkipButton_;
    ftxui::Component rewardMenuButton_;
    ftxui::Component rewardOwnedCardsButton_;
    ftxui::Component infoBackButton_;
    ftxui::Component summaryMenuButton_;
    ftxui::Component summaryNewRunButton_;
    ftxui::Component summaryExitButton_;
    ftxui::Component menuLibraryButton_;
    ftxui::Component mapOwnedCardsButton_;
    ftxui::Component encounterOwnedCardsButton_;
    ftxui::Component saveNameInputComponent_;
    ftxui::Component saveEntriesMenu_;
    ftxui::Component saveConfirmButton_;
    ftxui::Component loadConfirmButton_;
    ftxui::Component autoSaveButton_;
    ftxui::Component closeDialogButton_;
    ftxui::Component cardViewerCloseButton_;

    ftxui::Component mainMenuPage_;
    ftxui::Component setupPage_;
    ftxui::Component mapPage_;
    ftxui::Component encounterPage_;
    ftxui::Component battlePage_;
    ftxui::Component rewardPage_;
    ftxui::Component summaryPage_;
    ftxui::Component infoPage_;
    ftxui::Component cardViewerPage_;
    ftxui::Component saveDialogPage_;
    ftxui::Component pages_;
    ftxui::Component devModePage_;
    bool devModeVisible_ = false;
    std::shared_ptr<dev_mode::DevModeUi> devModeUi_;
    ftxui::Component root_;
};

#endif
