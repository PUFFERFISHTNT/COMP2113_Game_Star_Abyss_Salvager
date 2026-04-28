#include <ftxui/component/screen_interactive.hpp>
#include "tui_app.h"

// Starts the fullscreen terminal app, takes no command line input, and returns the process status.
int main() {
  auto screen = ftxui::ScreenInteractive::Fullscreen();
  screen.TrackMouse(true);
  auto component = CreateApp(screen);
  screen.Loop(component);
  return 0;
}
