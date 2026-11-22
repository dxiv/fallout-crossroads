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
BsaWireResult wire_fallout3_bsas_into_fnv(const std::filesystem::path& f3_install,
                                          const std::filesystem::path& fnv_install);

}  // namespace crossroads
