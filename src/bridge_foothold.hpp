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
  std::string f3_worldspace_hint;
};

// Builds CrossroadsFoothold.esp, deploys it into the BYO FNV Data folder, and
// enables it in plugins.txt when possible. Uses Fallout 3 only as a BYO hint
// source (worldspace names) — the foothold CELL itself is a Crossroads-authored
// empty interior loadable by the FNV host (Phase 1b spike).
FootholdBridgeResult build_and_deploy_foothold(const std::filesystem::path& f3_install,
                                               const std::filesystem::path& fnv_install);

std::filesystem::path foothold_staging_dir();

}  // namespace crossroads
