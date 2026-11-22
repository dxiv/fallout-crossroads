#include "app.hpp"

#include <cstdlib>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show_command) {
  return crossroads::run_app(instance, show_command);
}
