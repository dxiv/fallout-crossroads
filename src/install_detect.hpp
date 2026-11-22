#pragma once

#include <filesystem>
#include <string>

namespace crossroads {

struct InstallCheck {
  bool ok = false;
  std::string summary;      // short status for UI
  std::string detail;       // longer explanation
  std::string version_found;
};

// Fallout 3 PC 1.7.0.3-class (GOG 1.7.0.3 or Steam downgraded to 1.7.0.3).
InstallCheck validate_fallout3(const std::filesystem::path& install_root);

// Fallout: New Vegas Steam PC 1.4.0.525.
InstallCheck validate_fallout_nv(const std::filesystem::path& install_root);

bool both_installs_ready(const InstallCheck& f3, const InstallCheck& fnv);

}  // namespace crossroads
