#include "ui/star_abyss_app.h"
#include "engine/game_dev.h"

#include <algorithm>

#include <ftxui/component/event.hpp>
#include <ftxui/component/mouse.hpp>
#include <ftxui/dom/elements.hpp>

#include "ui/ui_localization.h"

using namespace ftxui;

Element StarAbyssApp::ApplyVerticalScrollFrame(Element content, int scroll_line) const {
    return content | focusPosition(0, scroll_line) | vscroll_indicator | yframe;
}

bool StarAbyssApp::ScrollActiveView(int delta_lines) {
    int& scroll_line = saveDialogVisible_ ? saveDialogScrollLine_
                                          : (cardViewerVisible_ ? cardViewerScrollLine_
                                                                : pageScrollLines_[PageIndex(engine_.state().page)]);
    scroll_line = std::max(0, scroll_line + delta_lines);
    return true;
}

bool StarAbyssApp::HandleEvent(Event event) {
    const auto contains = [](const Box& box, int x, int y) {
        return x >= box.x_min && x <= box.x_max && y >= box.y_min && y <= box.y_max;
    };
    const auto is_full_upgrade_gallery = [&]() {
        return engine_.state().page == AppPage::Encounter &&
               (engine_.state().encounter.encounterId == "market_upgrade" ||
                engine_.state().encounter.encounterId == "repair_upgrade" ||
                engine_.state().encounter.encounterId == "event_upgrade");
    };
    const auto active_scroll_component = [&]() -> Component {
        if (saveDialogVisible_) {
            return saveDialogPage_;
        }
        if (cardViewerVisible_) {
            return cardViewerPage_;
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
        const int delta = mouse_event.mouse().button == Mouse::WheelUp ? -kWheelScrollLines : kWheelScrollLines;
        if (saveDialogVisible_ && forward_scroll_event(mouse_event)) {
            return true;
        }
        return ScrollActiveView(delta);
    };

    if (event == Event::Custom) {
        ++ambientTick_;
        engine_.tickAnimation();
        SyncUi();
        return false;
    }

    // When the developer overlay is open it must take exclusive priority over
    // every global shortcut (digits for menu items, language toggle, scroll
    // hotkeys, etc.). Forward all events directly to the dev modal so its
    // Input components and Buttons receive raw key strokes (including digits
    // for the password). Only Escape is intercepted here as a safety exit.
    if (devModeVisible_) {
        if (event == Event::Escape) {
            if (devModeUi_ && devModeUi_->unlocked()) {
                devModeUi_->exitDeveloperMode();
            } else {
                devModeVisible_ = false;
                SyncUi();
            }
            return true;
        }
        return false;
    }

    // Same exclusive-priority rule for the persistent debug panel.
    if (devModeUi_ && devModeUi_->debugPanelVisible()) {
        return devModeUi_->handleOverlayEvent(event);
    }

    if (handle_mouse_wheel(event)) {
        return true;
    }

    if (is_full_upgrade_gallery() && event.is_mouse()) {
        const int mouse_x = event.mouse().x;
        const int mouse_y = event.mouse().y;
        if (event.mouse().motion == Mouse::Moved) {
            int hovered_index = -1;
            for (size_t i = 0; i < serviceCardBoxes_.size() && i < serviceCardChoiceIndices_.size(); ++i) {
                if (contains(serviceCardBoxes_[i], mouse_x, mouse_y)) {
                    hovered_index = static_cast<int>(i);
                    break;
                }
            }
            if (hovered_index != serviceHoveredCardIndex_) {
                serviceHoveredCardIndex_ = hovered_index;
                if (hovered_index >= 0) {
                    serviceFocusedCardIndex_ = hovered_index;
                }
                SyncUi();
                return true;
            }
        }
    }

    if (is_full_upgrade_gallery() &&
        event.is_mouse() &&
        event.mouse().button == Mouse::Left &&
        event.mouse().motion == Mouse::Released) {
        const int mouse_x = event.mouse().x;
        const int mouse_y = event.mouse().y;
        for (size_t i = 0; i < serviceCardBoxes_.size() && i < serviceCardChoiceIndices_.size(); ++i) {
            if (contains(serviceCardBoxes_[i], mouse_x, mouse_y)) {
                serviceFocusedCardIndex_ = static_cast<int>(i);
                engine_.resolveEncounter(serviceCardChoiceIndices_[i]);
                AutoSave();
                SyncUi();
                return true;
            }
        }
        if (serviceActionVisible_ && contains(serviceActionBox_, mouse_x, mouse_y)) {
            return false;
        }
    }

    if (is_full_upgrade_gallery()) {
        const int card_count = static_cast<int>(serviceCardChoiceIndices_.size());
        if (card_count > 0) {
            const int columns = std::max(1, serviceCardsPerRow_);
            if (event == Event::ArrowUp) {
                serviceHoveredCardIndex_ = -1;
                serviceFocusedCardIndex_ = std::max(0, serviceFocusedCardIndex_ - columns);
                SyncUi();
                return true;
            }
            if (event == Event::ArrowDown) {
                serviceHoveredCardIndex_ = -1;
                serviceFocusedCardIndex_ = std::min(card_count - 1, serviceFocusedCardIndex_ + columns);
                SyncUi();
                return true;
            }
            if (event == Event::ArrowLeft) {
                serviceHoveredCardIndex_ = -1;
                serviceFocusedCardIndex_ = std::max(0, serviceFocusedCardIndex_ - 1);
                SyncUi();
                return true;
            }
            if (event == Event::ArrowRight) {
                serviceHoveredCardIndex_ = -1;
                serviceFocusedCardIndex_ = std::min(card_count - 1, serviceFocusedCardIndex_ + 1);
                SyncUi();
                return true;
            }
            if (event == Event::Return || event == Event::Character('\n')) {
                const size_t choice_index = serviceCardChoiceIndices_[static_cast<size_t>(serviceFocusedCardIndex_)];
                engine_.resolveEncounter(choice_index);
                AutoSave();
                SyncUi();
                return true;
            }
        }
    }

    if (event == Event::PageUp || event == Event::PageDown) {
        const int delta = event == Event::PageUp ? -kPageScrollLines : kPageScrollLines;
        if (saveDialogVisible_ && forward_scroll_event(event)) {
            return true;
        }
        return ScrollActiveView(delta);
    }

    if (event == Event::ArrowUpCtrl || event == Event::ArrowDownCtrl) {
        const Event forwarded = event == Event::ArrowUpCtrl ? Event::PageUp : Event::PageDown;
        const int delta = event == Event::ArrowUpCtrl ? -kPageScrollLines : kPageScrollLines;
        if (saveDialogVisible_ && forward_scroll_event(forwarded)) {
            return true;
        }
        return ScrollActiveView(delta);
    }

    if (event == Event::Character('[') || event == Event::Character('{')) {
        if (saveDialogVisible_ && forward_scroll_event(Event::PageUp)) {
            return true;
        }
        return ScrollActiveView(-kPageScrollLines);
    }
    if (event == Event::Character(']') || event == Event::Character('}')) {
        if (saveDialogVisible_ && forward_scroll_event(Event::PageDown)) {
            return true;
        }
        return ScrollActiveView(kPageScrollLines);
    }

    if (saveDialogVisible_ && event == Event::Escape) {
        CloseSaveDialog();
        SyncUi();
        return true;
    }
    if (saveDialogVisible_) {
        return false;
    }

    if (cardViewerVisible_) {
        const AppPage page = engine_.state().page;
        const bool owned_cards_page = page == AppPage::Map || page == AppPage::Encounter ||
                                      page == AppPage::Battle || page == AppPage::Reward;
        const bool close_owned_cards = event == Event::Character('d') &&
                                       cardViewerMode_ == CardViewerMode::OwnedCards &&
                                       owned_cards_page;
        const bool close_library = event == Event::Character('c') &&
                                   cardViewerMode_ == CardViewerMode::CardLibrary &&
                                   page == AppPage::MainMenu;
        if (event == Event::Escape || close_owned_cards || close_library) {
            CloseCardViewer();
            SyncUi();
            return true;
        }
        return false;
    }

    if (engine_.state().page == AppPage::Battle && battlePileInspector_ != BattlePileInspector::None && event == Event::Escape) {
        CloseBattlePileInspector();
        SyncUi();
        return true;
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
    if (event == Event::Character('d')) {
        const AppPage page = engine_.state().page;
        if (page == AppPage::Map || page == AppPage::Encounter || page == AppPage::Battle || page == AppPage::Reward) {
            OpenCardViewer(CardViewerMode::OwnedCards);
            SyncUi();
            return true;
        }
    }
    if (event == Event::Character('c') && engine_.state().page == AppPage::MainMenu) {
        OpenCardViewer(CardViewerMode::CardLibrary);
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
                    SaveLanguagePreference();
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

void StarAbyssApp::OpenCardViewer(CardViewerMode mode) {
    if (saveDialogVisible_ || mode == CardViewerMode::None) {
        return;
    }
    const AppPage page = engine_.state().page;
    const bool owned_cards_page = page == AppPage::Map || page == AppPage::Encounter ||
                                  page == AppPage::Battle || page == AppPage::Reward;
    if ((mode == CardViewerMode::OwnedCards && !owned_cards_page) ||
        (mode == CardViewerMode::CardLibrary && page != AppPage::MainMenu)) {
        return;
    }
    cardViewerMode_ = mode;
    cardViewerVisible_ = true;
    cardViewerScrollLine_ = 0;
}

void StarAbyssApp::CloseCardViewer() {
    cardViewerVisible_ = false;
    cardViewerMode_ = CardViewerMode::None;
    cardViewerScrollLine_ = 0;
}

void StarAbyssApp::ToggleBattlePileInspector(BattlePileInspector target) {
    battlePileInspector_ = battlePileInspector_ == target ? BattlePileInspector::None : target;
    pageScrollLines_[PageIndex(AppPage::Battle)] = 0;
}

void StarAbyssApp::CloseBattlePileInspector() {
    battlePileInspector_ = BattlePileInspector::None;
}

void StarAbyssApp::BackFromInfo() {
    if (infoReturnsToRun_ && engine_.state().hasRun) engine_.resumeRun();
    else engine_.showMainMenu();
    SyncUi();
}
