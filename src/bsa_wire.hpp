#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace crossroads {

struct BsaWireResult {
  bool ok = false;
  std::string summary;
  std::string detail;
  std::vector<std::string> linked_names;
};

// Junction/symlink BYO Fallout 3 BSAs into the FNV Data folder under CR_FO3_*
// names and append those names to FalloutNV.ini archive lists when possible.
// Only call when Crossroads actually imported Capital content that needs FO3 assets.
BsaWireResult wire_fallout3_bsas_into_fnv(const std::filesystem::path& f3_install,
                                          const std::filesystem::path& fnv_install);

// Remove CR_FO3_* archive entries from FalloutNV.ini and delete CR_FO3_* links
// under FNV Data. Safe to call when a prior Launch wired BSAs without imported content.
BsaWireResult unwire_fallout3_bsas_from_fnv(const std::filesystem::path& fnv_install);

}  // namespace crossroads
