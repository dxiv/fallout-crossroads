#include "esp_writer.hpp"

#include "log.hpp"

#include <cstring>
#include <fstream>
#include <vector>

namespace crossroads {
namespace {

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

void append_sub(std::vector<char>& out, const char type[4], const void* data, std::uint16_t size) {
  append_bytes(out, type, 4);
  append_u16(out, size);
  if (size && data) {
    append_bytes(out, data, size);
  }
}

void append_sub_str(std::vector<char>& out, const char type[4], const std::string& s) {
  std::vector<char> payload;
  append_zstring(payload, s);
  append_sub(out, type, payload.data(), static_cast<std::uint16_t>(payload.size()));
}

void append_record(std::vector<char>& out,
                   const char type[4],
                   std::uint32_t flags,
                   std::uint32_t form_id,
                   const std::vector<char>& data) {
  append_bytes(out, type, 4);
  append_u32(out, static_cast<std::uint32_t>(data.size()));
  append_u32(out, flags);
  append_u32(out, form_id);
  append_u32(out, 0);  // VC / timestamp
  out.insert(out.end(), data.begin(), data.end());
}

std::vector<char> build_tes4() {
  std::vector<char> data;
  // HEDR: version(float) + numRecords(uint32) + nextObjectId(uint32)
  {
    std::vector<char> hedr;
    append_f32(hedr, 1.0f);      // FO3/FNV-ish
    append_u32(hedr, 2);         // CELL + (implicit)
    append_u32(hedr, 0x01000800);
    append_sub(data, "HEDR", hedr.data(), static_cast<std::uint16_t>(hedr.size()));
  }
  append_sub_str(data, "CNAM", "dxiv");
  append_sub_str(data, "SNAM", "Fallout: Crossroads foothold (Phase 1b)");
  append_sub_str(data, "MAST", "FalloutNV.esm");
  {
    // DATA after MAST: master file size (uint64). Unknown size OK as 0.
    std::uint64_t zero = 0;
    append_sub(data, "DATA", &zero, 8);
  }
  return data;
}

std::vector<char> build_interior_cell(const std::string& editor_id) {
  std::vector<char> data;
  append_sub_str(data, "EDID", editor_id);
  // DATA: flags uint8 — bit 0 = interior
  const std::uint8_t flags = 0x01;
  append_sub(data, "DATA", &flags, 1);
  // XCLL: lighting (32 bytes typical for FO3/FNV)
  // ambientRGB, directionalRGB, fogRGB as uint32 (BGRA-ish), fog near, fog far, etc.
  struct Xcll {
    std::uint32_t ambient = 0x001E1E1E;
    std::uint32_t directional = 0x00000000;
    std::uint32_t fog = 0x001E1E1E;
    float fog_near = 0.0f;
    float fog_far = 0.0f;
    std::int32_t directional_xy = 0;
    std::int32_t directional_z = 0;
    float directional_fade = 1.0f;
    float fog_clip = 0.0f;
  } xcll{};
  append_sub(data, "XCLL", &xcll, static_cast<std::uint16_t>(sizeof(xcll)));
  return data;
}

}  // namespace

EspWriteResult write_foothold_esp(const std::filesystem::path& out_esp, const std::string& cell_editor_id) {
  EspWriteResult result;
  result.path = out_esp;

  std::vector<char> file;
  append_record(file, "TES4", 0, 0, build_tes4());
  // With one master (FalloutNV.esm), new records use mod index 0x01.
  constexpr std::uint32_t kCellId = 0x01000F00;
  append_record(file, "CELL", 0, kCellId, build_interior_cell(cell_editor_id));

  std::error_code ec;
  std::filesystem::create_directories(out_esp.parent_path(), ec);
  std::ofstream out(out_esp, std::ios::binary | std::ios::trunc);
  if (!out) {
    result.summary = "Failed to write foothold ESP";
    result.detail = out_esp.string();
    log::error(result.summary);
    return result;
  }
  out.write(file.data(), static_cast<std::streamsize>(file.size()));
  if (!out) {
    result.summary = "Failed while writing foothold ESP bytes";
    return result;
  }

  result.ok = true;
  result.summary = "Wrote foothold ESP";
  result.detail = out_esp.string() + " (CELL " + cell_editor_id + ")";
  log::info(result.summary + ": " + result.detail);
  return result;
}

}  // namespace crossroads
