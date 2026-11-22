#include "crash_guard.hpp"

#include "log.hpp"
#include "settings.hpp"

#include <cstdio>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace crossroads {
namespace {

std::wstring crash_log_path() { return (settings_dir() / L"crash.log").wstring(); }

LONG WINAPI crossroads_unhandled(EXCEPTION_POINTERS* info) {
  const auto path = crash_log_path();
  FILE* f = nullptr;
  if (_wfopen_s(&f, path.c_str(), L"a") == 0 && f) {
    SYSTEMTIME st{};
    GetLocalTime(&st);
    std::fprintf(f, "\n==== Crossroads crash %04u-%02u-%02u %02u:%02u:%02u ====\n", st.wYear, st.wMonth,
                 st.wDay, st.wHour, st.wMinute, st.wSecond);
    if (info && info->ExceptionRecord) {
      std::fprintf(f, "code=0x%08lX addr=%p\n",
                   static_cast<unsigned long>(info->ExceptionRecord->ExceptionCode),
                   info->ExceptionRecord->ExceptionAddress);
    }
    std::fprintf(f, "See also crossroads.log. Do not attach third-party mods until stable.\n");
    std::fclose(f);
  }
  try {
    log::error("Unhandled exception — details appended to crash.log");
  } catch (...) {
  }
  return EXCEPTION_CONTINUE_SEARCH;
}

}  // namespace

void install_crash_guard() {
  SetUnhandledExceptionFilter(crossroads_unhandled);
  log::info("Crash guard installed → crash.log");
}

}  // namespace crossroads
