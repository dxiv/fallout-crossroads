#include "install_detect.hpp"

#include "log.hpp"

#include <cstddef>
#include <sstream>
#include <string>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace crossroads {
namespace {

std::wstring path_to_wide(const std::filesystem::path& p) { return p.wstring(); }

bool file_exists(const std::filesystem::path& p) {
  std::error_code ec;
  return std::filesystem::is_regular_file(p, ec);
}

bool dir_exists(const std::filesystem::path& p) {
  std::error_code ec;
  return std::filesystem::is_directory(p, ec);
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

std::string read_file_version(const std::filesystem::path& exe) {
  const std::wstring wpath = path_to_wide(exe);
  DWORD handle = 0;
  const DWORD size = GetFileVersionInfoSizeW(wpath.c_str(), &handle);
  if (size == 0) {
    return {};
  }
  std::vector<std::byte> buffer(size);
  if (!GetFileVersionInfoW(wpath.c_str(), 0, size, buffer.data())) {
    return {};
  }
  VS_FIXEDFILEINFO* info = nullptr;
  UINT len = 0;
  if (!VerQueryValueW(buffer.data(), L"\\", reinterpret_cast<void**>(&info), &len) || !info) {
    return {};
  }
  std::ostringstream oss;
  oss << HIWORD(info->dwFileVersionMS) << '.' << LOWORD(info->dwFileVersionMS) << '.'
      << HIWORD(info->dwFileVersionLS) << '.' << LOWORD(info->dwFileVersionLS);
  return oss.str();
}

bool version_matches_prefix(const std::string& found, const std::string& expected) {
  if (found.empty()) {
    return false;
  }
  if (found == expected) {
    return true;
  }
  auto take3 = [](const std::string& v) {
    int dots = 0;
    size_t i = 0;
    for (; i < v.size(); ++i) {
      if (v[i] == '.' && ++dots == 3) {
        break;
      }
    }
    return v.substr(0, i);
  };
  return take3(found) == take3(expected);
}

std::filesystem::path find_exe(const std::filesystem::path& root,
                               const std::vector<const wchar_t*>& names) {
  for (const wchar_t* name : names) {
    const auto candidate = root / name;
    if (file_exists(candidate)) {
      return candidate;
    }
  }
  // Steam sometimes nests under a single child folder.
  std::error_code ec;
  if (!dir_exists(root)) {
    return {};
  }
  for (const auto& entry : std::filesystem::directory_iterator(root, ec)) {
    if (ec || !entry.is_directory()) {
      continue;
    }
    for (const wchar_t* name : names) {
      const auto candidate = entry.path() / name;
      if (file_exists(candidate)) {
        return candidate;
      }
    }
  }
  return {};
}

InstallCheck validate_common(const std::filesystem::path& root,
                             const std::vector<const wchar_t*>& exe_names,
                             const wchar_t* esm_rel,
                             const char* expected_version,
                             const char* product_label) {
  InstallCheck result;
  if (root.empty()) {
    result.summary = std::string(product_label) + ": path not set";
    result.detail = "Choose the game install folder that contains the executable.";
    return result;
  }
  if (!dir_exists(root)) {
    result.summary = std::string(product_label) + ": folder missing";
    result.detail = "Path does not exist: " + wide_to_utf8(root.wstring());
    return result;
  }

  const auto exe = find_exe(root, exe_names);
  if (exe.empty()) {
    result.summary = std::string(product_label) + ": executable not found";
    std::ostringstream detail;
    detail << "Looked under " << wide_to_utf8(root.wstring()) << " for";
    for (const wchar_t* name : exe_names) {
      detail << " " << wide_to_utf8(name);
    }
    detail << ". Pick the folder that contains that .exe (Steam: steamapps\\common\\...).";
    result.detail = detail.str();
    log::warn(result.summary + " — " + result.detail);
    return result;
  }

  const auto data = exe.parent_path() / L"Data";
  if (!dir_exists(data)) {
    result.summary = std::string(product_label) + ": Data folder missing";
    result.detail = "Expected a Data directory next to " + wide_to_utf8(exe.filename().wstring());
    return result;
  }

  const auto esm = exe.parent_path() / esm_rel;
  if (!file_exists(esm)) {
    result.summary = std::string(product_label) + ": master plugin missing";
    result.detail = "Expected " + wide_to_utf8(esm.wstring()) + " from your legal install.";
    return result;
  }

  result.version_found = read_file_version(exe);
  if (result.version_found.empty()) {
    result.summary = std::string(product_label) + ": version unknown";
    result.detail =
        "Executable and Data look present, but the PE version resource could not be read. "
        "Crossroads expects " +
        std::string(expected_version) + "-class builds.";
    log::warn(result.detail);
    return result;
  }

  if (!version_matches_prefix(result.version_found, expected_version)) {
    result.summary = std::string(product_label) + ": unsupported version " + result.version_found;
    result.detail = "Found " + result.version_found + "; expected " + std::string(expected_version) +
                    "-class. For Fallout 3, use GOG 1.7.0.3 or Steam downgraded to 1.7.0.3. "
                    "For New Vegas, use Steam 1.4.0.525.";
    log::warn(result.detail);
    return result;
  }

  result.ok = true;
  result.summary = std::string(product_label) + ": OK (" + result.version_found + ")";
  result.detail = "Validated " + wide_to_utf8(exe.wstring());
  log::info(result.summary);
  return result;
}

}  // namespace

InstallCheck validate_fallout3(const std::filesystem::path& install_root) {
  return validate_common(install_root, {L"Fallout3.exe", L"Fallout3ng.exe"}, L"Data\\Fallout3.esm",
                         "1.7.0.3", "Fallout 3");
}

InstallCheck validate_fallout_nv(const std::filesystem::path& install_root) {
  return validate_common(install_root, {L"FalloutNV.exe"}, L"Data\\FalloutNV.esm", "1.4.0.525",
                         "Fallout: New Vegas");
}

bool both_installs_ready(const InstallCheck& f3, const InstallCheck& fnv) { return f3.ok && fnv.ok; }

}  // namespace crossroads
