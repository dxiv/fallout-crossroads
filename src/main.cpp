#include "app.hpp"
#include "crash_guard.hpp"
#include "log.hpp"
#include "settings.hpp"

#include <cstdlib>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show_command) {
  crossroads::log::init(crossroads::log_path());
  crossroads::install_crash_guard();
  return crossroads::run_app(instance, show_command);
}
