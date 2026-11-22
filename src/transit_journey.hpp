#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace crossroads {

enum class TransitLeg {
  CapitalStart = 0,
  PlaneWest,
  MojaveArrive,
  BoatEast,
  CapitalReturn,
  LoopComplete,
};

struct JourneyState {
  TransitLeg leg = TransitLeg::CapitalStart;
  int loop_count = 0;
  // Crossroads-owned session stub (not Bethesda save bytes).
  int level = 1;
  int caps = 0;
  std::vector<std::string> inventory_flags;  // e.g. "has_ticket", "dock_key"
  std::string capital_cell_edid;
  std::string capital_cow_hint;
  std::string mojave_cell_edid;
  std::string mojave_world_edid;
  std::string last_note;
};

std::string transit_leg_name(TransitLeg leg);
TransitLeg next_transit_leg(TransitLeg leg);

std::filesystem::path journey_path();
JourneyState load_journey();
bool save_journey(const JourneyState& state);

// Advances one leg and persists. Returns the new state.
JourneyState advance_journey(JourneyState state);

struct LegPlaybook {
  std::string title;
  std::string summary;
  std::string console_hint;  // coc / cow lines for the reviewer
  std::string next_action;
};

LegPlaybook playbook_for(const JourneyState& state);

}  // namespace crossroads
