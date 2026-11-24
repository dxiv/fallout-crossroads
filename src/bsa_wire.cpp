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

bool read_ini_file(const std::filesystem::path& ini_path, std::string& content) {
  std::ifstream in(ini_path);
  if (!in) {
    return false;
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  content = ss.str();
  return true;
}

bool write_ini_file(const std::filesystem::path& ini_path, const std::string& content) {
  std::ofstream out(ini_path, std::ios::binary | std::ios::trunc);
  if (!out) {
    return false;
  }
  out.write(content.data(), static_cast<std::streamsize>(content.size()));
  return static_cast<bool>(out);
}

bool rewrite_archive_list_value(std::string& content, const std::string& key,
                                const std::string& new_value) {
  const auto pos = content.find(key);
  if (pos == std::string::npos) {
    return false;
  }
  const auto value_start = pos + key.size();
  const auto line_end = content.find_first_of("\r\n", value_start);
  if (line_end == std::string::npos) {
    content = content.substr(0, value_start) + new_value;
  } else {
    content = content.substr(0, value_start) + new_value + content.substr(line_end);
  }
  return true;
}

std::string read_archive_list_value(const std::string& content, const std::string& key) {
  const auto pos = content.find(key);
  if (pos == std::string::npos) {
    return {};
  }
  const auto value_start = pos + key.size();
  const auto line_end = content.find_first_of("\r\n", value_start);
  return line_end == std::string::npos ? content.substr(value_start)
                                       : content.substr(value_start, line_end - value_start);
}

std::vector<std::string> split_archive_list(const std::string& line) {
  std::vector<std::string> out;
  std::string cur;
  for (char c : line) {
    if (c == ',') {
      while (!cur.empty() && (cur.front() == ' ' || cur.front() == '\t')) {
        cur.erase(cur.begin());
      }
      while (!cur.empty() && (cur.back() == ' ' || cur.back() == '\t')) {
        cur.pop_back();
      }
      if (!cur.empty()) {
        out.push_back(cur);
      }
      cur.clear();
      continue;
    }
    cur.push_back(c);
  }
  while (!cur.empty() && (cur.front() == ' ' || cur.front() == '\t')) {
    cur.erase(cur.begin());
  }
  while (!cur.empty() && (cur.back() == ' ' || cur.back() == '\t')) {
    cur.pop_back();
  }
  if (!cur.empty()) {
    out.push_back(cur);
  }
  return out;
}

std::string join_archive_list(const std::vector<std::string>& names) {
  std::ostringstream ss;
  for (std::size_t i = 0; i < names.size(); ++i) {
    if (i) {
      ss << ',';
    }
    ss << names[i];
  }
  return ss.str();
}

bool is_crossroads_fo3_archive(const std::string& name) {
  return name.size() >= 7 && name.compare(0, 7, "CR_FO3_") == 0;
}

bool ini_append_archives(const std::filesystem::path& ini_path, const std::vector<std::string>& names) {
  if (names.empty()) {
    return false;
  }
  std::string content;
  if (!read_ini_file(ini_path, content)) {
    content = "[Archive]\r\nsResourceArchiveList2=\r\n";
  }

  if (content.find("sResourceArchiveList2=") == std::string::npos) {
    if (content.find("[Archive]") == std::string::npos) {
      content += "\r\n[Archive]\r\n";
    }
    content += "sResourceArchiveList2=\r\n";
  }

  auto entries = split_archive_list(read_archive_list_value(content, "sResourceArchiveList2="));
  for (const auto& name : names) {
    bool present = false;
    for (const auto& e : entries) {
      if (e == name) {
        present = true;
        break;
      }
    }
    if (!present) {
      entries.push_back(name);
    }
  }
  rewrite_archive_list_value(content, "sResourceArchiveList2=", join_archive_list(entries));
  return write_ini_file(ini_path, content);
}

bool ini_remove_crossroads_fo3_archives(const std::filesystem::path& ini_path, int& removed) {
  removed = 0;
  std::string content;
  if (!read_ini_file(ini_path, content)) {
    return false;
  }
  bool changed = false;
  for (const char* key : {"sResourceArchiveList2=", "sResourceArchiveList="}) {
    if (content.find(key) == std::string::npos) {
      continue;
    }
    auto entries = split_archive_list(read_archive_list_value(content, key));
    std::vector<std::string> kept;
    kept.reserve(entries.size());
    for (const auto& e : entries) {
      if (is_crossroads_fo3_archive(e)) {
        ++removed;
        changed = true;
      } else {
        kept.push_back(e);
      }
    }
    rewrite_archive_list_value(content, key, join_archive_list(kept));
  }
  if (!changed) {
    return true;
  }
  return write_ini_file(ini_path, content);
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

BsaWireResult unwire_fallout3_bsas_from_fnv(const std::filesystem::path& fnv_install) {
  BsaWireResult result;
  const auto fnv_data = fnv_install / L"Data";
  std::ostringstream detail;
  int removed_files = 0;
  std::error_code ec;
  if (std::filesystem::is_directory(fnv_data, ec)) {
    for (const auto& entry : std::filesystem::directory_iterator(fnv_data, ec)) {
      if (!entry.is_regular_file() && !entry.is_symlink()) {
        continue;
      }
      const std::string name = wide_to_utf8(entry.path().filename().wstring());
      if (!is_crossroads_fo3_archive(name)) {
        continue;
      }
      std::filesystem::remove(entry.path(), ec);
      if (ec) {
        detail << "fail remove " << name << " (" << ec.message() << "); ";
        ec.clear();
        continue;
      }
      ++removed_files;
      result.linked_names.push_back(name);
    }
  }

  int removed_ini = 0;
  bool ini_ok = false;
  const auto ini = documents_falloutnv_ini();
  if (!ini.empty()) {
    ini_ok = ini_remove_crossroads_fo3_archives(ini, removed_ini);
  }

  result.ok = true;
  result.summary = "Unwired Fallout 3 BSA overlays from FNV";
  detail << "removed " << removed_files << " CR_FO3_* file(s); ";
  detail << "stripped " << removed_ini << " ini archive entr" << (removed_ini == 1 ? "y" : "ies");
  detail << (ini_ok ? "" : " (ini update skipped/failed)");
  result.detail = detail.str();
  log::info(result.summary + " — " + result.detail);
  return result;
}

}  // namespace crossroads
