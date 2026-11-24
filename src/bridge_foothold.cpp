#include "bridge_foothold.hpp"

#include "bsa_wire.hpp"
#include "capital_travel.hpp"
#include "cell_import.hpp"
#include "coverage_policy.hpp"
#include "esp_writer.hpp"
#include "log.hpp"
#include "mojave_foothold.hpp"
#include "settings.hpp"
#include "spine_esp.hpp"
#include "transit_esp.hpp"
#include "transit_journey.hpp"
#include "world_import.hpp"

#include <fstream>
#include <sstream>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <ShlObj.h>

namespace crossroads {
namespace {

std::string wide_to_utf8(const std::wstring& s) {
  if (s.empty()) {
    return {};
  }
  const int n = WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0, nullptr, nullptr);
  std::string out(n, '\0');
  WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), out.data(), n, nullptr, nullptr);
  return out;
}

std::filesystem::path local_appdata() {
  PWSTR path = nullptr;
  if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, nullptr, &path)) || !path) {
    return {};
  }
  std::filesystem::path out = path;
  CoTaskMemFree(path);
  return out;
}

bool file_contains_line(const std::filesystem::path& path, const std::string& line) {
  std::ifstream in(path);
  if (!in) {
    return false;
  }
  std::string row;
  while (std::getline(in, row)) {
    if (!row.empty() && row.back() == '\r') {
      row.pop_back();
    }
    if (row == line || row == ("*" + line)) {
      return true;
    }
  }
  return false;
}

bool ensure_plugin_enabled(const std::filesystem::path& plugins_txt, const std::string& esp_name) {
  std::error_code ec;
  std::filesystem::create_directories(plugins_txt.parent_path(), ec);
  if (file_contains_line(plugins_txt, esp_name)) {
    return true;
  }
  const bool exists = std::filesystem::is_regular_file(plugins_txt, ec);
  std::ofstream out(plugins_txt, exists ? std::ios::app : std::ios::trunc);
  if (!out) {
    return false;
  }
  if (!exists) {
    out << "FalloutNV.esm\n";
  } else {
    out << "\n";
  }
  out << esp_name << "\n";
  return static_cast<bool>(out);
}

bool ensure_loadorder_line(const std::filesystem::path& loadorder_txt, const std::string& esp_name) {
  std::error_code ec;
  if (loadorder_txt.empty()) {
    return false;
  }
  std::filesystem::create_directories(loadorder_txt.parent_path(), ec);
  if (file_contains_line(loadorder_txt, esp_name)) {
    return true;
  }
  const bool exists = std::filesystem::is_regular_file(loadorder_txt, ec);
  std::ofstream out(loadorder_txt, exists ? std::ios::app : std::ios::trunc);
  if (!out) {
    return false;
  }
  if (exists) {
    out << "\n";
  }
  out << esp_name << "\n";
  return static_cast<bool>(out);
}

bool deploy_named_esp(const std::filesystem::path& staged,
                      const std::filesystem::path& fnv_install,
                      const std::wstring& esp_name,
                      std::filesystem::path& out_data_esp,
                      std::string& plugins_note) {
  const auto data_dir = fnv_install / L"Data";
  if (!std::filesystem::is_directory(data_dir)) {
    plugins_note = "FNV Data folder missing";
    return false;
  }
  out_data_esp = data_dir / esp_name;
  std::error_code ec;
  std::filesystem::copy_file(staged, out_data_esp, std::filesystem::copy_options::overwrite_existing, ec);
  if (ec) {
    plugins_note = ec.message();
    return false;
  }
  const std::string name_utf8 = wide_to_utf8(esp_name);
  bool enabled = false;
  const auto local_dir = local_appdata() / L"FalloutNV";
  if (!local_dir.empty()) {
    enabled = ensure_plugin_enabled(local_dir / L"plugins.txt", name_utf8) || enabled;
    ensure_loadorder_line(local_dir / L"loadorder.txt", name_utf8);
  }
  enabled = ensure_plugin_enabled(data_dir / L"plugins.txt", name_utf8) || enabled;
  plugins_note = enabled ? (name_utf8 + " enabled") : ("enable " + name_utf8 + " manually");
  return true;
}

}  // namespace

std::filesystem::path foothold_staging_dir() { return settings_dir() / L"bridge"; }

FootholdBridgeResult build_and_deploy_foothold(const std::filesystem::path& f3_install,
                                               const std::filesystem::path& fnv_install) {
  FootholdBridgeResult result;
  const auto staged = foothold_staging_dir() / L"CrossroadsFoothold.esp";
  const auto transit_staged = foothold_staging_dir() / L"CrossroadsTransit.esp";
  const auto spine_staged = foothold_staging_dir() / L"CrossroadsSpine.esp";

  const JourneyState journey = load_journey();
  const CoveragePolicy coverage = coverage_for_journey(journey);
  result.coverage_note = coverage.note;

  CapitalImportBudget budget;
  budget.exterior_radius = coverage.capital_exterior_radius;
  budget.max_exterior_cells = coverage.capital_max_exterior_cells;
  budget.max_refs = coverage.capital_max_refs;

  const Phase1ImportResult pack = import_phase1_capital_pack(f3_install, staged, budget);
  bool wrote_esp = pack.ok;
  bool imported_capital_assets = pack.ok;
  if (pack.ok) {
    result.cell_editor_id = pack.interior_edid.empty() ? "CrossroadsFoothold" : pack.interior_edid;
    result.world_editor_id = pack.world_edid;
    result.f3_worldspace_hint = pack.world_edid.empty() ? pack.interior_edid : pack.world_edid;
    result.coc_hint = pack.coc_hint;
    result.cow_hint = pack.cow_hint;
    result.exterior_cells = pack.exterior_cells;
  } else {
    log::warn("Capital pack failed (" + pack.summary + "); trying interior-only import");
    const CellImportResult imported = import_capital_foothold_cell(f3_install, staged);
    if (imported.ok) {
      wrote_esp = true;
      imported_capital_assets = true;
      result.cell_editor_id = imported.cell_editor_id;
      result.f3_worldspace_hint = imported.cell_editor_id;
      result.coc_hint = "coc " + imported.cell_editor_id;
    } else {
      log::warn("F3 cell import failed (" + imported.summary + "); falling back to empty foothold CELL");
      result.cell_editor_id = "CrossroadsFoothold";
      result.coc_hint = "coc CrossroadsFoothold";
      const auto written = write_foothold_esp(staged, result.cell_editor_id);
      if (!written.ok) {
        result.summary = written.summary;
        result.detail = written.detail + " | pack: " + pack.summary + " | interior: " + imported.summary;
        return result;
      }
      wrote_esp = true;
      imported_capital_assets = false;
    }
  }

  if (!wrote_esp) {
    result.summary = "No foothold ESP produced";
    return result;
  }

  const CapitalTravelResult travel = append_capital_pipboy_travel(staged);
  if (!travel.ok) {
    result.summary = travel.summary;
    result.detail = travel.detail;
    return result;
  }

  const TransitEspResult transit = write_transit_esp(transit_staged);
  if (!transit.ok) {
    result.summary = transit.summary;
    result.detail = transit.detail;
    return result;
  }
  result.plane_cell = transit.plane_cell;
  result.boat_cell = transit.boat_cell;

  const SpineEspResult spine = write_spine_esp(spine_staged);
  if (!spine.ok) {
    result.summary = spine.summary;
    result.detail = spine.detail;
    return result;
  }

  const MojaveFootholdResult mojave = resolve_mojave_foothold(fnv_install);
  if (mojave.ok) {
    result.mojave_cell_edid = mojave.cell_editor_id;
    result.mojave_world_edid = mojave.world_editor_id;
    result.mojave_cow_hint = mojave.cow_hint;
  }

  // FO3 BSA overlays shadow FNV mesh/texture paths and can AV on save load.
  // Only wire when Capital content was actually imported from Fallout3.esm.
  BsaWireResult bsa;
  if (imported_capital_assets) {
    bsa = wire_fallout3_bsas_into_fnv(f3_install, fnv_install);
  } else {
    log::warn("Skipping FO3 BSA wire (no imported Capital content); clearing any prior CR_FO3_* overlays");
    bsa = unwire_fallout3_bsas_from_fnv(fnv_install);
    bsa.summary = "BSA wire skipped — " + bsa.summary;
  }

  std::string plugins_note;
  if (!deploy_named_esp(staged, fnv_install, L"CrossroadsFoothold.esp", result.esp_in_data, plugins_note)) {
    result.summary = "Failed to deploy foothold ESP into FNV Data";
    result.detail = plugins_note;
    return result;
  }
  std::string transit_note;
  if (!deploy_named_esp(transit_staged, fnv_install, L"CrossroadsTransit.esp", result.transit_esp_in_data,
                        transit_note)) {
    result.summary = "Failed to deploy CrossroadsTransit.esp";
    result.detail = transit_note;
    return result;
  }
  std::string spine_note;
  if (!deploy_named_esp(spine_staged, fnv_install, L"CrossroadsSpine.esp", result.spine_esp_in_data, spine_note)) {
    result.summary = "Failed to deploy CrossroadsSpine.esp";
    result.detail = spine_note;
    return result;
  }

  std::ostringstream detail;
  if (pack.ok) {
    detail << pack.detail;
  } else {
    detail << "fallback capital path";
  }
  detail << " | coverage hot=" << hot_wasteland_name(coverage.hot) << " (" << coverage.note << ")";
  detail << " | pipboy: " << travel.detail;
  detail << " | transit: " << transit.detail;
  detail << " | spine: " << spine.detail;
  detail << " | mojave: " << (mojave.ok ? mojave.detail : mojave.summary);
  detail << " | " << bsa.summary << " — " << bsa.detail;
  detail << " | " << plugins_note << " | " << transit_note << " | " << spine_note;

  result.ok = true;
  result.summary = "Phase 3 coverage + spine deployed";
  result.detail = detail.str();
  log::info(result.summary + " — " + result.detail);
  return result;
}

}  // namespace crossroads
