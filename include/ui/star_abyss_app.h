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
    // Creates the terminal app around the given screen and prepares all pages and overlays.
    explicit StarAbyssApp(ftxui::ScreenInteractive& screen);

    // Stops the app's background work during destruction and returns no value.
    ~StarAbyssApp();

    // Gives FTXUI the app root component, with no extra input needed.
    ftxui::Component root() const { return root_; }

private:
    enum class CardViewerMode {
        None = 0,
        OwnedCards,
        CardLibrary,
        Achievements,
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

    // Syncs visible labels, selected pages, and overlay state from the engine without returning anything.
    void SyncUi();

    // Processes one FTXUI event and returns whether the app handled it.
    bool HandleEvent(ftxui::Event event);

    // Scrolls the active view by the given line delta and returns whether scrolling was applied.
    bool ScrollActiveView(int delta_lines);

    // Saves the current run through the active save dialog state and returns nothing.
    void SaveRun();

    // Loads the selected run from the save dialog and returns nothing.
    void LoadRun();

    // Leaves an information page for the proper previous page and returns nothing.
    void BackFromInfo();

    // Opens the save dialog in the requested mode and returns nothing.
    void OpenSaveDialog(SaveDialogMode mode);

    // Closes the save dialog without taking input and returns nothing.
    void CloseSaveDialog();

    // Refreshes save slot lists from disk without taking input and returns nothing.
    void RefreshSaveEntries();

    // Saves to the named slot, optionally showing feedback, and returns nothing.
    void SaveToSlot(const std::string& slot_name, bool show_feedback);

    // Loads the currently selected save entry without taking input and returns nothing.
    void LoadSelectedSave();

    // Writes the active run to autosave when possible and returns nothing.
    void AutoSave();

    // Cleans a raw save name for filesystem use and returns the sanitized slot name.
    std::string SanitizeSaveName(const std::string& raw_name) const;

    // Formats the save entry at the given index and returns its display name.
    std::string SaveDisplayName(size_t index) const;

    // Opens the card viewer in the requested mode and returns nothing.
    void OpenCardViewer(CardViewerMode mode);

    // Closes the card viewer without input and returns nothing.
    void CloseCardViewer();

    // Toggles the requested battle pile inspector and returns nothing.
    void ToggleBattlePileInspector(BattlePileInspector target);

    // Closes the active battle pile inspector without input and returns nothing.
    void CloseBattlePileInspector();

    // Loads the saved UI language preference from disk and returns nothing.
    void LoadLanguagePreference();

    // Saves the current UI language preference without input and returns nothing.
    void SaveLanguagePreference() const;

    // Frames the given content at the requested scroll line and returns the scrollable element.
    ftxui::Element ApplyVerticalScrollFrame(ftxui::Element content, int scroll_line) const;

    // Converts an app page enum to an array index and returns that index.
    static constexpr size_t PageIndex(AppPage page) { return static_cast<size_t>(page); }

    // Renders the main menu without input and returns the page element.
    ftxui::Element RenderMainMenu() const;

    // Renders run setup without input and returns the page element.
    ftxui::Element RenderSetup() const;

    // Renders the route map without input and returns the page element.
    ftxui::Element RenderMap() const;

    // Renders the active encounter without input and returns the page element.
    ftxui::Element RenderEncounter() const;

    // Renders the battle screen without input and returns the page element.
    ftxui::Element RenderBattle() const;

    // Renders reward selection without input and returns the page element.
    ftxui::Element RenderReward() const;

    // Renders the run summary without input and returns the page element.
    ftxui::Element RenderSummary() const;

    // Renders the information page without input and returns the page element.
    ftxui::Element RenderInfo() const;

    // Renders the save dialog without input and returns the overlay element.
    ftxui::Element RenderSaveDialog() const;

    // Renders the card viewer without input and returns the overlay element.
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

    std::vector<std::string> difficultyEntries_ = {"Easy", "Normal", "Hard", "Hell"};
    std::vector<std::string> shipEntries_ = {"Vanguard", "Drift Fox", "Iron Bastion"};

    std::array<std::string, 8> menuLabels_{};
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

    std::array<ftxui::Component, 8> menuButtons_{};
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
