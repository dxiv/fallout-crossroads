#pragma once

#include <filesystem>
#include <string>

namespace crossroads {

struct MojaveFootholdResult {
  bool ok = false;
  std::string summary;
  std::string detail;
  std::string cell_editor_id;   // preferred coc target in vanilla FNV
  std::string world_editor_id;  // e.g. WastelandNV
  std::string cow_hint;         // exterior grid for Mojave walks
};

// Probes BYO FalloutNV.esm for a Mojave arrival foothold (interior or known EDID).
// Does not copy Bethesda assets — only resolves editor IDs the host already loads.
MojaveFootholdResult resolve_mojave_foothold(const std::filesystem::path& fnv_install);

}  // namespace crossroads
