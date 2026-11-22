#include "doctor.hpp"

#include "install_detect.hpp"
#include "log.hpp"
#include "settings.hpp"
#include "transit_journey.hpp"

#include <fstream>
#include <sstream>

namespace crossroads {

DoctorReport run_doctor(const std::filesystem::path& f3_install,
                        const std::filesystem::path& fnv_install) {
  DoctorReport report;
  report.report_path = settings_dir() / L"doctor.txt";

  const InstallCheck f3 = validate_fallout3(f3_install);
  const InstallCheck fnv = validate_fallout_nv(fnv_install);
  const JourneyState journey = load_journey();

  std::ostringstream body;
  body << "Fallout: Crossroads — doctor report\n";
  body << "version " CROSSROADS_VERSION "\n\n";
  body << "OWN\n";
  body << "  Fallout 3 1.7.0.3-class: " << (f3.ok ? "OK" : "MISSING") << " — " << f3.summary << "\n";
  body << "  Fallout NV 1.4.0.525: " << (fnv.ok ? "OK" : "MISSING") << " — " << fnv.summary << "\n\n";
  body << "JOURNEY\n";
  body << "  leg=" << transit_leg_name(journey.leg) << " loops=" << journey.loop_count
       << " level=" << journey.level << " caps=" << journey.caps << "\n\n";
  body << "EXPECTED PLUGINS (in FNV Data)\n";
  body << "  CrossroadsFoothold.esp\n";
  body << "  CrossroadsTransit.esp\n";
  body << "  CrossroadsSpine.esp\n\n";
  body << "MOD POLICY\n";
  body << "  Third-party mods are untested. Disable MO2/Vortex overlays when verifying Crossroads.\n";
  body << "  Never redistribute Bethesda BSAs/ESMs.\n\n";
  body << "PERF NOTES\n";
  body << "  Keep one wasteland hot per leg (see coverage policy).\n";
  body << "  LOD/meshes come only from BYO packages (CR_FO3_* links).\n";

  std::ofstream out(report.report_path, std::ios::binary | std::ios::trunc);
  if (!out) {
    report.summary = "Failed to write doctor.txt";
    return report;
  }
  out << body.str();

  report.ok = f3.ok && fnv.ok;
  report.summary = report.ok ? "Doctor OK" : "Doctor found install issues";
  report.detail = body.str();
  log::info(report.summary + " → doctor.txt");
  return report;
}

}  // namespace crossroads
