#ifndef TUI_APP_H
#define TUI_APP_H

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

// what it does: Creates the root FTXUI component for the Star Abyss application.
// what the inputs are: screen - interactive terminal screen used by the app.
// what the outputs are: Returns the root UI component (ftxui::Component).
ftxui::Component CreateApp(ftxui::ScreenInteractive& screen);

#endif
