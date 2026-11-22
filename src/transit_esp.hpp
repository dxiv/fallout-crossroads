#pragma once

#include <filesystem>
#include <string>

namespace crossroads {

struct TransitEspResult {
  bool ok = false;
  std::string summary;
  std::string detail;
  std::filesystem::path path;
  std::string plane_cell = "CR_PlaneCabin";
  std::string boat_cell = "CR_BoatCabin";
};

// Authors CrossroadsTransit.esp: plane + boat transit interiors (Crossroads-owned).
TransitEspResult write_transit_esp(const std::filesystem::path& out_esp);

}  // namespace crossroads
