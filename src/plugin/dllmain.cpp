#include "plugin.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

BOOL APIENTRY DllMain(HMODULE, DWORD reason, LPVOID) {
  switch (reason) {
    case DLL_PROCESS_ATTACH:
      crossroads::plugin::on_attach();
      break;
    case DLL_PROCESS_DETACH:
      crossroads::plugin::on_detach();
      break;
    default:
      break;
  }
  return TRUE;
}
