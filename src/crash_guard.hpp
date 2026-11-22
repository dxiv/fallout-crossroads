#pragma once

namespace crossroads {

// Installs process-wide crash logging (unhandled SEH → crash.log under AppData).
void install_crash_guard();

}  // namespace crossroads
