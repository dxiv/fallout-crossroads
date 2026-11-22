#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace crossroads {

struct EsmRecord {
  char type[4]{};
  std::uint32_t flags = 0;
  std::uint32_t form_id = 0;
  std::vector<char> data;
};

struct EsmRecordLoc {
  char type[4]{};
  std::uint32_t flags = 0;
  std::uint32_t form_id = 0;
  std::uint64_t payload_offset = 0;
  std::uint32_t payload_size = 0;
};

struct EsmIndex {
  bool ok = false;
  std::string summary;
  std::string detail;
  std::filesystem::path esm_path;
  std::unordered_map<std::uint32_t, EsmRecordLoc> locs;
  std::unordered_map<std::string, std::uint32_t> interior_cells_by_edid;
};

EsmIndex build_fallout3_index(const std::filesystem::path& install_root);
bool load_record(const EsmIndex& index, std::uint32_t form_id, EsmRecord& out);
std::string record_edid(const EsmRecord& rec);
bool record_is_interior_cell(const EsmRecord& rec);
std::uint32_t read_refr_base_formid(const EsmRecord& refr);

}  // namespace crossroads
