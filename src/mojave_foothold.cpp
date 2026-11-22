#include "mojave_foothold.hpp"

#include "log.hpp"

#include <cstdint>
#include <cstring>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace crossroads {
namespace {

bool read_exact(std::ifstream& in, void* dst, std::size_t n) {
  in.read(reinterpret_cast<char*>(dst), static_cast<std::streamsize>(n));
  return static_cast<std::size_t>(in.gcount()) == n;
}

std::string extract_edid(const std::vector<char>& data) {
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

const char* kPreferredCells[] = {"GSDocMitchellHouse", "Goodsprings", "NovacMotelRoomUnique",
                                 "NovacDinoGiftShop", "McCarranHangar", "NellisMessHall",
                                 "PrimmVikkiAndVance", nullptr};
const char* kPreferredWorlds[] = {"WastelandNV", "Wasteland", nullptr};

}  // namespace

MojaveFootholdResult resolve_mojave_foothold(const std::filesystem::path& fnv_install) {
  MojaveFootholdResult result;
  const auto esm = fnv_install / L"Data" / L"FalloutNV.esm";
  std::ifstream in(esm, std::ios::binary);
  if (!in) {
    result.summary = "FalloutNV.esm not readable";
    result.detail = "Expected BYO Data\\FalloutNV.esm";
    return result;
  }

  std::unordered_map<std::string, std::string> interior_hits;  // edid -> edid
  std::string world_edid;
  std::vector<std::uint64_t> group_ends;

  while (in) {
    while (!group_ends.empty() && static_cast<std::uint64_t>(in.tellg()) >= group_ends.back()) {
      group_ends.pop_back();
    }
    char type[4]{};
    if (!read_exact(in, type, 4)) {
      break;
    }
    if (std::memcmp(type, "GRUP", 4) == 0) {
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
    std::vector<char> payload(data_size);
    if (data_size && !read_exact(in, payload.data(), data_size)) {
      break;
    }

    if (std::memcmp(type, "WRLD", 4) == 0 && world_edid.empty()) {
      const auto edid = extract_edid(payload);
      for (int i = 0; kPreferredWorlds[i]; ++i) {
        if (edid == kPreferredWorlds[i]) {
          world_edid = edid;
          break;
        }
      }
      if (world_edid.empty() && edid.find("Wasteland") != std::string::npos) {
        world_edid = edid;
      }
    }
    if (std::memcmp(type, "CELL", 4) == 0) {
      const auto edid = extract_edid(payload);
      if (!edid.empty()) {
        for (int i = 0; kPreferredCells[i]; ++i) {
          if (edid == kPreferredCells[i]) {
            interior_hits[edid] = edid;
          }
        }
      }
    }
  }

  for (int i = 0; kPreferredCells[i]; ++i) {
    if (interior_hits.count(kPreferredCells[i])) {
      result.cell_editor_id = kPreferredCells[i];
      break;
    }
  }
  result.world_editor_id = world_edid.empty() ? "WastelandNV" : world_edid;

  if (result.cell_editor_id.empty()) {
    // Host still has Mojave exteriors; fall back to a well-known console target.
    result.cell_editor_id = "GSDocMitchellHouse";
    result.summary = "Mojave foothold (default Goodsprings house)";
    result.detail = "Preferred EDIDs not confirmed in scan; using " + result.cell_editor_id +
                    " / world " + result.world_editor_id;
  } else {
    result.summary = "Mojave foothold resolved";
    result.detail = "cell=" + result.cell_editor_id + " world=" + result.world_editor_id;
  }
  result.ok = true;
  log::info(result.summary + " — " + result.detail);
  return result;
}

}  // namespace crossroads
