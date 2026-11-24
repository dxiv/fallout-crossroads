#include "cell_import.hpp"
#include <cstdint>

#include "esm_index.hpp"
#include "log.hpp"

#include <cstring>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace crossroads {
namespace {

constexpr std::uint32_t kNewIdBase = 0x01002000;

const char* kPreferredCells[] = {
    "Vault101a",
    "Vault101b",
    "Vault101Oasis",
    "MegatonPlayerHouse",
    "MegatonCommonHouse",
    "TenpennySuitePlayer",
    "RivetCityUpperDeck",
    "AAAhome",
};

bool read_exact(std::ifstream& in, void* dst, std::size_t n) {
  in.read(reinterpret_cast<char*>(dst), static_cast<std::streamsize>(n));
  return static_cast<std::size_t>(in.gcount()) == n;
}

void append_bytes(std::vector<char>& out, const void* data, std::size_t n) {
  const char* p = static_cast<const char*>(data);
  out.insert(out.end(), p, p + n);
}
void append_u16(std::vector<char>& out, std::uint16_t v) { append_bytes(out, &v, 2); }
void append_u32(std::vector<char>& out, std::uint32_t v) { append_bytes(out, &v, 4); }
void append_f32(std::vector<char>& out, float v) { append_bytes(out, &v, 4); }
void append_zstring(std::vector<char>& out, const std::string& s) {
  out.insert(out.end(), s.begin(), s.end());
  out.push_back('\0');
}

void remap_formids_in_payload(std::vector<char>& data,
                              const std::unordered_map<std::uint32_t, std::uint32_t>& map) {
  if (data.size() < 4 || map.empty()) {
    return;
  }
  for (std::size_t i = 0; i + 4 <= data.size(); ++i) {
    std::uint32_t v = 0;
    std::memcpy(&v, data.data() + i, 4);
    const auto it = map.find(v);
    if (it != map.end()) {
      std::memcpy(data.data() + i, &it->second, 4);
    }
  }
}

void append_record(std::vector<char>& out, const EsmRecord& rec, std::uint32_t new_id,
                   const std::unordered_map<std::uint32_t, std::uint32_t>& map) {
  auto payload = rec.data;
  remap_formids_in_payload(payload, map);
  append_bytes(out, rec.type, 4);
  append_u32(out, static_cast<std::uint32_t>(payload.size()));
  append_u32(out, rec.flags);
  append_u32(out, new_id);
  append_u32(out, 0);
  out.insert(out.end(), payload.begin(), payload.end());
}

std::vector<char> wrap_grup(std::uint32_t label, std::int32_t group_type, const std::vector<char>& body) {
  // FO3/FNV GRUP header is 24 bytes (includes version fields). Size includes this header.
  std::vector<char> out;
  const std::uint32_t size = static_cast<std::uint32_t>(24 + body.size());
  append_bytes(out, "GRUP", 4);
  append_u32(out, size);
  append_u32(out, label);
  append_u32(out, static_cast<std::uint32_t>(group_type));
  append_u16(out, 0);  // stamp
  append_u16(out, 0);  // unknown
  append_u16(out, 0);  // version
  append_u16(out, 0);  // unknown2
  out.insert(out.end(), body.begin(), body.end());
  return out;
}

std::vector<char> build_tes4(std::uint32_t next_id) {
  std::vector<char> data;
  std::vector<char> hedr;
  append_f32(hedr, 1.0f);
  append_u32(hedr, 64);
  append_u32(hedr, next_id);
  // HEDR sub
  append_bytes(data, "HEDR", 4);
  append_u16(data, static_cast<std::uint16_t>(hedr.size()));
  data.insert(data.end(), hedr.begin(), hedr.end());

  auto put_str = [&](const char* type, const std::string& s) {
    std::vector<char> payload;
    append_zstring(payload, s);
    append_bytes(data, type, 4);
    append_u16(data, static_cast<std::uint16_t>(payload.size()));
    data.insert(data.end(), payload.begin(), payload.end());
  };
  put_str("CNAM", "dxiv");
  put_str("SNAM", "Fallout: Crossroads imported F3 foothold cell");
  put_str("MAST", "FalloutNV.esm");
  std::uint64_t zero = 0;
  append_bytes(data, "DATA", 4);
  append_u16(data, 8);
  append_bytes(data, &zero, 8);
  return data;
}

std::vector<std::uint32_t> collect_cell_child_ref_ids(const EsmIndex& index, std::uint32_t cell_id) {
  std::vector<std::uint32_t> refs;
  std::ifstream in(index.esm_path, std::ios::binary);
  if (!in) {
    return refs;
  }

  std::vector<std::uint64_t> group_ends;
  std::vector<std::int32_t> group_types;
  std::vector<std::uint32_t> group_labels;
  bool in_target_cell_tree = false;
  std::uint64_t target_end = 0;

  while (in) {
    while (!group_ends.empty() && static_cast<std::uint64_t>(in.tellg()) >= group_ends.back()) {
      if (!group_ends.empty() && target_end != 0 && group_ends.back() == target_end) {
        in_target_cell_tree = false;
        target_end = 0;
      }
      group_ends.pop_back();
      if (!group_types.empty()) {
        group_types.pop_back();
      }
      if (!group_labels.empty()) {
        group_labels.pop_back();
      }
    }

    char type[4]{};
    if (!read_exact(in, type, 4)) {
      break;
    }
    if (type[0] == 'G' && type[1] == 'R' && type[2] == 'U' && type[3] == 'P') {
      std::uint32_t group_size = 0;
      std::uint32_t label = 0;
      std::int32_t gtype = 0;
      std::uint16_t stamp = 0, unk = 0;
      if (!read_exact(in, &group_size, 4) || !read_exact(in, &label, 4) || !read_exact(in, &gtype, 4) ||
          !read_exact(in, &stamp, 2) || !read_exact(in, &unk, 2)) {
        break;
      }
      if (group_size < 24) {
        break;
      }
      const auto header_end = static_cast<std::uint64_t>(in.tellg());
      const auto end = header_end - 24 + group_size;
      group_ends.push_back(end);
      group_types.push_back(gtype);
      group_labels.push_back(label);
      if (gtype == 6 && label == cell_id) {
        in_target_cell_tree = true;
        target_end = end;
      }
      continue;
    }

    std::uint32_t data_size = 0, flags = 0, form_id = 0, vc = 0;
    if (!read_exact(in, &data_size, 4) || !read_exact(in, &flags, 4) || !read_exact(in, &form_id, 4) ||
        !read_exact(in, &vc, 4)) {
      break;
    }
    if (in_target_cell_tree) {
      const bool is_ref = (type[0] == 'R' && type[1] == 'E' && type[2] == 'F' && type[3] == 'R') ||
                          (type[0] == 'A' && type[1] == 'C' && type[2] == 'H' && type[3] == 'R') ||
                          (type[0] == 'A' && type[1] == 'C' && type[2] == 'R' && type[3] == 'E');
      if (is_ref) {
        refs.push_back(form_id);
      }
    }
    in.seekg(data_size, std::ios::cur);
  }
  return refs;
}

std::uint32_t choose_cell(const EsmIndex& index, std::string& edid_out) {
  for (const char* pref : kPreferredCells) {
    const auto it = index.interior_cells_by_edid.find(pref);
    if (it != index.interior_cells_by_edid.end()) {
      edid_out = pref;
      return it->second;
    }
  }
  // Prefer any EDID containing Vault/Megaton/Tenpenny/Rivet/Capital/Wasteland.
  for (const auto& [edid, id] : index.interior_cells_by_edid) {
    if (edid.find("Vault") != std::string::npos || edid.find("Megaton") != std::string::npos ||
        edid.find("Tenpenny") != std::string::npos || edid.find("Rivet") != std::string::npos ||
        edid.find("Capital") != std::string::npos) {
      edid_out = edid;
      return id;
    }
  }
  if (!index.interior_cells_by_edid.empty()) {
    const auto it = index.interior_cells_by_edid.begin();
    edid_out = it->first;
    return it->second;
  }
  return 0;
}

bool is_copyable_base_type(const char type[4]) {
  static const char* ok[] = {"STAT", "MSTT", "ACTI", "DOOR", "FURN", "CONT", "LIGH", "TREE",
                             "ASPC", "IDLM", "SCOL", "TACT", "TERM", "ARMO", "WEAP", "MISC",
                             "KEYM", "ALCH", "NOTE", "BOOK", "AMMO", "NPC_", "CREA", nullptr};
  for (int i = 0; ok[i]; ++i) {
    if (std::memcmp(type, ok[i], 4) == 0) {
      return true;
    }
  }
  return false;
}

}  // namespace

CellImportResult import_capital_foothold_cell(const std::filesystem::path& f3_install,
                                              const std::filesystem::path& out_esp) {
  CellImportResult result;
  const EsmIndex index = build_fallout3_index(f3_install);
  if (!index.ok) {
    result.summary = index.summary;
    result.detail = index.detail;
    return result;
  }

  std::string edid;
  const std::uint32_t cell_id = choose_cell(index, edid);
  if (!cell_id) {
    result.summary = "No interior CELL found in Fallout3.esm";
    return result;
  }
  result.cell_editor_id = edid;
  result.source_formid = cell_id;

  EsmRecord cell;
  if (!load_record(index, cell_id, cell)) {
    result.summary = "Failed to load chosen CELL";
    return result;
  }

  const auto ref_ids = collect_cell_child_ref_ids(index, cell_id);
  std::unordered_set<std::uint32_t> needed_bases;
  std::vector<EsmRecord> refs;
  refs.reserve(ref_ids.size());
  for (std::uint32_t rid : ref_ids) {
    EsmRecord refr;
    if (!load_record(index, rid, refr)) {
      continue;
    }
    refs.push_back(std::move(refr));
    const auto base = read_refr_base_formid(refs.back());
    if (base) {
      needed_bases.insert(base);
    }
  }

  // Cap refs for a manageable first foothold ESP.
  constexpr std::size_t kMaxRefs = 400;
  if (refs.size() > kMaxRefs) {
    refs.resize(kMaxRefs);
    log::warn("Truncated imported refs to " + std::to_string(kMaxRefs) + " for Phase 1c spike");
  }

  std::vector<EsmRecord> bases;
  for (std::uint32_t bid : needed_bases) {
    EsmRecord base;
    if (!load_record(index, bid, base)) {
      continue;
    }
    if (!is_copyable_base_type(base.type)) {
      continue;
    }
    bases.push_back(std::move(base));
  }

  std::unordered_map<std::uint32_t, std::uint32_t> remap;
  std::uint32_t next = kNewIdBase;
  auto take = [&](std::uint32_t old_id) {
    if (!remap.count(old_id)) {
      remap[old_id] = next++;
    }
    return remap[old_id];
  };
  take(cell_id);
  for (const auto& b : bases) {
    take(b.form_id);
  }
  for (const auto& r : refs) {
    take(r.form_id);
  }

  std::vector<char> file;
  // TES4
  {
    const auto tes4 = build_tes4(next);
    append_bytes(file, "TES4", 4);
    append_u32(file, static_cast<std::uint32_t>(tes4.size()));
    append_u32(file, 0);
    append_u32(file, 0);
    append_u32(file, 0);
    file.insert(file.end(), tes4.begin(), tes4.end());
  }

  for (const auto& b : bases) {
    append_record(file, b, remap[b.form_id], remap);
  }
  append_record(file, cell, remap[cell_id], remap);

  // Persistent-style child group containing all imported refs (type 8 under type 6).
  std::vector<char> persistent_body;
  for (const auto& r : refs) {
    append_record(persistent_body, r, remap[r.form_id], remap);
  }
  const auto persistent = wrap_grup(remap[cell_id], 8, persistent_body);
  const auto children = wrap_grup(remap[cell_id], 6, persistent);
  file.insert(file.end(), children.begin(), children.end());

  std::error_code ec;
  std::filesystem::create_directories(out_esp.parent_path(), ec);
  std::ofstream out(out_esp, std::ios::binary | std::ios::trunc);
  if (!out) {
    result.summary = "Failed to write imported cell ESP";
    result.detail = out_esp.string();
    return result;
  }
  out.write(file.data(), static_cast<std::streamsize>(file.size()));

  result.ok = true;
  result.esp_path = out_esp;
  result.base_objects = bases.size();
  result.refs = refs.size();
  result.summary = "Imported F3 cell " + edid;
  std::ostringstream detail;
  detail << "source form 0x" << std::hex << cell_id << std::dec << " | bases " << bases.size() << " | refs "
         << refs.size() << " | " << out_esp.string();
  result.detail = detail.str();
  log::info(result.summary + " — " + result.detail);
  return result;
}

}  // namespace crossroads
