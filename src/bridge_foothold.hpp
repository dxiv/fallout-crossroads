#pragma once

#include <filesystem>
#include <string>

namespace crossroads {

struct FootholdBridgeResult {
  bool ok = false;
  std::string summary;
  std::string detail;
  std::filesystem::path esp_in_data;
  std::filesystem::path transit_esp_in_data;
  std::string cell_editor_id;
  std::string world_editor_id;
  std::string f3_worldspace_hint;
  std::string coc_hint;
  std::string cow_hint;
  std::string mojave_cell_edid;
  std::string mojave_world_edid;
  std::string plane_cell = "CR_PlaneCabin";
  std::string boat_cell = "CR_BoatCabin";
  std::size_t exterior_cells = 0;
};

// Builds Capital pack + CrossroadsTransit.esp, wires BSAs, deploys plugins, resolves Mojave foothold.
FootholdBridgeResult build_and_deploy_foothold(const std::filesystem::path& f3_install,
                                               const std::filesystem::path& fnv_install);

std::filesystem::path foothold_staging_dir();

}  // namespace crossroads
