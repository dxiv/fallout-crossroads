#pragma once

#include <filesystem>
#include <string>

namespace crossroads {

struct DoctorReport {
  bool ok = false;
  std::string summary;
  std::string detail;
  std::filesystem::path report_path;
};

// Writes an environment / ownership / plugin checklist under AppData.
DoctorReport run_doctor(const std::filesystem::path& f3_install,
                        const std::filesystem::path& fnv_install);

}  // namespace crossroads
