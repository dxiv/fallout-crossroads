#pragma once

#include <filesystem>
#include <string>

namespace crossroads {

struct LaunchResult {
  bool ok = false;
  std::string summary;
  std::string detail;
  unsigned long process_id = 0;
};

// Launches FalloutNV.exe from the BYO install and injects CrossroadsPlugin.dll.
// plugin_dll should be the full path to the built CrossroadsPlugin.dll (usually
// next to FalloutCrossroads.exe).
LaunchResult launch_fnv_with_plugin(const std::filesystem::path& fnv_install,
                                    const std::filesystem::path& plugin_dll);

std::filesystem::path default_plugin_path_next_to_exe();

}  // namespace crossroads
