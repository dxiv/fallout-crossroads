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

struct CapitalImportBudget {
  std::size_t exterior_radius = 1;       // 1 → ~3×3, 2 → ~5×5
  std::size_t max_exterior_cells = 9;
  std::size_t max_refs = 800;
};

// Capital-flavored interior + WRLD exterior slice from BYO Fallout3.esm.
Phase1ImportResult import_phase1_capital_pack(const std::filesystem::path& f3_install,
                                              const std::filesystem::path& out_esp);

Phase1ImportResult import_phase1_capital_pack(const std::filesystem::path& f3_install,
                                              const std::filesystem::path& out_esp,
                                              const CapitalImportBudget& budget);

}  // namespace crossroads
