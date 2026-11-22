#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace crossroads {

struct CellImportResult {
  bool ok = false;
  std::string summary;
  std::string detail;
  std::filesystem::path esp_path;
  std::string cell_editor_id;
  std::uint32_t source_formid = 0;
  std::size_t base_objects = 0;
  std::size_t refs = 0;
};

// Imports one real Fallout 3 interior CELL (+ refs + direct base objects) from the
// BYO ESM into a remapped ESP. Prefers Capital-related editor IDs.
CellImportResult import_capital_foothold_cell(const std::filesystem::path& f3_install,
                                              const std::filesystem::path& out_esp);

}  // namespace crossroads
