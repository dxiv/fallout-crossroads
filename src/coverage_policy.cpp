#include "coverage_policy.hpp"

#include "log.hpp"

namespace crossroads {

std::string hot_wasteland_name(HotWasteland hot) {
  switch (hot) {
    case HotWasteland::Capital:
      return "Capital";
    case HotWasteland::Mojave:
      return "Mojave";
    case HotWasteland::TransitNeutral:
      return "TransitNeutral";
    case HotWasteland::None:
      return "None";
  }
  return "None";
}

HotWasteland hot_for_leg(TransitLeg leg) {
  switch (leg) {
    case TransitLeg::CapitalStart:
    case TransitLeg::CapitalReturn:
    case TransitLeg::LoopComplete:
      return HotWasteland::Capital;
    case TransitLeg::MojaveArrive:
      return HotWasteland::Mojave;
    case TransitLeg::PlaneWest:
    case TransitLeg::BoatEast:
      return HotWasteland::TransitNeutral;
  }
  return HotWasteland::Capital;
}

CoveragePolicy coverage_for_journey(const JourneyState& journey) {
  CoveragePolicy p;
  p.hot = hot_for_leg(journey.leg);
  p.capital_exterior_radius = 2;
  p.capital_max_exterior_cells = 25;
  p.capital_max_refs = 2500;

  // Prefer a Goodsprings-ish exterior grid when Mojave is hot (host already has land).
  if (!journey.mojave_world_edid.empty()) {
    p.mojave_cow_x = -18;
    p.mojave_cow_y = 16;
    p.mojave_cow_valid = true;
  }

  switch (p.hot) {
    case HotWasteland::Capital:
      p.note = "Capital exterior buffers hot; treat Mojave exterior as cold (unload when engine allows)";
      break;
    case HotWasteland::Mojave:
      p.note = "Mojave exterior hot on host; Capital imported slice stays loaded but not streamed";
      break;
    case HotWasteland::TransitNeutral:
      p.note = "Transit cabin: both wasteland exteriors cold; keep only transit CELL resident";
      break;
    case HotWasteland::None:
      p.note = "No hot wasteland";
      break;
  }
  log::info("Coverage policy: hot=" + hot_wasteland_name(p.hot) + " — " + p.note);
  return p;
}

}  // namespace crossroads
