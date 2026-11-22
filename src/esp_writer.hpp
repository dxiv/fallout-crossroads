#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace crossroads {

struct EspWriteResult {
  bool ok = false;
  std::string summary;
  std::string detail;
  std::filesystem::path path;
};

// Writes a minimal interior CELL plugin that depends on FalloutNV.esm only.
EspWriteResult write_foothold_esp(const std::filesystem::path& out_esp,
                                  const std::string& cell_editor_id);

}  // namespace crossroads
