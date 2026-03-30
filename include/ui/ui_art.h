#ifndef UI_ART_H
#define UI_ART_H

#include <string>
#include <vector>

#include <ftxui/screen/box.hpp>

#include "tui_game.h"

std::vector<std::string> PlayerArt(const RunState& state, int tick);
std::vector<std::string> EnemyArt(const RunState& state, int tick);
int LaneWidthHint(const ftxui::Box& lane_box);
std::vector<std::string> LaneRows(const RunState& state, int tick, int width);

#endif
