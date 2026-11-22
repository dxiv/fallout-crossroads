#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace crossroads {

struct SpineStub {
  std::string editor_id;
  std::string title;
};

struct SpineEspResult {
  bool ok = false;
  std::string summary;
  std::string detail;
  std::filesystem::path path;
  std::vector<SpineStub> stubs;
};

// Authors CrossroadsSpine.esp: Crossroads main-quest stub GLOBs + NOTE markers.
SpineEspResult write_spine_esp(const std::filesystem::path& out_esp);

}  // namespace crossroads
