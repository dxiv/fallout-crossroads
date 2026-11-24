#pragma once

#include <filesystem>
#include <string>

namespace crossroads {

struct CapitalTravelResult {
  bool ok = false;
  bool already_present = false;
  std::string summary;
  std::string detail;
};

// Appends Pip-Boy quest + Doc Mitchell gate door + world-map fast-travel marker
// into CrossroadsFoothold.esp (idempotent). No console required in-game.
CapitalTravelResult append_capital_pipboy_travel(const std::filesystem::path& foothold_esp);

}  // namespace crossroads
