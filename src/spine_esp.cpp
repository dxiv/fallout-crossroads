#include "spine_esp.hpp"

#include "log.hpp"

#include <fstream>
#include <sstream>
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
  append_sub_str(data, "SNAM", "Fallout: Crossroads spine stubs");
  append_sub_str(data, "MAST", "FalloutNV.esm");
  std::uint64_t zero = 0;
  append_sub(data, "DATA", &zero, 8);
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

std::vector<char> build_note(const std::string& editor_id, const std::string& title, const std::string& body) {
  std::vector<char> data;
  append_sub_str(data, "EDID", editor_id);
  append_sub_str(data, "FULL", title);
  // TNAM note type 0 = note text; DATA/TNAM layout varies — keep minimal FULL + EDID for stub identity.
  append_sub_str(data, "TNAM", body);
  return data;
}

}  // namespace

SpineEspResult write_spine_esp(const std::filesystem::path& out_esp) {
  SpineEspResult result;
  result.path = out_esp;
  result.stubs = {
      {"CR_SpineAirfield", "Find the wasteland airfield"},
      {"CR_SpineBoardPlane", "Board the westbound flight"},
      {"CR_SpineMojaveTouchdown", "Touch down in the Mojave"},
      {"CR_SpineFerryDock", "Reach the Colorado ferry"},
      {"CR_SpineCapitalReturn", "Return to the Capital Wasteland"},
  };

  constexpr std::uint32_t kBase = 0x0100B000;
  std::vector<char> file;
  const std::uint32_t n = static_cast<std::uint32_t>(2 + result.stubs.size());
  append_record(file, "TES4", 0, 0, build_tes4(kBase + 0x100, n));
  append_record(file, "GLOB", 0, kBase + 0, build_glob("CR_SpineStage", 0.0f));
  append_record(file, "GLOB", 0, kBase + 1, build_glob("CR_SpineCombatReady", 1.0f));

  std::uint32_t id = kBase + 0x10;
  for (const auto& stub : result.stubs) {
    append_record(file, "NOTE", 0, id++, build_note(stub.editor_id, stub.title, stub.title));
  }

  std::error_code ec;
  std::filesystem::create_directories(out_esp.parent_path(), ec);
  std::ofstream out(out_esp, std::ios::binary | std::ios::trunc);
  if (!out) {
    result.summary = "Failed to write CrossroadsSpine.esp";
    return result;
  }
  out.write(file.data(), static_cast<std::streamsize>(file.size()));

  std::ostringstream detail;
  detail << result.stubs.size() << " spine NOTE stubs + CR_SpineStage/CombatReady";
  result.ok = true;
  result.summary = "Wrote CrossroadsSpine.esp";
  result.detail = detail.str();
  log::info(result.summary + " — " + result.detail);
  return result;
}

}  // namespace crossroads
