#ifndef TUI_APP_H
#define TUI_APP_H

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

// Builds the app's root component from the given terminal screen and returns it to FTXUI.
ftxui::Component CreateApp(ftxui::ScreenInteractive& screen);

#endif
