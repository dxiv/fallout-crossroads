#include "bridge_foothold.hpp"

#include "bsa_wire.hpp"
#include "cell_import.hpp"
#include "esp_writer.hpp"
#include "log.hpp"
#include "settings.hpp"
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

bool deploy_esp(const std::filesystem::path& staged,
                const std::filesystem::path& fnv_install,
                std::filesystem::path& out_data_esp,
                std::string& plugins_note) {
  const auto data_dir = fnv_install / L"Data";
  if (!std::filesystem::is_directory(data_dir)) {
    plugins_note = "FNV Data folder missing";
    return false;
  }
  out_data_esp = data_dir / L"CrossroadsFoothold.esp";
  std::error_code ec;
  std::filesystem::copy_file(staged, out_data_esp, std::filesystem::copy_options::overwrite_existing, ec);
  if (ec) {
    plugins_note = ec.message();
    return false;
  }
  bool enabled = false;
  const auto local = local_appdata() / L"FalloutNV" / L"plugins.txt";
  if (!local.empty()) {
    enabled = ensure_plugin_enabled(local, "CrossroadsFoothold.esp") || enabled;
  }
  enabled = ensure_plugin_enabled(data_dir / L"plugins.txt", "CrossroadsFoothold.esp") || enabled;
  plugins_note = enabled ? "plugins.txt updated" : "enable CrossroadsFoothold.esp manually";
  return true;
}

}  // namespace

std::filesystem::path foothold_staging_dir() { return settings_dir() / L"bridge"; }

FootholdBridgeResult build_and_deploy_foothold(const std::filesystem::path& f3_install,
                                               const std::filesystem::path& fnv_install) {
  FootholdBridgeResult result;
  const auto staged = foothold_staging_dir() / L"CrossroadsFoothold.esp";

  // Phase 1d: Capital pack (interior + WRLD exterior slice).
  const Phase1ImportResult pack = import_phase1_capital_pack(f3_install, staged);
  bool wrote_esp = pack.ok;
  if (pack.ok) {
    result.cell_editor_id = pack.interior_edid.empty() ? "CrossroadsFoothold" : pack.interior_edid;
    result.world_editor_id = pack.world_edid;
    result.f3_worldspace_hint = pack.world_edid.empty() ? pack.interior_edid : pack.world_edid;
    result.coc_hint = pack.coc_hint;
    result.cow_hint = pack.cow_hint;
    result.exterior_cells = pack.exterior_cells;
  } else {
    log::warn("Phase 1 Capital pack failed (" + pack.summary + "); trying interior-only import");
    const CellImportResult imported = import_capital_foothold_cell(f3_install, staged);
    if (imported.ok) {
      wrote_esp = true;
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
    }
  }

  if (!wrote_esp) {
    result.summary = "No foothold ESP produced";
    return result;
  }

  const BsaWireResult bsa = wire_fallout3_bsas_into_fnv(f3_install, fnv_install);

  std::string plugins_note;
  if (!deploy_esp(staged, fnv_install, result.esp_in_data, plugins_note)) {
    result.summary = "Failed to deploy foothold ESP into FNV Data";
    result.detail = plugins_note;
    return result;
  }

  std::ostringstream detail;
  if (pack.ok) {
    detail << pack.detail;
  } else if (!result.coc_hint.empty()) {
    detail << "fallback import path";
  }
  detail << " | " << bsa.summary << " — " << bsa.detail;
  detail << " | deployed " << wide_to_utf8(result.esp_in_data.wstring());
  detail << " | " << plugins_note;
  if (!result.coc_hint.empty()) {
    detail << " | console: " << result.coc_hint;
  }
  if (!result.cow_hint.empty()) {
    detail << " | " << result.cow_hint;
  }

  result.ok = true;
  if (pack.ok) {
    result.summary = "Phase 1 Capital pack deployed";
  } else if (result.cell_editor_id == "CrossroadsFoothold") {
    result.summary = "Foothold ESP deployed (fallback)";
  } else {
    result.summary = "Capital interior deployed (pack fallback)";
  }
  result.detail = detail.str();
  log::info(result.summary + " — " + result.detail);
  return result;
}

}  // namespace crossroads
