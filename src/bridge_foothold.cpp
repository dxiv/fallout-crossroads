#include "bridge_foothold.hpp"

#include "esm_probe.hpp"
#include "esp_writer.hpp"
#include "log.hpp"
#include "settings.hpp"

#include <fstream>
#include <sstream>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <ShlObj.h>

namespace crossroads {
namespace {

std::wstring utf8_to_wide(const std::string& s) {
  if (s.empty()) {
    return {};
  }
  const int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
  std::wstring out(n, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), out.data(), n);
  return out;
}

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
    // Minimal vanilla-style list so we do not enable only our ESP in an empty file.
    out << "FalloutNV.esm\n";
  } else {
    out << "\n";
  }
  out << esp_name << "\n";
  return static_cast<bool>(out);
}

std::string pick_worldspace_hint(const std::filesystem::path& f3_install) {
  const auto probe = probe_fallout3_worldspaces(f3_install, 64);
  if (!probe.ok || probe.worldspaces.empty()) {
    return {};
  }
  // Prefer Capital Wasteland-style names when present.
  for (const auto& w : probe.worldspaces) {
    const auto& id = w.editor_id;
    if (id.find("Wasteland") != std::string::npos || id.find("Capital") != std::string::npos ||
        id.find("DCWorld") != std::string::npos || id.find("Megaton") != std::string::npos) {
      return id;
    }
  }
  return probe.worldspaces.front().editor_id;
}

}  // namespace

std::filesystem::path foothold_staging_dir() { return settings_dir() / L"bridge"; }

FootholdBridgeResult build_and_deploy_foothold(const std::filesystem::path& f3_install,
                                               const std::filesystem::path& fnv_install) {
  FootholdBridgeResult result;
  result.cell_editor_id = "CrossroadsFoothold";
  result.f3_worldspace_hint = pick_worldspace_hint(f3_install);

  const auto staged = foothold_staging_dir() / L"CrossroadsFoothold.esp";
  const auto written = write_foothold_esp(staged, result.cell_editor_id);
  if (!written.ok) {
    result.summary = written.summary;
    result.detail = written.detail;
    return result;
  }

  const auto data_dir = fnv_install / L"Data";
  if (!std::filesystem::is_directory(data_dir)) {
    result.summary = "FNV Data folder missing";
    result.detail = wide_to_utf8(data_dir.wstring());
    return result;
  }

  result.esp_in_data = data_dir / L"CrossroadsFoothold.esp";
  std::error_code ec;
  std::filesystem::copy_file(staged, result.esp_in_data, std::filesystem::copy_options::overwrite_existing, ec);
  if (ec) {
    result.summary = "Failed to copy foothold ESP into FNV Data";
    result.detail = ec.message();
    log::error(result.summary + ": " + result.detail);
    return result;
  }

  // Prefer the per-user plugins list; also try Data\plugins.txt as a fallback.
  bool enabled = false;
  const auto local = local_appdata() / L"FalloutNV" / L"plugins.txt";
  if (!local.empty()) {
    enabled = ensure_plugin_enabled(local, "CrossroadsFoothold.esp") || enabled;
  }
  enabled = ensure_plugin_enabled(data_dir / L"plugins.txt", "CrossroadsFoothold.esp") || enabled;

  std::ostringstream detail;
  detail << "Deployed " << wide_to_utf8(result.esp_in_data.wstring());
  detail << " | cell EDID " << result.cell_editor_id;
  if (!result.f3_worldspace_hint.empty()) {
    detail << " | F3 worldspace hint (BYO): " << result.f3_worldspace_hint;
  }
  detail << " | plugins.txt " << (enabled ? "updated" : "update failed — enable CrossroadsFoothold.esp manually");
  detail << " | in-game: coc " << result.cell_editor_id;

  result.ok = true;
  result.summary = "Foothold bridge deployed";
  result.detail = detail.str();
  log::info(result.summary + " — " + result.detail);
  (void)utf8_to_wide;
  return result;
}

}  // namespace crossroads
