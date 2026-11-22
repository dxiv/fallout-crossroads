#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace crossroads {

struct WorldspaceHit {
  std::string editor_id;
  std::uint32_t form_id = 0;
};

struct EsmProbeResult {
  bool ok = false;
  std::string summary;
  std::string detail;
  std::vector<WorldspaceHit> worldspaces;
};

// Reads the player's own Fallout3.esm (BYO). Never copies bytes into the repo.
// Returns up to max_worldspaces WRLD editor IDs for the Capital-foothold spike.
EsmProbeResult probe_fallout3_worldspaces(const std::filesystem::path& install_root,
                                          std::size_t max_worldspaces = 32);

}  // namespace crossroads
