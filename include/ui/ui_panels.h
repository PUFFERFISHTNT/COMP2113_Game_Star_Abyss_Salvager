#ifndef UI_PANELS_H
#define UI_PANELS_H

#include <string>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

#include "tui_game.h"
#include "ui/ui_localization.h"

extern const ftxui::Color kAbyss;
extern const ftxui::Color kPanel;
extern const ftxui::Color kPanelAlt;
extern const ftxui::Color kHull;
extern const ftxui::Color kShield;
extern const ftxui::Color kHeat;
extern const ftxui::Color kGold;
extern const ftxui::Color kEnemy;
extern const ftxui::Color kDim;

// what it does: Creates a button style using the requested accent color.
// what the inputs are: accent - color applied to the button border and focus state.
// what the outputs are: Returns configured button options (ftxui::ButtonOption).
ftxui::ButtonOption AccentButton(ftxui::Color accent);

// what it does: Builds the keyboard label for a battle card slot.
// what the inputs are: index - zero-based card slot index.
// what the outputs are: Returns the slot label text (std::string).
std::string BattleKeyLabel(size_t index);

// what it does: Builds a single localized description string for a card.
// what the inputs are: language - selected UI language; card - card definition to describe.
// what the outputs are: Returns the localized description text (std::string).
std::string CardDescriptionText(UiLanguage language, const CardDef& card);

// what it does: Splits a localized card description into display lines.
// what the inputs are: language - selected UI language; card - card definition to describe.
// what the outputs are: Returns description lines (std::vector<std::string>).
std::vector<std::string> CardDescriptionLines(UiLanguage language, const CardDef& card);

// what it does: Compacts card fact strings into a localized short text block.
// what the inputs are: language - selected UI language; facts - fact strings to combine.
// what the outputs are: Returns compact fact text (std::string).
std::string CompactFactText(UiLanguage language, const std::vector<std::string>& facts);

// what it does: Builds the localized scroll hint line used by long panels.
// what the inputs are: language - selected UI language.
// what the outputs are: Returns scroll hint text (std::string).
std::string ScrollHintLine(UiLanguage language);

// what it does: Renders text as a wrapped paragraph for a target width.
// what the inputs are: text - paragraph text; width_hint - preferred wrapping width.
// what the outputs are: Returns the wrapped paragraph element (ftxui::Element).
ftxui::Element WrappedParagraph(const std::string& text, int width_hint);

// what it does: Selects the accent color associated with a card.
// what the inputs are: card - card definition pointer, or nullptr for a fallback color.
// what the outputs are: Returns the card accent color (ftxui::Color).
ftxui::Color CardAccent(const CardDef* card);

// what it does: Selects the frame color associated with a card.
// what the inputs are: card - card definition pointer, or nullptr for a fallback color.
// what the outputs are: Returns the card frame color (ftxui::Color).
ftxui::Color CardFrameColor(const CardDef* card);

// what it does: Selects the background color for a card panel.
// what the inputs are: card - card definition pointer, or nullptr for a fallback color; playable - whether the card can currently be played.
// what the outputs are: Returns the card background color (ftxui::Color).
ftxui::Color CardBackground(const CardDef* card, bool playable);

// what it does: Builds localized fact lines for a card's mechanics.
// what the inputs are: language - selected UI language; card - card definition to summarize.
// what the outputs are: Returns card fact lines (std::vector<std::string>).
std::vector<std::string> CardFacts(UiLanguage language, const CardDef& card);

// what it does: Builds localized player status lines for the current run.
// what the inputs are: language - selected UI language; state - current run state.
// what the outputs are: Returns player status lines (std::vector<std::string>).
std::vector<std::string> PlayerStatusLines(UiLanguage language, const RunState& state);

// what it does: Builds localized player summary lines for battle panels.
// what the inputs are: language - selected UI language; state - current run state.
// what the outputs are: Returns player summary lines (std::vector<std::string>).
std::vector<std::string> BattlePlayerSummaryLines(UiLanguage language, const RunState& state);

// what it does: Builds localized enemy summary lines for battle panels.
// what the inputs are: language - selected UI language; state - current run state.
// what the outputs are: Returns enemy summary lines (std::vector<std::string>).
std::vector<std::string> BattleEnemySummaryLines(UiLanguage language, const RunState& state);

// what it does: Builds localized relic display lines for the current run.
// what the inputs are: language - selected UI language; engine - game engine used for relic names; state - current run state.
// what the outputs are: Returns relic lines (std::vector<std::string>).
std::vector<std::string> RelicLines(UiLanguage language, const GameEngine& engine, const RunState& state);

// what it does: Renders an owned deck card panel for service screens.
// what the inputs are: language - selected UI language; entry - owned card entry; selectable - whether the card can be chosen; hotkey - displayed shortcut number; direct_select_mode - whether click selects immediately; focused_target - whether the card is focused; purge_mode - whether purge styling is active; card_width - target panel width; upgrade_preview - optional upgraded card preview.
// what the outputs are: Returns the service deck card panel element (ftxui::Element).
ftxui::Element ServiceDeckCardPanel(UiLanguage language,
                                    const CardOwnershipEntry& entry,
                                    bool selectable,
                                    int hotkey,
                                    bool direct_select_mode,
                                    bool focused_target,
                                    bool purge_mode,
                                    int card_width,
                                    const CardDef* upgrade_preview);

// what it does: Renders a labeled gauge panel for numeric combat resources.
// what the inputs are: label - gauge label text; value - current value; maxValue - maximum value; gauge_color - filled gauge color; text_color - label and value color.
// what the outputs are: Returns the gauge panel element (ftxui::Element).
ftxui::Element GaugePanel(const std::string& label,
                          int value,
                          int maxValue,
                          ftxui::Color gauge_color,
                          ftxui::Color text_color);

// what it does: Renders a titled information panel with localized lines.
// what the inputs are: language - selected UI language; title - panel title; lines - body lines; accent - border/accent color.
// what the outputs are: Returns the information panel element (ftxui::Element).
ftxui::Element InfoPanel(UiLanguage language,
                         const std::string& title,
                         const std::vector<std::string>& lines,
                         ftxui::Color accent);

// what it does: Renders a battle entity status panel for player or enemy data.
// what the inputs are: title - entity title; hull_label - hull label text; hp - current hull value; max_hp - maximum hull value; accent - panel accent color; lines - extra status lines.
// what the outputs are: Returns the battle entity panel element (ftxui::Element).
ftxui::Element BattleEntityPanel(const std::string& title,
                                 const std::string& hull_label,
                                 int hp,
                                 int max_hp,
                                 ftxui::Color accent,
                                 const std::vector<std::string>& lines);

// what it does: Renders a compact action panel containing a title, hint, and button.
// what the inputs are: title - panel title; hint - helper text; button - action component to render; accent - panel accent color.
// what the outputs are: Returns the action panel element (ftxui::Element).
ftxui::Element ActionPanel(const std::string& title,
                           const std::string& hint,
                           const ftxui::Component& button,
                           ftxui::Color accent);

#endif
