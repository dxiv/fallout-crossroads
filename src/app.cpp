#include "app.hpp"

#include "install_detect.hpp"
#include "legal.hpp"
#include "log.hpp"
#include "resource.h"
#include "settings.hpp"

#include <commctrl.h>
#include <shobjidl.h>
#include <windowsx.h>

#include <filesystem>
#include <string>
#include <string_view>

namespace crossroads {
namespace {

Settings g_settings;
InstallCheck g_f3;
InstallCheck g_fnv;
HWND g_log_box = nullptr;

std::wstring utf8_to_wide(const std::string& s) {
  if (s.empty()) {
    return {};
  }
  const int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
  std::wstring out(n, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), out.data(), n);
  return out;
}

std::string wide_to_utf8(std::wstring_view s) {
  if (s.empty()) {
    return {};
  }
  const int n = WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0, nullptr, nullptr);
  std::string out(n, '\0');
  WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), out.data(), n, nullptr, nullptr);
  return out;
}

void append_log_line(HWND list, const std::wstring& line) {
  if (!list) {
    return;
  }
  const int index = ListBox_AddString(list, line.c_str());
  ListBox_SetCurSel(list, index);
}

void ui_log_sink(log::Level level, std::string_view message) {
  if (!g_log_box) {
    return;
  }
  const wchar_t* tag = L"INFO";
  switch (level) {
    case log::Level::Warn:
      tag = L"WARN";
      break;
    case log::Level::Error:
      tag = L"ERROR";
      break;
    case log::Level::Info:
      break;
  }
  append_log_line(g_log_box, std::wstring(tag) + L"  " + utf8_to_wide(std::string(message)));
}

bool browse_for_folder(HWND owner, const wchar_t* title, std::filesystem::path& out_path) {
  IFileDialog* dialog = nullptr;
  HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));
  if (FAILED(hr) || !dialog) {
    return false;
  }
  DWORD options = 0;
  dialog->GetOptions(&options);
  dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
  dialog->SetTitle(title);
  hr = dialog->Show(owner);
  if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
    dialog->Release();
    return false;
  }
  if (FAILED(hr)) {
    dialog->Release();
    return false;
  }
  IShellItem* item = nullptr;
  hr = dialog->GetResult(&item);
  dialog->Release();
  if (FAILED(hr) || !item) {
    return false;
  }
  PWSTR path = nullptr;
  hr = item->GetDisplayName(SIGDN_FILESYSPATH, &path);
  item->Release();
  if (FAILED(hr) || !path) {
    return false;
  }
  out_path = path;
  CoTaskMemFree(path);
  return true;
}

void set_window_text(HWND dlg, int id, const std::wstring& text) {
  SetDlgItemTextW(dlg, id, text.c_str());
}

std::wstring get_window_text(HWND dlg, int id) {
  const int len = GetWindowTextLengthW(GetDlgItem(dlg, id));
  std::wstring text(static_cast<size_t>(len) + 1, L'\0');
  GetDlgItemTextW(dlg, id, text.data(), len + 1);
  text.resize(wcslen(text.c_str()));
  return text;
}

void refresh_status(HWND dlg) {
  g_settings.fallout3_install = get_window_text(dlg, IDC_F3_PATH);
  g_settings.fallout_nv_install = get_window_text(dlg, IDC_FNV_PATH);

  g_f3 = validate_fallout3(g_settings.fallout3_install);
  g_fnv = validate_fallout_nv(g_settings.fallout_nv_install);

  set_window_text(dlg, IDC_F3_STATUS, utf8_to_wide(g_f3.summary));
  set_window_text(dlg, IDC_FNV_STATUS, utf8_to_wide(g_fnv.summary));

  const bool ready = both_installs_ready(g_f3, g_fnv);
  EnableWindow(GetDlgItem(dlg, IDC_CONTINUE), ready ? TRUE : FALSE);
  if (ready) {
    set_window_text(dlg, IDC_GATE_NOTE,
                    L"Both installs validated. Continue unlocks the Phase 0 menu shell (no game world yet).");
  } else {
    set_window_text(dlg, IDC_GATE_NOTE,
                    L"Crossroads refuses to continue until BOTH legal installs validate. "
                    L"No Bethesda files are copied into this project.");
  }
}

void load_paths_into_ui(HWND dlg) {
  set_window_text(dlg, IDC_F3_PATH, g_settings.fallout3_install.wstring());
  set_window_text(dlg, IDC_FNV_PATH, g_settings.fallout_nv_install.wstring());
  refresh_status(dlg);
}

void show_menu_shell(HWND owner) {
  MessageBoxW(owner,
              L"Phase 0 menu shell\r\n"
              L"\r\n"
              L"Install gate passed. There is no playable world yet.\r\n"
              L"Next milestone: FNV-hosted bridge and a Capital Wasteland "
              L"foothold. See README.md for the roadmap.",
              L"Fallout: Crossroads", MB_OK | MB_ICONINFORMATION);
}

INT_PTR CALLBACK MainProc(HWND dlg, UINT msg, WPARAM wparam, LPARAM lparam) {
  switch (msg) {
    case WM_INITDIALOG: {
      g_log_box = GetDlgItem(dlg, IDC_LOG);
      log::set_sink(ui_log_sink);
      InitCommonControls();
      set_window_text(dlg, IDC_TITLE, L"Fallout: Crossroads  —  Phase 0 boot");
      g_settings = load_settings();
      load_paths_into_ui(dlg);
      log::info("Boot UI ready.");
      return TRUE;
    }
    case WM_COMMAND: {
      switch (LOWORD(wparam)) {
        case IDC_F3_BROWSE: {
          std::filesystem::path chosen;
          if (browse_for_folder(dlg, L"Select Fallout 3 install folder", chosen)) {
            set_window_text(dlg, IDC_F3_PATH, chosen.wstring());
            refresh_status(dlg);
          }
          return TRUE;
        }
        case IDC_FNV_BROWSE: {
          std::filesystem::path chosen;
          if (browse_for_folder(dlg, L"Select Fallout: New Vegas install folder", chosen)) {
            set_window_text(dlg, IDC_FNV_PATH, chosen.wstring());
            refresh_status(dlg);
          }
          return TRUE;
        }
        case IDC_SAVE: {
          g_settings.fallout3_install = get_window_text(dlg, IDC_F3_PATH);
          g_settings.fallout_nv_install = get_window_text(dlg, IDC_FNV_PATH);
          if (save_settings(g_settings)) {
            MessageBoxW(dlg, L"Settings saved under %AppData%\\FalloutCrossroads\\", L"Saved",
                        MB_OK | MB_ICONINFORMATION);
          } else {
            MessageBoxW(dlg, L"Could not write settings.json.", L"Error", MB_OK | MB_ICONERROR);
          }
          return TRUE;
        }
        case IDC_RECHECK:
          refresh_status(dlg);
          return TRUE;
        case IDC_LEGAL:
          show_legal_dialog(dlg);
          return TRUE;
        case IDC_CREDITS:
          MessageBoxW(dlg, credits_text(), L"Credits — Fallout: Crossroads", MB_OK | MB_ICONINFORMATION);
          return TRUE;
        case IDC_CONTINUE:
          refresh_status(dlg);
          if (!both_installs_ready(g_f3, g_fnv)) {
            MessageBoxW(dlg,
                        L"Both Fallout 3 (1.7.0.3-class) and Fallout: New Vegas (1.4.0.525) "
                        L"must validate before continuing.",
                        L"Install gate", MB_OK | MB_ICONWARNING);
            return TRUE;
          }
          save_settings(g_settings);
          show_menu_shell(dlg);
          return TRUE;
        case IDC_QUIT:
        case IDCANCEL:
          EndDialog(dlg, 0);
          return TRUE;
        default:
          break;
      }
      break;
    }
    case WM_CLOSE:
      EndDialog(dlg, 0);
      return TRUE;
    default:
      break;
  }
  return FALSE;
}

}  // namespace

int run_app(HINSTANCE instance, int /*show_command*/) {
  HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
  if (FAILED(hr)) {
    return 1;
  }

  log::init(log_path());
  log::info("Fallout: Crossroads boot " CROSSROADS_VERSION);

  const INT_PTR result = DialogBoxParamW(instance, MAKEINTRESOURCEW(IDD_MAIN), nullptr, MainProc, 0);
  if (result == -1) {
    log::error("DialogBoxParam failed.");
  }

  log::shutdown();
  CoUninitialize();
  return result == -1 ? 1 : 0;
}

}  // namespace crossroads
