#include "host_launch.hpp"

#include "log.hpp"

#include <filesystem>
#include <string>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Shlwapi.h>

namespace crossroads {
namespace {

std::string narrow(const std::wstring& s) {
  if (s.empty()) {
    return {};
  }
  const int n = WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0, nullptr, nullptr);
  std::string out(n, '\0');
  WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), out.data(), n, nullptr, nullptr);
  return out;
}

bool inject_dll(HANDLE process, const std::filesystem::path& dll_path, std::string& error) {
  const std::wstring wpath = dll_path.wstring();
  const size_t bytes = (wpath.size() + 1) * sizeof(wchar_t);
  void* remote = VirtualAllocEx(process, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  if (!remote) {
    error = "VirtualAllocEx failed (" + std::to_string(GetLastError()) + ")";
    return false;
  }
  if (!WriteProcessMemory(process, remote, wpath.c_str(), bytes, nullptr)) {
    error = "WriteProcessMemory failed (" + std::to_string(GetLastError()) + ")";
    VirtualFreeEx(process, remote, 0, MEM_RELEASE);
    return false;
  }
  HMODULE kernel = GetModuleHandleW(L"kernel32.dll");
  auto load_library = reinterpret_cast<LPTHREAD_START_ROUTINE>(GetProcAddress(kernel, "LoadLibraryW"));
  if (!load_library) {
    error = "GetProcAddress(LoadLibraryW) failed";
    VirtualFreeEx(process, remote, 0, MEM_RELEASE);
    return false;
  }
  HANDLE thread = CreateRemoteThread(process, nullptr, 0, load_library, remote, 0, nullptr);
  if (!thread) {
    error = "CreateRemoteThread failed (" + std::to_string(GetLastError()) + ")";
    VirtualFreeEx(process, remote, 0, MEM_RELEASE);
    return false;
  }
  WaitForSingleObject(thread, 15000);
  DWORD code = 0;
  GetExitCodeThread(thread, &code);
  CloseHandle(thread);
  VirtualFreeEx(process, remote, 0, MEM_RELEASE);
  if (code == 0) {
    error =
        "LoadLibraryW returned null inside the host (bitness mismatch, missing DLL deps, "
        "or the host blocked the load). CrossroadsPlugin.dll must be Win32 like FalloutNV.exe.";
    return false;
  }
  return true;
}

}  // namespace

std::filesystem::path default_plugin_path_next_to_exe() {
  wchar_t module[MAX_PATH]{};
  GetModuleFileNameW(nullptr, module, MAX_PATH);
  PathRemoveFileSpecW(module);
  return std::filesystem::path(module) / L"CrossroadsPlugin.dll";
}

LaunchResult launch_fnv_with_plugin(const std::filesystem::path& fnv_install,
                                    const std::filesystem::path& plugin_dll) {
  LaunchResult result;
  const auto exe = fnv_install / L"FalloutNV.exe";
  if (!std::filesystem::is_regular_file(exe)) {
    result.summary = "FalloutNV.exe missing";
    result.detail = narrow(exe.wstring());
    return result;
  }
  if (!std::filesystem::is_regular_file(plugin_dll)) {
    result.summary = "CrossroadsPlugin.dll missing";
    result.detail = "Build the plugin and keep it next to FalloutCrossroads.exe (" +
                    narrow(plugin_dll.wstring()) + ")";
    return result;
  }

  STARTUPINFOW si{};
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi{};
  std::wstring cmd = L"\"" + exe.wstring() + L"\"";
  std::vector<wchar_t> cmd_buf(cmd.begin(), cmd.end());
  cmd_buf.push_back(L'\0');

  // Suspended start so the plugin can load before the game fully initializes.
  const DWORD flags = CREATE_SUSPENDED;
  if (!CreateProcessW(exe.c_str(), cmd_buf.data(), nullptr, nullptr, FALSE, flags, nullptr,
                      fnv_install.wstring().c_str(), &si, &pi)) {
    result.summary = "CreateProcess failed";
    result.detail = "Win32 error " + std::to_string(GetLastError()) +
                    ". If Steam owns the install, start Steam first; do not bypass DRM.";
    log::error(result.summary + ": " + result.detail);
    return result;
  }

  std::string inject_error;
  const bool injected = inject_dll(pi.hProcess, plugin_dll, inject_error);
  ResumeThread(pi.hThread);

  result.process_id = pi.dwProcessId;
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);

  if (!injected) {
    result.summary = "Host started; plugin inject failed";
    result.detail = inject_error;
    log::error(result.summary + ": " + result.detail);
    return result;
  }

  result.ok = true;
  result.summary = "Launched FalloutNV.exe with CrossroadsPlugin";
  result.detail = "pid " + std::to_string(result.process_id);
  log::info(result.summary + " (" + result.detail + ")");
  return result;
}

}  // namespace crossroads
