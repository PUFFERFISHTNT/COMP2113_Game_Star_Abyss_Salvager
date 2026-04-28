#include <ftxui/component/screen_interactive.hpp>
#include "tui_app.h"

// what it does: Starts the terminal application and runs the FTXUI event loop.
// what the inputs are: None.
// what the outputs are: Returns the process exit status (int).
int main() {
  auto screen = ftxui::ScreenInteractive::Fullscreen();
  screen.TrackMouse(true);
  auto component = CreateApp(screen);
  screen.Loop(component);
  return 0;
}
