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
    // what it does: Constructs the terminal UI application and wires its components.
    // what the inputs are: screen - interactive FTXUI screen used for rendering and events.
    // what the outputs are: Creates a StarAbyssApp instance (constructor).
    explicit StarAbyssApp(ftxui::ScreenInteractive& screen);

    // what it does: Stops background app activity and releases UI resources.
    // what the inputs are: None.
    // what the outputs are: No return value (destructor).
    ~StarAbyssApp();

    // what it does: Provides the root component used by the FTXUI event loop.
    // what the inputs are: None.
    // what the outputs are: Returns the app root component (ftxui::Component).
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

    // what it does: Synchronizes UI labels, selected pages, and controls with engine state.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void SyncUi();

    // what it does: Handles a single keyboard or mouse event for app-level interactions.
    // what the inputs are: event - FTXUI event to process.
    // what the outputs are: Returns true when the event is consumed; otherwise false (bool).
    bool HandleEvent(ftxui::Event event);

    // what it does: Scrolls the currently active scrollable view.
    // what the inputs are: delta_lines - signed number of lines to scroll.
    // what the outputs are: Returns true when scrolling was handled; otherwise false (bool).
    bool ScrollActiveView(int delta_lines);

    // what it does: Saves the current run using the active save dialog context.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void SaveRun();

    // what it does: Opens the load-run flow and refreshes available save slots.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void LoadRun();

    // what it does: Returns from an information page to the correct previous page.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void BackFromInfo();

    // what it does: Opens the save dialog in the requested mode.
    // what the inputs are: mode - save dialog mode to display.
    // what the outputs are: No return value (void).
    void OpenSaveDialog(SaveDialogMode mode);

    // what it does: Closes the save dialog and clears its visible state.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void CloseSaveDialog();

    // what it does: Rebuilds the list of visible save entries from disk.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void RefreshSaveEntries();

    // what it does: Saves the current run to a named save slot.
    // what the inputs are: slot_name - target save slot name; show_feedback - whether to show status feedback.
    // what the outputs are: No return value (void).
    void SaveToSlot(const std::string& slot_name, bool show_feedback);

    // what it does: Loads the save slot currently selected in the save dialog.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void LoadSelectedSave();

    // what it does: Writes the current run to the autosave slot.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void AutoSave();

    // what it does: Removes invalid characters from a user-entered save name.
    // what the inputs are: raw_name - original save name text.
    // what the outputs are: Returns a filesystem-safe save name (std::string).
    std::string SanitizeSaveName(const std::string& raw_name) const;

    // what it does: Builds the display label for a save entry.
    // what the inputs are: index - zero-based save entry index.
    // what the outputs are: Returns the display label text (std::string).
    std::string SaveDisplayName(size_t index) const;

    // what it does: Opens the card viewer in the requested browsing mode.
    // what the inputs are: mode - card viewer mode to show.
    // what the outputs are: No return value (void).
    void OpenCardViewer(CardViewerMode mode);

    // what it does: Closes the card viewer and resets its visible state.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void CloseCardViewer();

    // what it does: Opens or closes the battle pile inspector for the requested pile.
    // what the inputs are: target - pile inspector target to toggle.
    // what the outputs are: No return value (void).
    void ToggleBattlePileInspector(BattlePileInspector target);

    // what it does: Closes the battle pile inspector.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void CloseBattlePileInspector();

    // what it does: Loads the saved UI language preference from disk.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void LoadLanguagePreference();

    // what it does: Saves the current UI language preference to disk.
    // what the inputs are: None.
    // what the outputs are: No return value (void).
    void SaveLanguagePreference() const;

    // what it does: Applies a vertical scroll frame to long rendered content.
    // what the inputs are: content - element to frame; scroll_line - top visible line offset.
    // what the outputs are: Returns the framed element (ftxui::Element).
    ftxui::Element ApplyVerticalScrollFrame(ftxui::Element content, int scroll_line) const;

    // what it does: Converts an app page enum to the matching array index.
    // what the inputs are: page - app page enum value.
    // what the outputs are: Returns the page index (size_t).
    static constexpr size_t PageIndex(AppPage page) { return static_cast<size_t>(page); }

    // what it does: Renders the main menu page.
    // what the inputs are: None.
    // what the outputs are: Returns the page element (ftxui::Element).
    ftxui::Element RenderMainMenu() const;

    // what it does: Renders the run setup page.
    // what the inputs are: None.
    // what the outputs are: Returns the page element (ftxui::Element).
    ftxui::Element RenderSetup() const;

    // what it does: Renders the route map page.
    // what the inputs are: None.
    // what the outputs are: Returns the page element (ftxui::Element).
    ftxui::Element RenderMap() const;

    // what it does: Renders the active encounter page.
    // what the inputs are: None.
    // what the outputs are: Returns the page element (ftxui::Element).
    ftxui::Element RenderEncounter() const;

    // what it does: Renders the battle page.
    // what the inputs are: None.
    // what the outputs are: Returns the page element (ftxui::Element).
    ftxui::Element RenderBattle() const;

    // what it does: Renders the reward selection page.
    // what the inputs are: None.
    // what the outputs are: Returns the page element (ftxui::Element).
    ftxui::Element RenderReward() const;

    // what it does: Renders the run summary page.
    // what the inputs are: None.
    // what the outputs are: Returns the page element (ftxui::Element).
    ftxui::Element RenderSummary() const;

    // what it does: Renders the information page.
    // what the inputs are: None.
    // what the outputs are: Returns the page element (ftxui::Element).
    ftxui::Element RenderInfo() const;

    // what it does: Renders the save/load dialog overlay.
    // what the inputs are: None.
    // what the outputs are: Returns the dialog element (ftxui::Element).
    ftxui::Element RenderSaveDialog() const;

    // what it does: Renders the card viewer overlay.
    // what the inputs are: None.
    // what the outputs are: Returns the viewer element (ftxui::Element).
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
