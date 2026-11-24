#include "capital_travel.hpp"

#include "log.hpp"

#include <cstdint>
#include <fstream>
#include <sstream>
#include <vector>

namespace crossroads {
namespace {

// Vanilla FalloutNV.esm form IDs (mod index 0x00).
constexpr std::uint32_t kMitchellCell = 0x00103DF9;     // GSDocMitchellHouse
constexpr std::uint32_t kGoodspringsCell = 0x000DAE41;  // WastelandNV -18,16
constexpr std::uint32_t kDoorBase = 0x0016B5E9;         // NVProspectorSaloonDoor
constexpr std::uint32_t kMapMarkerBase = 0x00000010;    // MapMarker

// Dedicated travel forms (0x01000E** band — keeps clear of import 0x01004***).
constexpr std::uint32_t kArriveCell = 0x01000E00;
constexpr std::uint32_t kDestDoorRef = 0x01000E10;
constexpr std::uint32_t kOriginDoorRef = 0x01000E11;
constexpr std::uint32_t kMapMarkerRef = 0x01000E12;
constexpr std::uint32_t kExteriorDoorRef = 0x01000E13;
constexpr std::uint32_t kGuideNote = 0x01000E14;
constexpr std::uint32_t kGuideNoteRef = 0x01000E15;
constexpr std::uint32_t kQuestId = 0x01000E20;

constexpr std::uint32_t kRefrPersistent = 0x00000400;

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

std::vector<char> wrap_grup(std::uint32_t label, std::int32_t group_type, const std::vector<char>& body) {
  // FO3/FNV GRUP header is 24 bytes: type, size, label, groupType, stamp, unknown, version, unknown2.
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

void append_data_pose(std::vector<char>& data, float x, float y, float z) {
  std::vector<char> pose;
  append_f32(pose, x);
  append_f32(pose, y);
  append_f32(pose, z);
  append_f32(pose, 0.0f);
  append_f32(pose, 0.0f);
  append_f32(pose, 0.0f);
  append_sub(data, "DATA", pose.data(), static_cast<std::uint16_t>(pose.size()));
}

void append_xtel(std::vector<char>& data, std::uint32_t dest_door, float x, float y, float z) {
  std::vector<char> xtel;
  append_u32(xtel, dest_door);
  append_f32(xtel, x);
  append_f32(xtel, y);
  append_f32(xtel, z);
  append_f32(xtel, 0.0f);
  append_f32(xtel, 0.0f);
  append_f32(xtel, 0.0f);
  append_u32(xtel, 0);
  append_sub(data, "XTEL", xtel.data(), static_cast<std::uint16_t>(xtel.size()));
}

std::vector<char> build_arrive_cell() {
  std::vector<char> data;
  append_sub_str(data, "EDID", "CR_CapitalArrive");
  const std::uint8_t flags = 0x01;  // interior
  append_sub(data, "DATA", &flags, 1);
  struct Xcll {
    std::uint32_t ambient = 0x00202028;
    std::uint32_t directional = 0x00101018;
    std::uint32_t fog = 0x00202028;
    float fog_near = 0.0f;
    float fog_far = 0.0f;
    std::int32_t directional_xy = 0;
    std::int32_t directional_z = 0;
    float directional_fade = 1.0f;
    float fog_clip = 0.0f;
  } xcll{};
  append_sub(data, "XCLL", &xcll, static_cast<std::uint16_t>(sizeof(xcll)));

  std::vector<char> file;
  append_record(file, "CELL", 0, kArriveCell, data);
  return file;
}

std::vector<char> build_door_refr(std::uint32_t form_id,
                                  const char* edid,
                                  std::uint32_t dest_door,
                                  float x,
                                  float y,
                                  float z,
                                  float dest_x,
                                  float dest_y,
                                  float dest_z) {
  std::vector<char> data;
  append_sub_str(data, "EDID", edid);
  {
    std::uint32_t base = kDoorBase;
    append_sub(data, "NAME", &base, 4);
  }
  append_xtel(data, dest_door, dest_x, dest_y, dest_z);
  append_data_pose(data, x, y, z);

  std::vector<char> file;
  append_record(file, "REFR", kRefrPersistent, form_id, data);
  return file;
}

std::vector<char> build_map_marker_refr() {
  std::vector<char> data;
  append_sub_str(data, "EDID", "CR_CapitalMapMarker");
  {
    std::uint32_t base = kMapMarkerBase;
    append_sub(data, "NAME", &base, 4);
  }
  append_sub(data, "XMRK", nullptr, 0);
  const std::uint8_t fnam = 0x03;  // Visible | Can Travel To
  append_sub(data, "FNAM", &fnam, 1);
  append_sub_str(data, "FULL", "Crossroads Capital");
  {
    std::uint8_t tnam[2] = {2, 0};  // Settlement
    append_sub(data, "TNAM", tnam, 2);
  }
  append_data_pose(data, -71000.0f, 68000.0f, 4900.0f);

  std::vector<char> file;
  append_record(file, "REFR", kRefrPersistent, kMapMarkerRef, data);
  return file;
}

std::vector<char> build_guide_note() {
  std::vector<char> data;
  append_sub_str(data, "EDID", "CR_CapitalGuideNote");
  append_sub_str(data, "FULL", "Crossroads Capital Guide");
  const std::uint8_t note_type = 0;  // text
  append_sub(data, "DATA", &note_type, 1);
  append_sub_str(data, "TNAM",
                 "Capital foothold travel (no console):\n"
                 "1) Use the Crossroads door in Doc Mitchell's house.\n"
                 "2) Or open the World Map and fast-travel to Crossroads Capital "
                 "(marker near Goodsprings; gate beside it).\n"
                 "Both lead to CR_CapitalArrive on the New Vegas host.");
  std::vector<char> file;
  append_record(file, "NOTE", 0, kGuideNote, data);
  return file;
}

std::vector<char> build_note_refr() {
  std::vector<char> data;
  append_sub_str(data, "EDID", "CR_CapitalGuideNoteRef");
  {
    std::uint32_t base = kGuideNote;
    append_sub(data, "NAME", &base, 4);
  }
  // On the clinic floor near the gate door.
  append_data_pose(data, 1980.0f, 1720.0f, 7360.0f);
  std::vector<char> file;
  append_record(file, "REFR", kRefrPersistent, kGuideNoteRef, data);
  return file;
}

std::vector<char> build_quest() {
  std::vector<char> data;
  append_sub_str(data, "EDID", "CR_CapitalTransitQuest");
  append_sub_str(data, "FULL", "Crossroads: Capital Foothold");
  {
    std::vector<char> qdata;
    qdata.push_back(0x01);  // Start Game Enabled
    qdata.push_back(55);
    qdata.push_back(0);
    qdata.push_back(0);
    append_f32(qdata, 0.0f);
    append_sub(data, "DATA", qdata.data(), static_cast<std::uint16_t>(qdata.size()));
  }

  {
    std::int16_t indx = 0;
    append_sub(data, "INDX", &indx, 2);
  }
  {
    std::uint8_t qsdt = 0;
    append_sub(data, "QSDT", &qsdt, 1);
  }
  append_sub_str(data, "CNAM",
                 "Crossroads opened a Capital foothold gate. Use the door in Doc Mitchell's "
                 "house, or open the world map and fast-travel to Crossroads Capital.");

  {
    std::int32_t qobj = 10;
    append_sub(data, "QOBJ", &qobj, 4);
  }
  append_sub_str(data, "NNAM",
                 "Reach the Capital foothold via the Doc Mitchell gate or map marker "
                 "Crossroads Capital.");
  {
    std::vector<char> qsta;
    append_u32(qsta, kOriginDoorRef);
    qsta.push_back(0);
    qsta.push_back(0);
    qsta.push_back(0);
    qsta.push_back(0);
    append_sub(data, "QSTA", qsta.data(), static_cast<std::uint16_t>(qsta.size()));
  }

  std::vector<char> file;
  append_record(file, "QUST", 0, kQuestId, data);
  return file;
}

std::vector<char> build_travel_blob() {
  auto cell = build_arrive_cell();
  auto note = build_guide_note();
  auto dest = build_door_refr(kDestDoorRef, "CR_CapitalGateArrive", kOriginDoorRef, 128.0f, 0.0f,
                              0.0f, 2000.0f, 1750.0f, 7360.0f);
  auto origin = build_door_refr(kOriginDoorRef, "CR_CapitalGateMitchell", kDestDoorRef, 2000.0f,
                                1750.0f, 7360.0f, 128.0f, 0.0f, 0.0f);
  auto note_ref = build_note_refr();
  auto marker = build_map_marker_refr();
  auto exterior = build_door_refr(kExteriorDoorRef, "CR_CapitalGateGoodsprings", kDestDoorRef,
                                  -70950.0f, 68040.0f, 4900.0f, 128.0f, 0.0f, 0.0f);

  auto arrive_cell_grup = wrap_grup(kArriveCell, 6, wrap_grup(kArriveCell, 8, dest));

  std::vector<char> mitchell_persistent = origin;
  mitchell_persistent.insert(mitchell_persistent.end(), note_ref.begin(), note_ref.end());
  auto mitchell_cell_grup =
      wrap_grup(kMitchellCell, 6, wrap_grup(kMitchellCell, 8, mitchell_persistent));

  std::vector<char> world_persistent = marker;
  world_persistent.insert(world_persistent.end(), exterior.begin(), exterior.end());
  auto world_cell_grup =
      wrap_grup(kGoodspringsCell, 6, wrap_grup(kGoodspringsCell, 8, world_persistent));

  auto quest = build_quest();

  std::vector<char> out;
  out.insert(out.end(), cell.begin(), cell.end());
  out.insert(out.end(), note.begin(), note.end());
  out.insert(out.end(), arrive_cell_grup.begin(), arrive_cell_grup.end());
  out.insert(out.end(), mitchell_cell_grup.begin(), mitchell_cell_grup.end());
  out.insert(out.end(), world_cell_grup.begin(), world_cell_grup.end());
  out.insert(out.end(), quest.begin(), quest.end());
  return out;
}

bool file_contains_quest_edid(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    return false;
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str().find("CR_CapitalTransitQuest") != std::string::npos;
}

}  // namespace

CapitalTravelResult append_capital_pipboy_travel(const std::filesystem::path& foothold_esp) {
  CapitalTravelResult result;
  std::error_code ec;
  if (!std::filesystem::is_regular_file(foothold_esp, ec)) {
    result.summary = "Foothold ESP missing for Pip-Boy travel inject";
    result.detail = foothold_esp.string();
    return result;
  }

  // Always rebuild travel bytes so GRUP/header fixes redeploy cleanly.
  if (file_contains_quest_edid(foothold_esp)) {
    std::ifstream in(foothold_esp, std::ios::binary);
    std::ostringstream ss;
    ss << in.rdbuf();
    std::string bytes = ss.str();
    const auto cut = bytes.find("CR_CapitalArrive");
    if (cut != std::string::npos) {
      // Trim back to start of the CR_CapitalArrive CELL header (4 bytes before EDID payload).
      // EDID sits inside the CELL record; search backward for "CELL".
      const auto cell = bytes.rfind("CELL", cut);
      if (cell != std::string::npos && cell > 0) {
        bytes.resize(cell);
        std::ofstream trim(foothold_esp, std::ios::binary | std::ios::trunc);
        trim.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
      }
    }
  }

  const auto blob = build_travel_blob();
  std::ofstream out(foothold_esp, std::ios::binary | std::ios::app);
  if (!out) {
    result.summary = "Failed to append Pip-Boy Capital travel";
    result.detail = foothold_esp.string();
    return result;
  }
  out.write(blob.data(), static_cast<std::streamsize>(blob.size()));
  if (!out) {
    result.summary = "Failed while writing Pip-Boy Capital travel bytes";
    return result;
  }

  result.ok = true;
  result.summary = "Pip-Boy Capital travel injected";
  result.detail =
      "Quest Crossroads: Capital Foothold + Doc Mitchell gate + map marker "
      "Crossroads Capital (CR_CapitalArrive) in " +
      foothold_esp.string();
  log::info(result.summary + " — " + result.detail);
  return result;
}

}  // namespace crossroads
