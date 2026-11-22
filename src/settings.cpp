#include "settings.hpp"

#include "log.hpp"

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

std::string json_escape(const std::string& s) {
  std::string out;
  out.reserve(s.size() + 8);
  for (char c : s) {
    switch (c) {
      case '\\':
        out += "\\\\";
        break;
      case '"':
        out += "\\\"";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        out += c;
        break;
    }
  }
  return out;
}

// Minimal extract of a JSON string value for a known key: "key": "value"
std::string extract_string_field(const std::string& json, const std::string& key) {
  const std::string needle = "\"" + key + "\"";
  const auto key_pos = json.find(needle);
  if (key_pos == std::string::npos) {
    return {};
  }
  const auto colon = json.find(':', key_pos + needle.size());
  if (colon == std::string::npos) {
    return {};
  }
  const auto first_quote = json.find('"', colon + 1);
  if (first_quote == std::string::npos) {
    return {};
  }
  std::string value;
  for (size_t i = first_quote + 1; i < json.size(); ++i) {
    if (json[i] == '\\' && i + 1 < json.size()) {
      value.push_back(json[i + 1]);
      ++i;
      continue;
    }
    if (json[i] == '"') {
      break;
    }
    value.push_back(json[i]);
  }
  return value;
}

}  // namespace

std::filesystem::path settings_dir() {
  PWSTR appdata = nullptr;
  if (FAILED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, nullptr, &appdata)) || !appdata) {
    return std::filesystem::current_path() / L"FalloutCrossroads";
  }
  std::filesystem::path dir = std::filesystem::path(appdata) / L"FalloutCrossroads";
  CoTaskMemFree(appdata);
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);
  return dir;
}

std::filesystem::path settings_path() { return settings_dir() / L"settings.json"; }
std::filesystem::path log_path() { return settings_dir() / L"crossroads.log"; }

Settings load_settings() {
  Settings s;
  const auto path = settings_path();
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    log::info("No settings file yet; using empty install paths.");
    return s;
  }
  std::ostringstream oss;
  oss << in.rdbuf();
  const std::string json = oss.str();
  const auto f3 = extract_string_field(json, "fallout3_install");
  const auto fnv = extract_string_field(json, "fallout_nv_install");
  if (!f3.empty()) {
    s.fallout3_install = utf8_to_wide(f3);
  }
  if (!fnv.empty()) {
    s.fallout_nv_install = utf8_to_wide(fnv);
  }
  log::info("Loaded settings from " + wide_to_utf8(path.wstring()));
  return s;
}

bool save_settings(const Settings& settings) {
  const auto path = settings_path();
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out) {
    log::error("Failed to write settings file.");
    return false;
  }
  const std::string f3 = wide_to_utf8(settings.fallout3_install.wstring());
  const std::string fnv = wide_to_utf8(settings.fallout_nv_install.wstring());
  out << "{\n"
      << "  \"fallout3_install\": \"" << json_escape(f3) << "\",\n"
      << "  \"fallout_nv_install\": \"" << json_escape(fnv) << "\"\n"
      << "}\n";
  log::info("Saved settings.");
  return true;
}

}  // namespace crossroads
