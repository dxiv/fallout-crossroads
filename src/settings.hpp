#pragma once

#include <filesystem>
#include <string>

namespace crossroads {

struct Settings {
  std::filesystem::path fallout3_install;
  std::filesystem::path fallout_nv_install;
};

std::filesystem::path settings_dir();
std::filesystem::path settings_path();
std::filesystem::path log_path();

Settings load_settings();
bool save_settings(const Settings& settings);

}  // namespace crossroads
