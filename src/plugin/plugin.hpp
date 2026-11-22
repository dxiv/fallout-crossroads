#pragma once

namespace crossroads::plugin {

// Called once after the DLL is mapped into FalloutNV.exe.
void on_attach();
void on_detach();

}  // namespace crossroads::plugin
