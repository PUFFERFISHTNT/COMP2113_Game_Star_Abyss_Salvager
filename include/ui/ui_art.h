#ifndef UI_ART_H
#define UI_ART_H

#include <string>
#include <vector>

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/box.hpp>

#include "tui_game.h"

// what it does: Builds the animated player ship art for the battle view.
// what the inputs are: state - current run state; tick - animation frame counter.
// what the outputs are: Returns rendered player art rows (ftxui::Elements).
ftxui::Elements PlayerArt(const RunState& state, int tick);

// what it does: Builds the animated enemy art for the battle view.
// what the inputs are: state - current run state; tick - animation frame counter.
// what the outputs are: Returns rendered enemy art rows (ftxui::Elements).
ftxui::Elements EnemyArt(const RunState& state, int tick);

// what it does: Calculates a safe text width for the battle lane area.
// what the inputs are: lane_box - measured FTXUI box for the lane.
// what the outputs are: Returns the lane width hint in terminal cells (int).
int LaneWidthHint(const ftxui::Box& lane_box);

// what it does: Builds centered battle lane text rows for the current combat state.
// what the inputs are: state - current run state; tick - animation frame counter; width - target lane width.
// what the outputs are: Returns lane text rows (std::vector<std::string>).
std::vector<std::string> LaneRows(const RunState& state, int tick, int width);

#endif
