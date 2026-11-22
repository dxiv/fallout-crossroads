#include "world_import.hpp"

#include "esm_index.hpp"
#include "log.hpp"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace crossroads {
namespace {

constexpr std::uint32_t kNewIdBase = 0x01004000;
constexpr std::size_t kMaxExteriorCells = 9;
constexpr std::size_t kMaxRefsTotal = 800;

const char* kPreferredWorlds[] = {"Wasteland", "WastelandMaster", "CapitalWasteland", "Worldspace"};
const char* kPreferredInteriors[] = {"Vault101a", "Vault101b", "MegatonPlayerHouse", "TenpennySuitePlayer",
                                     "RivetCityUpperDeck"};

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
  std::vector<char> out;
  const std::uint32_t size = static_cast<std::uint32_t>(24 + body.size());
  append_bytes(out, "GRUP", 4);
  append_u32(out, size);
  append_u32(out, label);
  append_u32(out, static_cast<std::uint32_t>(group_type));
  append_u16(out, 0);
  append_u16(out, 0);
  out.insert(out.end(), body.begin(), body.end());
  return out;
}

std::vector<char> build_tes4(std::uint32_t next_id) {
  std::vector<char> data;
  std::vector<char> hedr;
  append_f32(hedr, 1.0f);
  append_u32(hedr, 128);
  append_u32(hedr, next_id);
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
  put_str("SNAM", "Fallout: Crossroads Phase 1 Capital pack");
  put_str("MAST", "FalloutNV.esm");
  std::uint64_t zero = 0;
  append_bytes(data, "DATA", 4);
  append_u16(data, 8);
  append_bytes(data, &zero, 8);
  return data;
}

bool is_copyable_base_type(const char type[4]) {
  static const char* ok[] = {"STAT", "MSTT", "ACTI", "DOOR", "FURN", "CONT", "LIGH", "TREE", "ASPC",
                             "IDLM", "SCOL", "TACT", "TERM", "ARMO", "WEAP", "MISC", "KEYM", "ALCH",
                             "NOTE", "BOOK", "AMMO", "NPC_", "CREA", "TXST", "LAND", nullptr};
  for (int i = 0; ok[i]; ++i) {
    if (std::memcmp(type, ok[i], 4) == 0) {
      return true;
    }
  }
  return false;
}

bool read_xclc(const EsmRecord& cell, int& x, int& y) {
  const auto& data = cell.data;
  std::size_t i = 0;
  while (i + 6 <= data.size()) {
    const char* st = data.data() + i;
    const std::uint16_t sz = *reinterpret_cast<const std::uint16_t*>(data.data() + i + 4);
    i += 6;
    if (i + sz > data.size()) {
      break;
    }
    if (st[0] == 'X' && st[1] == 'C' && st[2] == 'L' && st[3] == 'C' && sz >= 8) {
      std::memcpy(&x, data.data() + i, 4);
      std::memcpy(&y, data.data() + i + 4, 4);
      return true;
    }
    i += sz;
  }
  return false;
}

struct ChildBag {
  std::vector<std::uint32_t> refs;
  std::vector<std::uint32_t> lands;
};

std::unordered_map<std::uint32_t, ChildBag> collect_all_cell_children(const EsmIndex& index) {
  std::unordered_map<std::uint32_t, ChildBag> out;
  std::ifstream in(index.esm_path, std::ios::binary);
  if (!in) {
    return out;
  }

  struct GroupFrame {
    std::uint64_t end = 0;
    std::int32_t type = 0;
    std::uint32_t label = 0;
  };
  std::vector<GroupFrame> groups;
  std::uint32_t active_cell = 0;

  auto refresh_active = [&]() {
    active_cell = 0;
    for (auto it = groups.rbegin(); it != groups.rend(); ++it) {
      if (it->type == 6) {
        active_cell = it->label;
        break;
      }
    }
  };

  while (in) {
    while (!groups.empty() && static_cast<std::uint64_t>(in.tellg()) >= groups.back().end) {
      groups.pop_back();
      refresh_active();
    }

    char type[4]{};
    if (!read_exact(in, type, 4)) {
      break;
    }
    if (std::memcmp(type, "GRUP", 4) == 0) {
      std::uint32_t group_size = 0, label = 0;
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
      groups.push_back(GroupFrame{header_end - 24 + group_size, gtype, label});
      refresh_active();
      continue;
    }

    std::uint32_t data_size = 0, flags = 0, form_id = 0, vc = 0;
    if (!read_exact(in, &data_size, 4) || !read_exact(in, &flags, 4) || !read_exact(in, &form_id, 4) ||
        !read_exact(in, &vc, 4)) {
      break;
    }

    if (active_cell != 0) {
      if (std::memcmp(type, "REFR", 4) == 0 || std::memcmp(type, "ACHR", 4) == 0 ||
          std::memcmp(type, "ACRE", 4) == 0) {
        out[active_cell].refs.push_back(form_id);
      } else if (std::memcmp(type, "LAND", 4) == 0) {
        out[active_cell].lands.push_back(form_id);
      }
    }
    in.seekg(data_size, std::ios::cur);
  }
  return out;
}

struct ExtCellInfo {
  std::uint32_t form_id = 0;
  int gx = 0;
  int gy = 0;
  std::size_t ref_count = 0;
};

std::vector<std::uint32_t> list_world_exterior_cells(const EsmIndex& index, std::uint32_t wrld_id) {
  std::vector<std::uint32_t> cells;
  std::ifstream in(index.esm_path, std::ios::binary);
  if (!in) {
    return cells;
  }
  std::vector<std::uint64_t> group_ends;
  bool in_world = false;
  std::uint64_t world_end = 0;

  while (in) {
    while (!group_ends.empty() && static_cast<std::uint64_t>(in.tellg()) >= group_ends.back()) {
      if (!group_ends.empty() && world_end != 0 && group_ends.back() == world_end) {
        in_world = false;
        world_end = 0;
      }
      group_ends.pop_back();
    }
    char type[4]{};
    if (!read_exact(in, type, 4)) {
      break;
    }
    if (std::memcmp(type, "GRUP", 4) == 0) {
      std::uint32_t group_size = 0, label = 0;
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
      if (gtype == 1 && label == wrld_id) {
        in_world = true;
        world_end = end;
      }
      continue;
    }
    std::uint32_t data_size = 0, flags = 0, form_id = 0, vc = 0;
    if (!read_exact(in, &data_size, 4) || !read_exact(in, &flags, 4) || !read_exact(in, &form_id, 4) ||
        !read_exact(in, &vc, 4)) {
      break;
    }
    if (in_world && std::memcmp(type, "CELL", 4) == 0) {
      cells.push_back(form_id);
    }
    in.seekg(data_size, std::ios::cur);
  }
  return cells;
}

std::uint32_t find_world(const EsmIndex& index, std::string& edid_out) {
  // Scan WRLD records by loading candidates from locs.
  for (const char* pref : kPreferredWorlds) {
    for (const auto& [id, loc] : index.locs) {
      if (std::memcmp(loc.type, "WRLD", 4) != 0) {
        continue;
      }
      EsmRecord rec;
      if (!load_record(index, id, rec)) {
        continue;
      }
      const auto edid = record_edid(rec);
      if (edid == pref) {
        edid_out = edid;
        return id;
      }
    }
  }
  for (const auto& [id, loc] : index.locs) {
    if (std::memcmp(loc.type, "WRLD", 4) != 0) {
      continue;
    }
    EsmRecord rec;
    if (!load_record(index, id, rec)) {
      continue;
    }
    const auto edid = record_edid(rec);
    if (edid.find("Wasteland") != std::string::npos || edid.find("Capital") != std::string::npos) {
      edid_out = edid;
      return id;
    }
  }
  return 0;
}

std::uint32_t find_interior(const EsmIndex& index, std::string& edid_out) {
  for (const char* pref : kPreferredInteriors) {
    const auto it = index.interior_cells_by_edid.find(pref);
    if (it != index.interior_cells_by_edid.end()) {
      edid_out = pref;
      return it->second;
    }
  }
  for (const auto& [edid, id] : index.interior_cells_by_edid) {
    if (edid.find("Vault") != std::string::npos || edid.find("Megaton") != std::string::npos) {
      edid_out = edid;
      return id;
    }
  }
  if (!index.interior_cells_by_edid.empty()) {
    edid_out = index.interior_cells_by_edid.begin()->first;
    return index.interior_cells_by_edid.begin()->second;
  }
  return 0;
}

}  // namespace

Phase1ImportResult import_phase1_capital_pack(const std::filesystem::path& f3_install,
                                              const std::filesystem::path& out_esp) {
  Phase1ImportResult result;
  const EsmIndex index = build_fallout3_index(f3_install);
  if (!index.ok) {
    result.summary = index.summary;
    result.detail = index.detail;
    return result;
  }

  const auto children = collect_all_cell_children(index);

  std::string interior_edid;
  const std::uint32_t interior_id = find_interior(index, interior_edid);
  std::string world_edid;
  const std::uint32_t world_id = find_world(index, world_edid);

  std::vector<ExtCellInfo> exterior_pick;
  if (world_id) {
    auto cell_ids = list_world_exterior_cells(index, world_id);
    for (std::uint32_t cid : cell_ids) {
      EsmRecord cell;
      if (!load_record(index, cid, cell) || record_is_interior_cell(cell)) {
        continue;
      }
      ExtCellInfo info;
      info.form_id = cid;
      read_xclc(cell, info.gx, info.gy);
      const auto it = children.find(cid);
      info.ref_count = (it == children.end()) ? 0 : it->second.refs.size();
      exterior_pick.push_back(info);
    }
    // Prefer denser cells near the densest cell (proxy for a landmark pocket).
    if (!exterior_pick.empty()) {
      auto best = exterior_pick.begin();
      for (auto it = exterior_pick.begin(); it != exterior_pick.end(); ++it) {
        if (it->ref_count > best->ref_count) {
          best = it;
        }
      }
      const int cx = best->gx;
      const int cy = best->gy;
      std::vector<ExtCellInfo> near;
      for (const auto& e : exterior_pick) {
        if (std::abs(e.gx - cx) <= 1 && std::abs(e.gy - cy) <= 1) {
          near.push_back(e);
        }
      }
      if (near.empty()) {
        near.push_back(*best);
      }
      if (near.size() > kMaxExteriorCells) {
        near.resize(kMaxExteriorCells);
      }
      exterior_pick.swap(near);
      result.cow_hint = "cow " + world_edid + " " + std::to_string(cx) + " " + std::to_string(cy);
    }
  }

  std::unordered_set<std::uint32_t> needed_bases;
  std::vector<EsmRecord> base_recs;
  std::vector<EsmRecord> all_refs;
  std::vector<EsmRecord> all_lands;
  std::vector<EsmRecord> exterior_cells;
  EsmRecord interior_cell;
  EsmRecord world_rec;
  bool have_interior = false;
  bool have_world = false;

  auto absorb_children = [&](std::uint32_t cell_id) {
    const auto it = children.find(cell_id);
    if (it == children.end()) {
      return;
    }
    for (std::uint32_t rid : it->second.refs) {
      if (all_refs.size() >= kMaxRefsTotal) {
        break;
      }
      EsmRecord r;
      if (!load_record(index, rid, r)) {
        continue;
      }
      const auto base = read_refr_base_formid(r);
      if (base) {
        needed_bases.insert(base);
      }
      all_refs.push_back(std::move(r));
    }
    for (std::uint32_t lid : it->second.lands) {
      EsmRecord land;
      if (load_record(index, lid, land)) {
        all_lands.push_back(std::move(land));
      }
    }
  };

  if (interior_id && load_record(index, interior_id, interior_cell)) {
    have_interior = true;
    result.interior_edid = interior_edid;
    result.coc_hint = "coc " + interior_edid;
    absorb_children(interior_id);
  }
  if (world_id && load_record(index, world_id, world_rec)) {
    have_world = true;
    result.world_edid = world_edid;
  }
  for (const auto& e : exterior_pick) {
    EsmRecord cell;
    if (!load_record(index, e.form_id, cell)) {
      continue;
    }
    exterior_cells.push_back(std::move(cell));
    absorb_children(e.form_id);
  }
  result.exterior_cells = exterior_cells.size();

  if (!have_interior && exterior_cells.empty()) {
    result.summary = "No Capital interior or exterior cells imported";
    result.detail = "Fallout3.esm indexed but no usable CELL/WRLD slice found";
    return result;
  }

  for (std::uint32_t bid : needed_bases) {
    EsmRecord base;
    if (!load_record(index, bid, base)) {
      continue;
    }
    if (!is_copyable_base_type(base.type)) {
      continue;
    }
    base_recs.push_back(std::move(base));
  }

  std::unordered_map<std::uint32_t, std::uint32_t> remap;
  std::uint32_t next = kNewIdBase;
  auto take = [&](std::uint32_t old_id) {
    if (!remap.count(old_id)) {
      remap[old_id] = next++;
    }
    return remap[old_id];
  };
  if (have_world) {
    take(world_id);
  }
  if (have_interior) {
    take(interior_id);
  }
  for (const auto& c : exterior_cells) {
    take(c.form_id);
  }
  for (const auto& b : base_recs) {
    take(b.form_id);
  }
  for (const auto& r : all_refs) {
    take(r.form_id);
  }
  for (const auto& l : all_lands) {
    take(l.form_id);
  }

  std::vector<char> file;
  {
    const auto tes4 = build_tes4(next);
    append_bytes(file, "TES4", 4);
    append_u32(file, static_cast<std::uint32_t>(tes4.size()));
    append_u32(file, 0);
    append_u32(file, 0);
    append_u32(file, 0);
    file.insert(file.end(), tes4.begin(), tes4.end());
  }
  for (const auto& b : base_recs) {
    append_record(file, b, remap[b.form_id], remap);
  }

  if (have_world) {
    append_record(file, world_rec, remap[world_id], remap);
    std::vector<char> world_body;
    for (const auto& cell : exterior_cells) {
      append_record(world_body, cell, remap[cell.form_id], remap);
      std::vector<char> pers;
      std::vector<char> temp;
      const auto ch = children.find(cell.form_id);
      if (ch != children.end()) {
        for (std::uint32_t rid : ch->second.refs) {
          const auto itmap = remap.find(rid);
          if (itmap == remap.end()) {
            continue;
          }
          // find record in all_refs
          for (const auto& r : all_refs) {
            if (r.form_id == rid) {
              append_record(pers, r, itmap->second, remap);
              break;
            }
          }
        }
        for (std::uint32_t lid : ch->second.lands) {
          const auto itmap = remap.find(lid);
          if (itmap == remap.end()) {
            continue;
          }
          for (const auto& l : all_lands) {
            if (l.form_id == lid) {
              append_record(temp, l, itmap->second, remap);
              break;
            }
          }
        }
      }
      auto body = wrap_grup(remap[cell.form_id], 8, pers);
      auto body2 = wrap_grup(remap[cell.form_id], 9, temp);
      std::vector<char> cell_children;
      cell_children.insert(cell_children.end(), body.begin(), body.end());
      cell_children.insert(cell_children.end(), body2.begin(), body2.end());
      auto wrapped = wrap_grup(remap[cell.form_id], 6, cell_children);
      world_body.insert(world_body.end(), wrapped.begin(), wrapped.end());
    }
    auto world_grup = wrap_grup(remap[world_id], 1, world_body);
    file.insert(file.end(), world_grup.begin(), world_grup.end());
  }

  if (have_interior) {
    append_record(file, interior_cell, remap[interior_id], remap);
    std::vector<char> pers;
    const auto ch = children.find(interior_id);
    if (ch != children.end()) {
      for (std::uint32_t rid : ch->second.refs) {
        const auto itmap = remap.find(rid);
        if (itmap == remap.end()) {
          continue;
        }
        for (const auto& r : all_refs) {
          if (r.form_id == rid) {
            append_record(pers, r, itmap->second, remap);
            break;
          }
        }
      }
    }
    auto pers_g = wrap_grup(remap[interior_id], 8, pers);
    auto kids = wrap_grup(remap[interior_id], 6, pers_g);
    file.insert(file.end(), kids.begin(), kids.end());
  }

  std::error_code ec;
  std::filesystem::create_directories(out_esp.parent_path(), ec);
  std::ofstream out(out_esp, std::ios::binary | std::ios::trunc);
  if (!out) {
    result.summary = "Failed to write Phase 1 Capital pack ESP";
    return result;
  }
  out.write(file.data(), static_cast<std::streamsize>(file.size()));

  result.ok = true;
  result.esp_path = out_esp;
  result.refs = all_refs.size();
  result.bases = base_recs.size();
  result.summary = "Phase 1 Capital pack imported";
  std::ostringstream detail;
  detail << "interior=" << (have_interior ? interior_edid : "(none)");
  detail << " world=" << (have_world ? world_edid : "(none)");
  detail << " exterior_cells=" << exterior_cells.size();
  detail << " bases=" << base_recs.size() << " refs=" << all_refs.size();
  if (!result.coc_hint.empty()) {
    detail << " | " << result.coc_hint;
  }
  if (!result.cow_hint.empty()) {
    detail << " | " << result.cow_hint;
  }
  result.detail = detail.str();
  log::info(result.summary + " — " + result.detail);
  return result;
}

}  // namespace crossroads
