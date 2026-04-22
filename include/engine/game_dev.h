#ifndef GAME_DEV_H
#define GAME_DEV_H

// =============================================================================
// Developer mode (debug overlay).
//
// This module is fully optional and self-contained. It exposes two FTXUI
// components to the host application:
//
//   1. component()   - The password / options modal (opened from the main
//                      menu). Visibility is controlled externally by the
//                      host via a bool& reference.
//
//   2. overlay()     - A persistent overlay layer. Once the password has
//                      been validated, this layer renders a small floating
//                      "DEV" button anchored at the bottom-left corner of
//                      the screen. Clicking the button opens an independent
//                      debug panel that supports live HP/Shield/Energy/Heat/
//                      Credits adjustment, free card library access (every
//                      card in the game can be added straight to the
//                      player's hand at any time), and reloading the
//                      currently active encounter to restore its full
//                      description text.
//
// Strict bilingual rendering: every UI string emitted by the overlay is
// chosen via Tr(language_, english, chinese) so Chinese mode shows only
// Chinese and English mode shows only English. Engine-supplied gameplay
// text is routed through TranslateGameText exactly like the rest of the
// game.
// =============================================================================

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

#include "tui_game.h"
#include "ui/ui_localization.h"

namespace dev_mode {

inline constexpr const char* kPassword = "123456";

class DevModeUi {
public:
    DevModeUi(GameEngine& engine,
              ftxui::ScreenInteractive& screen,
              UiLanguage& language,
              bool& visible,
              std::function<void()> on_close);

    ftxui::Component component() const { return root_; }
    ftxui::Component overlay() const { return overlay_root_; }

    void open();
    void dismissModal();
    void exitDeveloperMode();
    bool handleOverlayEvent(ftxui::Event event);
    bool unlocked() const { return unlocked_; }
    bool debugPanelVisible() const { return debug_panel_visible_; }

private:
    enum DevPage : int {
        Password = 0,
        Main = 1,
    };

    void rebuildLabels();
    ftxui::Element renderPassword() const;
    ftxui::Element renderMain() const;
    ftxui::Element renderOverlay() const;
    ftxui::Element renderDebugPanel() const;
    void resetUiState();
    void rebuildDestinationOptions();
    void openDebugPanel();
    void closeDebugPanel();

    GameEngine& engine_;
    UiLanguage& language_;
    bool& visible_;
    std::function<void()> on_close_;

    int page_ = Password;
    std::string password_input_;
    std::string status_line_;
    bool unlocked_ = false;
    bool debug_panel_visible_ = false;

    int difficulty_index_ = 1;
    int ship_index_ = 0;

    int destination_index_ = 0;
    std::vector<std::string> destination_labels_;
    std::vector<std::pair<int, int>> destination_targets_;

    int encounter_index_ = 0;
    std::vector<std::string> encounter_labels_;
    std::vector<std::string> encounter_ids_;
    std::vector<std::string> encounter_payloads_;

    int enemy_index_ = 0;
    std::vector<std::string> enemy_labels_;
    std::vector<std::string> enemy_ids_;

    int battle_node_index_ = 0;
    std::vector<std::string> battle_node_labels_;

    int card_index_ = 0;
    std::vector<std::string> card_labels_;
    std::vector<std::string> card_ids_;

    int debug_card_index_ = 0;
    std::vector<std::string> debug_card_labels_;

    int ending_index_ = 0;
    std::vector<std::string> ending_labels_;

    std::string title_label_;
    std::string password_prompt_label_;
    std::string password_confirm_label_;
    std::string password_cancel_label_;
    std::string password_error_label_;
    std::string close_label_;
    std::string modal_exit_dev_label_;
    std::string section_run_;
    std::string section_destination_;
    std::string section_encounter_;
    std::string section_battle_;
    std::string section_cards_;
    std::string section_ending_;
    std::string label_difficulty_;
    std::string label_character_;
    std::string label_init_run_;
    std::string label_jump_;
    std::string label_trigger_encounter_;
    std::string label_start_battle_;
    std::string label_skip_battle_;
    std::string label_add_to_hand_;
    std::string label_add_to_draw_;
    std::string label_play_top_;
    std::string label_show_ending_;
    std::string label_return_map_;
    std::string label_status_;

    std::string label_floating_button_;
    std::string label_floating_exit_dev_;
    std::string label_debug_panel_title_;
    std::string label_debug_close_;
    std::string label_debug_exit_dev_;
    std::string label_debug_reload_event_;
    std::string label_debug_section_stats_;
    std::string label_debug_section_cards_;
    std::string label_debug_section_misc_;
    std::string label_debug_hp_;
    std::string label_debug_shield_;
    std::string label_debug_energy_;
    std::string label_debug_heat_;
    std::string label_debug_scrap_;
    std::string label_debug_add_to_hand_;
    std::string label_debug_skip_battle_;
    std::string label_debug_return_map_;
    std::string label_inc_;
    std::string label_dec_;
    std::string label_inc5_;
    std::string label_dec5_;
    std::string label_inc50_;
    std::string label_dec50_;

    std::vector<std::string> difficulty_entries_;
    std::vector<std::string> ship_entries_;
    std::vector<std::string> ship_ids_;

    // Components for the password / options modal.
    ftxui::Component password_input_component_;
    ftxui::Component password_confirm_button_;
    ftxui::Component password_cancel_button_;

    ftxui::Component difficulty_toggle_;
    ftxui::Component ship_toggle_;
    ftxui::Component init_run_button_;
    ftxui::Component destination_menu_;
    ftxui::Component jump_button_;
    ftxui::Component encounter_menu_;
    ftxui::Component trigger_encounter_button_;
    ftxui::Component battle_node_toggle_;
    ftxui::Component enemy_menu_;
    ftxui::Component start_battle_button_;
    ftxui::Component skip_battle_button_;
    ftxui::Component card_menu_;
    ftxui::Component add_to_hand_button_;
    ftxui::Component add_to_draw_button_;
    ftxui::Component play_top_button_;
    ftxui::Component ending_menu_;
    ftxui::Component show_ending_button_;
    ftxui::Component return_map_button_;
    ftxui::Component close_button_;
    ftxui::Component modal_exit_dev_button_;

    ftxui::Component password_page_;
    ftxui::Component main_page_;
    ftxui::Component pages_;
    ftxui::Component root_;

    // Components for the persistent debug overlay (floating button + panel).
    int overlay_page_ = 0;
    ftxui::Component floating_button_;
    ftxui::Component exit_dev_button_;
    ftxui::Component dbg_hp_dec_, dbg_hp_inc_, dbg_hp_dec5_, dbg_hp_inc5_;
    ftxui::Component dbg_shield_dec_, dbg_shield_inc_, dbg_shield_dec5_, dbg_shield_inc5_;
    ftxui::Component dbg_energy_dec_, dbg_energy_inc_;
    ftxui::Component dbg_heat_dec_, dbg_heat_inc_, dbg_heat_dec5_, dbg_heat_inc5_;
    ftxui::Component dbg_scrap_dec_, dbg_scrap_inc_, dbg_scrap_dec50_, dbg_scrap_inc50_;
    ftxui::Component dbg_card_menu_;
    ftxui::Component dbg_add_to_hand_button_;
    ftxui::Component dbg_reload_event_button_;
    ftxui::Component dbg_skip_battle_button_;
    ftxui::Component dbg_return_map_button_;
    ftxui::Component dbg_close_button_;
    ftxui::Component dbg_exit_dev_button_;
    ftxui::Component debug_panel_container_;
    ftxui::Component floating_bar_;
    ftxui::Component overlay_interactive_;
    ftxui::Component overlay_root_;
};

}  // namespace dev_mode

#endif
