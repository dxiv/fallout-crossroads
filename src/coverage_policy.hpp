#pragma once

#include "transit_journey.hpp"

#include <cstddef>
#include <string>

namespace crossroads {

enum class HotWasteland {
  None,
  Capital,
  Mojave,
  TransitNeutral,
};

struct CoveragePolicy {
  HotWasteland hot = HotWasteland::Capital;
  // Capital exterior import budget (Phase 3 expands beyond the 1d 3×3 spike).
  std::size_t capital_exterior_radius = 2;  // cells from densest: 2 → up to 5×5
  std::size_t capital_max_exterior_cells = 25;
  std::size_t capital_max_refs = 2500;
  // Mojave is host-native; we only track which foothold grid to prefer.
  int mojave_cow_x = 0;
  int mojave_cow_y = 0;
  bool mojave_cow_valid = false;
  std::string note;
};

std::string hot_wasteland_name(HotWasteland hot);
HotWasteland hot_for_leg(TransitLeg leg);

// Builds the active coverage policy for the current journey leg.
CoveragePolicy coverage_for_journey(const JourneyState& journey);

}  // namespace crossroads
