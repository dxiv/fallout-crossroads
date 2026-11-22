#include "plugin.hpp"

#include "log.hpp"
#include "settings.hpp"

namespace crossroads::plugin {

void on_attach() {
  log::init(log_path());
  log::info("CrossroadsPlugin attached inside FalloutNV.exe host");
  log::info("Phase 1b: look for CrossroadsFoothold.esp in Data\\");
  log::info("Console: coc CrossroadsFoothold");
  const Settings s = load_settings();
  if (!s.fallout3_install.empty()) {
    log::info("F3 install path is configured (BYO). Full Capital cell import is a later step.");
  } else {
    log::warn("No Fallout 3 path in settings; run FalloutCrossroads.exe first.");
  }
}

void on_detach() { log::info("CrossroadsPlugin detaching"); }

}  // namespace crossroads::plugin
