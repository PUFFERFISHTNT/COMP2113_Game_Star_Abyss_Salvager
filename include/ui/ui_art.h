#ifndef UI_ART_H
#define UI_ART_H

#include <string>
#include <vector>

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/box.hpp>

#include "tui_game.h"

ftxui::Elements PlayerArt(const RunState& state, int tick);
ftxui::Elements EnemyArt(const RunState& state, int tick);
int LaneWidthHint(const ftxui::Box& lane_box);
std::vector<std::string> LaneRows(const RunState& state, int tick, int width);

#endif
