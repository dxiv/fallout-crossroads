#pragma once

#include "cell_import.hpp"

#include <filesystem>
#include <string>

namespace crossroads {

struct Phase1ImportResult {
  bool ok = false;
  std::string summary;
  std::string detail;
  std::filesystem::path esp_path;
  std::string interior_edid;
  std::string world_edid;
  std::string coc_hint;   // e.g. coc Vault101a
  std::string cow_hint;   // e.g. cow Wasteland -4 2
  std::size_t exterior_cells = 0;
  std::size_t refs = 0;
  std::size_t bases = 0;
};

// Phase 1 complete content pack: Capital-flavored interior + WRLD exterior slice
// from BYO Fallout3.esm into one remapped ESP.
Phase1ImportResult import_phase1_capital_pack(const std::filesystem::path& f3_install,
                                              const std::filesystem::path& out_esp);

}  // namespace crossroads
