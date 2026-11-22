#include "plugin.hpp"

#include "coverage_policy.hpp"
#include "crash_guard.hpp"
#include "log.hpp"
#include "settings.hpp"
#include "transit_journey.hpp"

#include <string>

namespace crossroads::plugin {

void on_attach() {
  log::init(log_path());
  install_crash_guard();
  log::info("CrossroadsPlugin attached inside FalloutNV.exe host");
  log::info(std::string("Crossroads ") + CROSSROADS_VERSION);
  const JourneyState journey = load_journey();
  const LegPlaybook book = playbook_for(journey);
  log::info(std::string("Transit leg: ") + transit_leg_name(journey.leg) + " — " + book.title);
  log::info(book.summary);
  if (!book.console_hint.empty()) {
    log::info(std::string("Console:\n") + book.console_hint);
  }
  log::info(book.next_action);
  log::info("Plugins expected: CrossroadsFoothold.esp + CrossroadsTransit.esp + CrossroadsSpine.esp");
  const CoveragePolicy coverage = coverage_for_journey(journey);
  log::info("Hot wasteland: " + hot_wasteland_name(coverage.hot) + " — " + coverage.note);
  const Settings s = load_settings();
  if (s.fallout3_install.empty()) {
    log::warn("No Fallout 3 path in settings; run FalloutCrossroads.exe first.");
  }
}

void on_detach() { log::info("CrossroadsPlugin detaching"); }

}  // namespace crossroads::plugin
