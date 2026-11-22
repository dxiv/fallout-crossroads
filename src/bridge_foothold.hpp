#pragma once

#include <filesystem>
#include <string>

namespace crossroads {

struct FootholdBridgeResult {
  bool ok = false;
  std::string summary;
  std::string detail;
  std::filesystem::path esp_in_data;
  std::string cell_editor_id;
  std::string world_editor_id;
  std::string f3_worldspace_hint;
  std::string coc_hint;
  std::string cow_hint;
  std::size_t exterior_cells = 0;
};

// Builds CrossroadsFoothold.esp from BYO Fallout 3 (Phase 1 Capital pack:
// interior + WRLD exterior slice), deploys into FNV Data, wires BSAs, and
// enables the plugin when possible.
FootholdBridgeResult build_and_deploy_foothold(const std::filesystem::path& f3_install,
                                               const std::filesystem::path& fnv_install);

std::filesystem::path foothold_staging_dir();

}  // namespace crossroads
