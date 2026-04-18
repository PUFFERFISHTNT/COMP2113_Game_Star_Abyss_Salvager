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

ftxui::ButtonOption AccentButton(ftxui::Color accent);
std::string BattleKeyLabel(size_t index);
std::string CardDescriptionText(UiLanguage language, const CardDef& card);
std::vector<std::string> CardDescriptionLines(UiLanguage language, const CardDef& card);
std::string CompactFactText(UiLanguage language, const std::vector<std::string>& facts);
std::string ScrollHintLine(UiLanguage language);
ftxui::Element WrappedParagraph(const std::string& text, int width_hint);
ftxui::Color CardAccent(const CardDef* card);
ftxui::Color CardFrameColor(const CardDef* card);
ftxui::Color CardBackground(const CardDef* card, bool playable);
std::vector<std::string> CardFacts(UiLanguage language, const CardDef& card);
std::vector<std::string> PlayerStatusLines(UiLanguage language, const RunState& state);
std::vector<std::string> BattlePlayerSummaryLines(UiLanguage language, const RunState& state);
std::vector<std::string> BattleEnemySummaryLines(UiLanguage language, const RunState& state);
std::vector<std::string> RelicLines(UiLanguage language, const GameEngine& engine, const RunState& state);

ftxui::Element GaugePanel(const std::string& label,
                          int value,
                          int maxValue,
                          ftxui::Color gauge_color,
                          ftxui::Color text_color);
ftxui::Element InfoPanel(UiLanguage language,
                         const std::string& title,
                         const std::vector<std::string>& lines,
                         ftxui::Color accent);
ftxui::Element BattleEntityPanel(const std::string& title,
                                 const std::string& hull_label,
                                 int hp,
                                 int max_hp,
                                 ftxui::Color accent,
                                 const std::vector<std::string>& lines);
ftxui::Element ActionPanel(const std::string& title,
                           const std::string& hint,
                           const ftxui::Component& button,
                           ftxui::Color accent);

#endif
