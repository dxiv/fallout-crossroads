#include "esm_probe.hpp"

#include "log.hpp"

#include <cstdint>
#include <fstream>
#include <sstream>
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

}  // namespace

EsmProbeResult probe_fallout3_worldspaces(const std::filesystem::path& install_root,
                                          std::size_t max_worldspaces) {
  EsmProbeResult out;
  const auto esm = install_root / L"Data" / L"Fallout3.esm";
  std::ifstream in(esm, std::ios::binary);
  if (!in) {
    out.summary = "Fallout3.esm not readable";
    out.detail = "Expected BYO file under Data\\Fallout3.esm.";
    return out;
  }

  // Nested GRUP end offsets (absolute file positions).
  std::vector<std::uint64_t> group_ends;

  while (in && out.worldspaces.size() < max_worldspaces) {
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

    std::uint32_t data_size = 0;
    std::uint32_t flags = 0;
    std::uint32_t form_id = 0;
    std::uint32_t vc = 0;
    if (!read_exact(in, &data_size, 4) || !read_exact(in, &flags, 4) || !read_exact(in, &form_id, 4) ||
        !read_exact(in, &vc, 4)) {
      break;
    }
    std::vector<char> payload(data_size);
    if (data_size && !read_exact(in, payload.data(), payload.size())) {
      break;
    }
    if (type[0] == 'W' && type[1] == 'R' && type[2] == 'L' && type[3] == 'D') {
      WorldspaceHit hit;
      hit.form_id = form_id;
      hit.editor_id = extract_edid(payload);
      if (hit.editor_id.empty()) {
        std::ostringstream id;
        id << "WRLD_" << std::hex << form_id;
        hit.editor_id = id.str();
      }
      out.worldspaces.push_back(hit);
    }
  }

  if (out.worldspaces.empty()) {
    out.summary = "No worldspaces listed yet";
    out.detail =
        "Opened Fallout3.esm but found no WRLD records in this pass. "
        "Capital foothold bridging still needs deeper record support.";
    log::warn(out.detail);
    return out;
  }

  std::ostringstream found;
  for (std::size_t i = 0; i < out.worldspaces.size(); ++i) {
    if (i) {
      found << ", ";
    }
    found << out.worldspaces[i].editor_id;
  }
  out.ok = true;
  out.summary = "F3 ESM probe OK (" + std::to_string(out.worldspaces.size()) + " WRLD)";
  out.detail = found.str();
  log::info(out.summary + ": " + out.detail);
  return out;
}

}  // namespace crossroads
