#include "bsa_wire.hpp"

#include "log.hpp"

#include <cwctype>
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

std::filesystem::path documents_falloutnv_ini() {
  PWSTR docs = nullptr;
  if (FAILED(SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_DEFAULT, nullptr, &docs)) || !docs) {
    return {};
  }
  std::filesystem::path p = std::filesystem::path(docs) / L"My Games" / L"FalloutNV" / L"FalloutNV.ini";
  CoTaskMemFree(docs);
  return p;
}

bool link_or_copy(const std::filesystem::path& src, const std::filesystem::path& dst, std::string& how) {
  std::error_code ec;
  if (std::filesystem::exists(dst, ec)) {
    std::filesystem::remove(dst, ec);
  }
  // Prefer unprivileged symlink; fall back to hard link; last resort copy.
  if (CreateSymbolicLinkW(dst.c_str(), src.c_str(), SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE)) {
    how = "symlink";
    return true;
  }
  if (CreateHardLinkW(dst.c_str(), src.c_str(), nullptr)) {
    how = "hardlink";
    return true;
  }
  std::filesystem::copy_file(src, dst, std::filesystem::copy_options::overwrite_existing, ec);
  if (!ec) {
    how = "copy";
    return true;
  }
  how = ec.message();
  return false;
}

bool ini_append_archives(const std::filesystem::path& ini_path, const std::vector<std::string>& names) {
  if (names.empty()) {
    return false;
  }
  std::ifstream in(ini_path);
  std::string content;
  if (in) {
    std::ostringstream ss;
    ss << in.rdbuf();
    content = ss.str();
  } else {
    content = "[Archive]\r\nsResourceArchiveList2=\r\n";
  }

  auto ensure_key = [&](const std::string& key) {
    if (content.find(key) == std::string::npos) {
      if (content.find("[Archive]") == std::string::npos) {
        content += "\r\n[Archive]\r\n";
      }
      content += key + "\r\n";
    }
  };
  ensure_key("sResourceArchiveList2=");

  for (const auto& name : names) {
    if (content.find(name) != std::string::npos) {
      continue;
    }
    const auto pos = content.find("sResourceArchiveList2=");
    if (pos == std::string::npos) {
      continue;
    }
    const auto line_end = content.find_first_of("\r\n", pos);
    const auto value_start = pos + std::string("sResourceArchiveList2=").size();
    std::string line =
        line_end == std::string::npos ? content.substr(value_start) : content.substr(value_start, line_end - value_start);
    if (!line.empty() && line.back() != ',') {
      line.push_back(',');
    }
    line += name;
    if (line_end == std::string::npos) {
      content = content.substr(0, value_start) + line;
    } else {
      content = content.substr(0, value_start) + line + content.substr(line_end);
    }
  }

  std::ofstream out(ini_path, std::ios::binary | std::ios::trunc);
  if (!out) {
    return false;
  }
  out.write(content.data(), static_cast<std::streamsize>(content.size()));
  return static_cast<bool>(out);
}

}  // namespace

BsaWireResult wire_fallout3_bsas_into_fnv(const std::filesystem::path& f3_install,
                                          const std::filesystem::path& fnv_install) {
  BsaWireResult result;
  const auto f3_data = f3_install / L"Data";
  const auto fnv_data = fnv_install / L"Data";
  if (!std::filesystem::is_directory(f3_data) || !std::filesystem::is_directory(fnv_data)) {
    result.summary = "Data folders missing for BSA wiring";
    return result;
  }

  std::ostringstream detail;
  std::error_code ec;
  for (const auto& entry : std::filesystem::directory_iterator(f3_data, ec)) {
    if (!entry.is_regular_file()) {
      continue;
    }
    auto ext = entry.path().extension().wstring();
    for (auto& c : ext) {
      c = static_cast<wchar_t>(towlower(c));
    }
    if (ext != L".bsa") {
      continue;
    }
    const std::string leaf = "CR_FO3_" + wide_to_utf8(entry.path().filename().wstring());
    const auto dst = fnv_data / utf8_to_wide(leaf);
    std::string how;
    if (!link_or_copy(entry.path(), dst, how)) {
      detail << "fail " << leaf << " (" << how << "); ";
      continue;
    }
    result.linked_names.push_back(leaf);
    detail << how << " " << leaf << "; ";
  }

  if (result.linked_names.empty()) {
    result.summary = "No Fallout 3 BSAs linked";
    result.detail = detail.str();
    return result;
  }

  const auto ini = documents_falloutnv_ini();
  bool ini_ok = false;
  if (!ini.empty()) {
    ini_ok = ini_append_archives(ini, result.linked_names);
  }

  result.ok = true;
  result.summary = "Wired " + std::to_string(result.linked_names.size()) + " F3 BSA(s) into FNV Data";
  result.detail = detail.str() + (ini_ok ? "ini updated" : "ini update skipped/failed");
  log::info(result.summary + " — " + result.detail);
  return result;
}

}  // namespace crossroads
