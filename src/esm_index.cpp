#include "esm_index.hpp"

#include "log.hpp"

#include <cstring>
#include <fstream>

namespace crossroads {
namespace {

bool read_exact(std::ifstream& in, void* dst, std::size_t n) {
  in.read(reinterpret_cast<char*>(dst), static_cast<std::streamsize>(n));
  return static_cast<std::size_t>(in.gcount()) == n;
}

std::string four(const char t[4]) { return std::string(t, t + 4); }

std::string edid_from_payload(const std::vector<char>& data) {
  std::size_t i = 0;
  while (i + 6 <= data.size()) {
    const char* st = data.data() + i;
    const std::uint16_t sz = *reinterpret_cast<const std::uint16_t*>(data.data() + i + 4);
    i += 6;
    if (i + sz > data.size()) {
      break;
    }
    if (st[0] == 'E' && st[1] == 'D' && st[2] == 'I' && st[3] == 'D' && sz > 0) {
      std::string id(data.data() + i, data.data() + i + sz);
      while (!id.empty() && id.back() == '\0') {
        id.pop_back();
      }
      return id;
    }
    i += sz;
  }
  return {};
}

bool payload_is_interior_cell(const std::vector<char>& data) {
  std::size_t i = 0;
  while (i + 6 <= data.size()) {
    const char* st = data.data() + i;
    const std::uint16_t sz = *reinterpret_cast<const std::uint16_t*>(data.data() + i + 4);
    i += 6;
    if (i + sz > data.size()) {
      break;
    }
    if (st[0] == 'D' && st[1] == 'A' && st[2] == 'T' && st[3] == 'A' && sz >= 1) {
      return (static_cast<unsigned char>(data[i]) & 0x01) != 0;
    }
    i += sz;
  }
  return false;
}

}  // namespace

std::string record_edid(const EsmRecord& rec) { return edid_from_payload(rec.data); }

bool record_is_interior_cell(const EsmRecord& rec) {
  return four(rec.type) == "CELL" && payload_is_interior_cell(rec.data);
}

std::uint32_t read_refr_base_formid(const EsmRecord& refr) {
  const auto& data = refr.data;
  std::size_t i = 0;
  while (i + 6 <= data.size()) {
    const char* st = data.data() + i;
    const std::uint16_t sz = *reinterpret_cast<const std::uint16_t*>(data.data() + i + 4);
    i += 6;
    if (i + sz > data.size()) {
      break;
    }
    if (st[0] == 'N' && st[1] == 'A' && st[2] == 'M' && st[3] == 'E' && sz == 4) {
      std::uint32_t id = 0;
      std::memcpy(&id, data.data() + i, 4);
      return id;
    }
    i += sz;
  }
  return 0;
}

bool load_record(const EsmIndex& index, std::uint32_t form_id, EsmRecord& out) {
  const auto it = index.locs.find(form_id);
  if (it == index.locs.end()) {
    return false;
  }
  std::ifstream in(index.esm_path, std::ios::binary);
  if (!in) {
    return false;
  }
  const auto& loc = it->second;
  in.seekg(static_cast<std::ifstream::off_type>(loc.payload_offset), std::ios::beg);
  out.flags = loc.flags;
  out.form_id = loc.form_id;
  std::memcpy(out.type, loc.type, 4);
  out.data.resize(loc.payload_size);
  if (loc.payload_size && !read_exact(in, out.data.data(), out.data.size())) {
    return false;
  }
  return true;
}

EsmIndex build_fallout3_index(const std::filesystem::path& install_root) {
  EsmIndex out;
  out.esm_path = install_root / L"Data" / L"Fallout3.esm";
  std::ifstream in(out.esm_path, std::ios::binary);
  if (!in) {
    out.summary = "Fallout3.esm not readable";
    return out;
  }

  std::vector<std::uint64_t> group_ends;
  std::size_t records = 0;

  while (in) {
    while (!group_ends.empty() && static_cast<std::uint64_t>(in.tellg()) >= group_ends.back()) {
      group_ends.pop_back();
    }

    char type[4]{};
    if (!read_exact(in, type, 4)) {
      break;
    }
    if (type[0] == 'G' && type[1] == 'R' && type[2] == 'U' && type[3] == 'P') {
      std::uint32_t group_size = 0;
      char rest[16]{};
      if (!read_exact(in, &group_size, 4) || !read_exact(in, rest, 16)) {
        break;
      }
      if (group_size < 24) {
        break;
      }
      const auto header_end = static_cast<std::uint64_t>(in.tellg());
      group_ends.push_back(header_end - 24 + group_size);
      continue;
    }

    std::uint32_t data_size = 0, flags = 0, form_id = 0, vc = 0;
    if (!read_exact(in, &data_size, 4) || !read_exact(in, &flags, 4) || !read_exact(in, &form_id, 4) ||
        !read_exact(in, &vc, 4)) {
      break;
    }
    EsmRecordLoc loc;
    std::memcpy(loc.type, type, 4);
    loc.flags = flags;
    loc.form_id = form_id;
    loc.payload_offset = static_cast<std::uint64_t>(in.tellg());
    loc.payload_size = data_size;

    if (four(type) == "CELL" && data_size > 0 && data_size < 1024 * 1024) {
      std::vector<char> payload(data_size);
      if (!read_exact(in, payload.data(), payload.size())) {
        break;
      }
      if (payload_is_interior_cell(payload)) {
        const auto edid = edid_from_payload(payload);
        if (!edid.empty()) {
          out.interior_cells_by_edid.emplace(edid, form_id);
        }
      }
    } else {
      in.seekg(data_size, std::ios::cur);
    }

    out.locs[form_id] = loc;
    ++records;
  }

  out.ok = !out.locs.empty();
  out.summary = out.ok ? ("Indexed " + std::to_string(records) + " F3 records") : "F3 index empty";
  out.detail = std::to_string(out.interior_cells_by_edid.size()) + " interior CELL EDIDs";
  log::info(out.summary + " (" + out.detail + ")");
  return out;
}

}  // namespace crossroads
