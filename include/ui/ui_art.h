#ifndef UI_ART_H
#define UI_ART_H

#include <string>
#include <vector>

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/box.hpp>

#include "tui_game.h"

// Builds player ship art from the run state and animation tick and returns the rows to render.
ftxui::Elements PlayerArt(const RunState& state, int tick);

// Builds enemy art from the run state and animation tick and returns the rows to render.
ftxui::Elements EnemyArt(const RunState& state, int tick);

// Measures the reflected lane box and returns a safe width hint.
int LaneWidthHint(const ftxui::Box& lane_box);

// Builds combat lane text from state, animation tick, and width and returns the text rows.
std::vector<std::string> LaneRows(const RunState& state, int tick, int width);

#endif
