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

// Creates button options from the accent color and returns the configured style.
ftxui::ButtonOption AccentButton(ftxui::Color accent);

// Converts a card slot index to its battle hotkey label and returns the text.
std::string BattleKeyLabel(size_t index);

// Builds localized rules text from the given card and returns the description.
std::string CardDescriptionText(UiLanguage language, const CardDef& card);

// Splits a card description for display and returns the lines.
std::vector<std::string> CardDescriptionLines(UiLanguage language, const CardDef& card);

// Joins card fact strings for the selected language and returns compact text.
std::string CompactFactText(UiLanguage language, const std::vector<std::string>& facts);

// Builds the scroll hint for the selected language and returns it.
std::string ScrollHintLine(UiLanguage language);

// Wraps the given text to the width hint and returns a paragraph element.
ftxui::Element WrappedParagraph(const std::string& text, int width_hint);

// Picks the accent color for the given card pointer and returns a safe fallback when needed.
ftxui::Color CardAccent(const CardDef* card);

// Picks the frame color for the given card pointer and returns a safe fallback when needed.
ftxui::Color CardFrameColor(const CardDef* card);

// Picks a card background from the card and playable state and returns that color.
ftxui::Color CardBackground(const CardDef* card, bool playable);

// Builds localized fact labels from the given card and returns them as lines.
std::vector<std::string> CardFacts(UiLanguage language, const CardDef& card);

// Builds map and encounter player status text from the language and run state and returns the lines.
std::vector<std::string> PlayerStatusLines(UiLanguage language, const RunState& state);

// Builds battle player summary text from the language and run state and returns the lines.
std::vector<std::string> BattlePlayerSummaryLines(UiLanguage language, const RunState& state);

// Builds battle enemy summary text from the language and run state and returns the lines.
std::vector<std::string> BattleEnemySummaryLines(UiLanguage language, const RunState& state);

// Builds relic display text from the language, engine, and run state and returns the lines.
std::vector<std::string> RelicLines(UiLanguage language, const GameEngine& engine, const RunState& state);

// Renders one service deck card from its selection state and returns the panel element.
ftxui::Element ServiceDeckCardPanel(UiLanguage language,
                                    const CardOwnershipEntry& entry,
                                    bool selectable,
                                    int hotkey,
                                    bool direct_select_mode,
                                    bool focused_target,
                                    bool purge_mode,
                                    int card_width,
                                    const CardDef* upgrade_preview);

// Renders a numeric gauge from labels, values, and colors and returns the panel element.
ftxui::Element GaugePanel(const std::string& label,
                          int value,
                          int maxValue,
                          ftxui::Color gauge_color,
                          ftxui::Color text_color);

// Renders titled information from localized lines and an accent color and returns the panel element.
ftxui::Element InfoPanel(UiLanguage language,
                         const std::string& title,
                         const std::vector<std::string>& lines,
                         ftxui::Color accent);

// Renders a battle entity from hull values, accent color, and detail lines and returns the panel element.
ftxui::Element BattleEntityPanel(const std::string& title,
                                 const std::string& hull_label,
                                 int hp,
                                 int max_hp,
                                 ftxui::Color accent,
                                 const std::vector<std::string>& lines);

// Renders a titled action block around the given button and returns the element.
ftxui::Element ActionPanel(const std::string& title,
                           const std::string& hint,
                           const ftxui::Component& button,
                           ftxui::Color accent);

#endif
