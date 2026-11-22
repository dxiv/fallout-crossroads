#include "transit_esp.hpp"

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
  append_u32(out, 0);
  out.insert(out.end(), data.begin(), data.end());
}

std::vector<char> build_tes4(std::uint32_t next_id, std::uint32_t num_records) {
  std::vector<char> data;
  std::vector<char> hedr;
  append_f32(hedr, 1.0f);
  append_u32(hedr, num_records);
  append_u32(hedr, next_id);
  append_sub(data, "HEDR", hedr.data(), static_cast<std::uint16_t>(hedr.size()));
  append_sub_str(data, "CNAM", "dxiv");
  append_sub_str(data, "SNAM", "Fallout: Crossroads Transit (plane + boat)");
  append_sub_str(data, "MAST", "FalloutNV.esm");
  std::uint64_t zero = 0;
  append_sub(data, "DATA", &zero, 8);
  return data;
}

std::vector<char> build_interior_cell(const std::string& editor_id) {
  std::vector<char> data;
  append_sub_str(data, "EDID", editor_id);
  const std::uint8_t flags = 0x01;
  append_sub(data, "DATA", &flags, 1);
  struct Xcll {
    std::uint32_t ambient = 0x00282832;
    std::uint32_t directional = 0x00101018;
    std::uint32_t fog = 0x00282832;
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

std::vector<char> build_glob(const std::string& editor_id, float value) {
  std::vector<char> data;
  append_sub_str(data, "EDID", editor_id);
  const char type = 'f';
  append_sub(data, "FNAM", &type, 1);
  append_sub(data, "FLTV", &value, 4);
  return data;
}

}  // namespace

TransitEspResult write_transit_esp(const std::filesystem::path& out_esp) {
  TransitEspResult result;
  result.path = out_esp;

  constexpr std::uint32_t kPlane = 0x0100A100;
  constexpr std::uint32_t kBoat = 0x0100A101;
  constexpr std::uint32_t kGlobLeg = 0x0100A110;
  constexpr std::uint32_t kGlobLoop = 0x0100A111;

  std::vector<char> file;
  append_record(file, "TES4", 0, 0, build_tes4(0x0100A200, 4));
  append_record(file, "GLOB", 0, kGlobLeg, build_glob("CR_TransitLeg", 0.0f));
  append_record(file, "GLOB", 0, kGlobLoop, build_glob("CR_TransitLoops", 0.0f));
  append_record(file, "CELL", 0, kPlane, build_interior_cell(result.plane_cell));
  append_record(file, "CELL", 0, kBoat, build_interior_cell(result.boat_cell));

  std::error_code ec;
  std::filesystem::create_directories(out_esp.parent_path(), ec);
  std::ofstream out(out_esp, std::ios::binary | std::ios::trunc);
  if (!out) {
    result.summary = "Failed to write CrossroadsTransit.esp";
    return result;
  }
  out.write(file.data(), static_cast<std::streamsize>(file.size()));
  result.ok = true;
  result.summary = "Wrote CrossroadsTransit.esp";
  result.detail = result.plane_cell + " + " + result.boat_cell + " + CR_TransitLeg/Loops GLOBs";
  log::info(result.summary + " — " + result.detail);
  return result;
}

}  // namespace crossroads
