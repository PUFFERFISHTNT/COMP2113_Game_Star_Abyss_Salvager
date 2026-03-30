#include "ui/star_abyss_app.h"

#include <ftxui/component/event.hpp>
#include <ftxui/component/mouse.hpp>

#include "ui/ui_localization.h"

using namespace ftxui;

bool StarAbyssApp::HandleEvent(Event event) {
    const auto active_scroll_component = [&]() -> Component {
        if (saveDialogVisible_) {
            return saveDialogPage_;
        }

        switch (engine_.state().page) {
        case AppPage::MainMenu:
            return mainMenuPage_;
        case AppPage::Setup:
            return setupPage_;
        case AppPage::Map:
            return mapPage_;
        case AppPage::Encounter:
            return encounterPage_;
        case AppPage::Battle:
            return battlePage_;
        case AppPage::Reward:
            return rewardPage_;
        case AppPage::Summary:
            return summaryPage_;
        case AppPage::Info:
            return infoPage_;
        default:
            return nullptr;
        }
    };

    const auto forward_scroll_event = [&](Event forwarded) {
        Component target = active_scroll_component();
        if (target != nullptr && target->OnEvent(forwarded)) {
            SyncUi();
            return true;
        }
        return false;
    };

    const auto handle_mouse_wheel = [&](Event mouse_event) {
        if (!mouse_event.is_mouse()) {
            return false;
        }
        if (mouse_event.mouse().button != Mouse::WheelUp && mouse_event.mouse().button != Mouse::WheelDown) {
            return false;
        }
        if (forward_scroll_event(mouse_event)) {
            return true;
        }
        return forward_scroll_event(mouse_event.mouse().button == Mouse::WheelUp ? Event::PageUp : Event::PageDown);
    };

    if (event == Event::Custom) {
        ++ambientTick_;
        engine_.tickAnimation();
        SyncUi();
        return false;
    }

    if (handle_mouse_wheel(event)) {
        return true;
    }

    if (event == Event::ArrowUpCtrl || event == Event::ArrowDownCtrl) {
        return forward_scroll_event(event == Event::ArrowUpCtrl ? Event::PageUp : Event::PageDown);
    }

    if (event == Event::Character('[') || event == Event::Character('{')) {
        return forward_scroll_event(Event::PageUp);
    }
    if (event == Event::Character(']') || event == Event::Character('}')) {
        return forward_scroll_event(Event::PageDown);
    }

    if (saveDialogVisible_ && event == Event::Escape) {
        CloseSaveDialog();
        SyncUi();
        return true;
    }
    if (saveDialogVisible_) {
        return false;
    }

    if (event == Event::Escape || event == Event::Character('q')) {
        if (engine_.state().page == AppPage::MainMenu) {
            screen_.ExitLoopClosure()();
        } else if (engine_.state().page == AppPage::Info) {
            BackFromInfo();
        } else {
            engine_.returnToMenu();
            SyncUi();
        }
        return true;
    }

    if (event == Event::Character('s')) {
        OpenSaveDialog(SaveDialogMode::Save);
        SyncUi();
        return true;
    }
    if (event == Event::Character('l')) {
        OpenSaveDialog(SaveDialogMode::Load);
        SyncUi();
        return true;
    }
    if (event == Event::Character('e') && engine_.state().page == AppPage::Battle) {
        engine_.endTurn();
        AutoSave();
        SyncUi();
        return true;
    }

    if (event.is_character()) {
        const std::string chars = event.character();
        if (chars.empty()) return false;
        const char c = chars[0];
        const AppPage page = engine_.state().page;

        if (page == AppPage::Battle && c == '0') {
            engine_.playCard(9);
            AutoSave();
            SyncUi();
            return true;
        }

        if (c >= '1' && c <= '9') {
            const int index = c - '1';
            switch (page) {
            case AppPage::MainMenu:
                if (index == 0) engine_.resumeRun();
                else if (index == 1) engine_.startAnotherRun();
                else if (index == 2) LoadRun();
                else if (index == 3) {
                    infoReturnsToRun_ = false;
                    engine_.showLeaderboard("data/leaderboard.txt");
                } else if (index == 4) {
                    infoReturnsToRun_ = false;
                    engine_.showHelp();
                } else if (index == 5) {
                    language_ = language_ == UiLanguage::English ? UiLanguage::Chinese : UiLanguage::English;
                } else if (index == 6) screen_.ExitLoopClosure()();
                break;
            case AppPage::Map:
                if (static_cast<size_t>(index) < engine_.state().routeOptions.size() && static_cast<size_t>(index) < mapButtons_.size()) {
                    engine_.chooseNode(static_cast<size_t>(index));
                    AutoSave();
                }
                break;
            case AppPage::Encounter:
                if (static_cast<size_t>(index) < engine_.state().encounter.choices.size() && static_cast<size_t>(index) < encounterButtons_.size()) {
                    engine_.resolveEncounter(static_cast<size_t>(index));
                    AutoSave();
                }
                break;
            case AppPage::Battle:
                if (index >= 0 && static_cast<size_t>(index) < kBattleSlots) {
                    engine_.playCard(static_cast<size_t>(index));
                    AutoSave();
                }
                break;
            case AppPage::Reward:
                if (static_cast<size_t>(index) < engine_.state().reward.cardIds.size() && static_cast<size_t>(index) < rewardButtons_.size()) {
                    engine_.chooseReward(index);
                    AutoSave();
                } else if (index == static_cast<int>(engine_.state().reward.cardIds.size())) {
                    engine_.chooseReward(-1);
                    AutoSave();
                }
                break;
            default:
                break;
            }
            SyncUi();
            return true;
        }
    }
    return false;
}

void StarAbyssApp::BackFromInfo() {
    if (infoReturnsToRun_ && engine_.state().hasRun) engine_.resumeRun();
    else engine_.showMainMenu();
    SyncUi();
}
