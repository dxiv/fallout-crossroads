#include "transit_journey.hpp"

#include "log.hpp"
#include "settings.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace crossroads {
namespace {

std::string extract_string_field(const std::string& json, const std::string& key) {
  const std::string needle = "\"" + key + "\"";
  const auto key_pos = json.find(needle);
  if (key_pos == std::string::npos) {
    return {};
  }
  const auto colon = json.find(':', key_pos + needle.size());
  if (colon == std::string::npos) {
    return {};
  }
  const auto first_quote = json.find('"', colon + 1);
  if (first_quote == std::string::npos) {
    return {};
  }
  std::string value;
  for (size_t i = first_quote + 1; i < json.size(); ++i) {
    if (json[i] == '\\' && i + 1 < json.size()) {
      value.push_back(json[i + 1]);
      ++i;
      continue;
    }
    if (json[i] == '"') {
      break;
    }
    value.push_back(json[i]);
  }
  return value;
}

int extract_int_field(const std::string& json, const std::string& key, int fallback) {
  const std::string needle = "\"" + key + "\"";
  const auto key_pos = json.find(needle);
  if (key_pos == std::string::npos) {
    return fallback;
  }
  const auto colon = json.find(':', key_pos + needle.size());
  if (colon == std::string::npos) {
    return fallback;
  }
  try {
    return std::stoi(json.substr(colon + 1));
  } catch (...) {
    return fallback;
  }
}

std::string json_escape(const std::string& s) {
  std::string out;
  for (char c : s) {
    switch (c) {
      case '\\':
        out += "\\\\";
        break;
      case '"':
        out += "\\\"";
        break;
      case '\n':
        out += "\\n";
        break;
      default:
        out += c;
        break;
    }
  }
  return out;
}

std::string join_flags(const std::vector<std::string>& flags) {
  std::ostringstream oss;
  for (std::size_t i = 0; i < flags.size(); ++i) {
    if (i) {
      oss << ",";
    }
    oss << flags[i];
  }
  return oss.str();
}

std::vector<std::string> split_flags(const std::string& csv) {
  std::vector<std::string> out;
  std::string cur;
  for (char c : csv) {
    if (c == ',') {
      if (!cur.empty()) {
        out.push_back(cur);
        cur.clear();
      }
    } else {
      cur.push_back(c);
    }
  }
  if (!cur.empty()) {
    out.push_back(cur);
  }
  return out;
}

TransitLeg leg_from_string(const std::string& s) {
  if (s == "PlaneWest") {
    return TransitLeg::PlaneWest;
  }
  if (s == "MojaveArrive") {
    return TransitLeg::MojaveArrive;
  }
  if (s == "BoatEast") {
    return TransitLeg::BoatEast;
  }
  if (s == "CapitalReturn") {
    return TransitLeg::CapitalReturn;
  }
  if (s == "LoopComplete") {
    return TransitLeg::LoopComplete;
  }
  return TransitLeg::CapitalStart;
}

}  // namespace

std::string transit_leg_name(TransitLeg leg) {
  switch (leg) {
    case TransitLeg::CapitalStart:
      return "CapitalStart";
    case TransitLeg::PlaneWest:
      return "PlaneWest";
    case TransitLeg::MojaveArrive:
      return "MojaveArrive";
    case TransitLeg::BoatEast:
      return "BoatEast";
    case TransitLeg::CapitalReturn:
      return "CapitalReturn";
    case TransitLeg::LoopComplete:
      return "LoopComplete";
  }
  return "CapitalStart";
}

TransitLeg next_transit_leg(TransitLeg leg) {
  switch (leg) {
    case TransitLeg::CapitalStart:
      return TransitLeg::PlaneWest;
    case TransitLeg::PlaneWest:
      return TransitLeg::MojaveArrive;
    case TransitLeg::MojaveArrive:
      return TransitLeg::BoatEast;
    case TransitLeg::BoatEast:
      return TransitLeg::CapitalReturn;
    case TransitLeg::CapitalReturn:
      return TransitLeg::LoopComplete;
    case TransitLeg::LoopComplete:
      return TransitLeg::CapitalStart;
  }
  return TransitLeg::CapitalStart;
}

std::filesystem::path journey_path() { return settings_dir() / L"journey.json"; }

JourneyState load_journey() {
  JourneyState s;
  std::ifstream in(journey_path(), std::ios::binary);
  if (!in) {
    return s;
  }
  std::ostringstream oss;
  oss << in.rdbuf();
  const std::string json = oss.str();
  s.leg = leg_from_string(extract_string_field(json, "leg"));
  s.loop_count = extract_int_field(json, "loop_count", 0);
  s.level = extract_int_field(json, "level", 1);
  s.caps = extract_int_field(json, "caps", 0);
  s.capital_cell_edid = extract_string_field(json, "capital_cell_edid");
  s.capital_cow_hint = extract_string_field(json, "capital_cow_hint");
  s.mojave_cell_edid = extract_string_field(json, "mojave_cell_edid");
  s.mojave_world_edid = extract_string_field(json, "mojave_world_edid");
  s.last_note = extract_string_field(json, "last_note");
  s.inventory_flags = split_flags(extract_string_field(json, "inventory_flags"));
  return s;
}

bool save_journey(const JourneyState& state) {
  std::ofstream out(journey_path(), std::ios::binary | std::ios::trunc);
  if (!out) {
    log::error("Failed to write journey.json");
    return false;
  }
  out << "{\n"
      << "  \"leg\": \"" << json_escape(transit_leg_name(state.leg)) << "\",\n"
      << "  \"loop_count\": " << state.loop_count << ",\n"
      << "  \"level\": " << state.level << ",\n"
      << "  \"caps\": " << state.caps << ",\n"
      << "  \"inventory_flags\": \"" << json_escape(join_flags(state.inventory_flags)) << "\",\n"
      << "  \"capital_cell_edid\": \"" << json_escape(state.capital_cell_edid) << "\",\n"
      << "  \"capital_cow_hint\": \"" << json_escape(state.capital_cow_hint) << "\",\n"
      << "  \"mojave_cell_edid\": \"" << json_escape(state.mojave_cell_edid) << "\",\n"
      << "  \"mojave_world_edid\": \"" << json_escape(state.mojave_world_edid) << "\",\n"
      << "  \"last_note\": \"" << json_escape(state.last_note) << "\"\n"
      << "}\n";
  return true;
}

JourneyState advance_journey(JourneyState state) {
  const TransitLeg prev = state.leg;
  state.leg = next_transit_leg(state.leg);
  if (prev == TransitLeg::LoopComplete) {
    ++state.loop_count;
    state.level = std::max(1, state.level);
  }
  // Crossroads-owned flags that survive the Capital ↔ Mojave seam.
  auto ensure = [&](const char* flag) {
    for (const auto& f : state.inventory_flags) {
      if (f == flag) {
        return;
      }
    }
    state.inventory_flags.push_back(flag);
  };
  switch (state.leg) {
    case TransitLeg::PlaneWest:
      ensure("plane_ticket");
      state.caps += 50;
      state.last_note = "Boarded plane west from Capital airfield";
      break;
    case TransitLeg::MojaveArrive:
      ensure("arrived_mojave");
      state.level += 1;
      state.last_note = "Touched down in the Mojave";
      break;
    case TransitLeg::BoatEast:
      ensure("ferry_pass");
      state.caps += 25;
      state.last_note = "Cast off on the Colorado ferry east";
      break;
    case TransitLeg::CapitalReturn:
      ensure("returned_capital");
      state.level += 1;
      state.last_note = "Disembarked back in the Capital Wasteland";
      break;
    case TransitLeg::LoopComplete:
      ensure("loop_complete");
      state.last_note = "Crossroads travel loop complete — save journey header";
      break;
    case TransitLeg::CapitalStart:
      state.last_note = "New loop: Capital foothold";
      break;
  }
  save_journey(state);
  log::info("Journey advanced to " + transit_leg_name(state.leg) + " — " + state.last_note);
  return state;
}

LegPlaybook playbook_for(const JourneyState& state) {
  LegPlaybook p;
  switch (state.leg) {
    case TransitLeg::CapitalStart:
      p.title = "Leg 1 — Capital foothold";
      p.summary =
          "Use Pip-Boy quest Crossroads: Capital Foothold — Doc Mitchell gate or map "
          "fast-travel to Crossroads Capital (lands CR_CapitalArrive).";
      p.console_hint = "Pip-Boy → Quests → Crossroads: Capital Foothold\n"
                       "(optional) coc CR_CapitalArrive";
      p.next_action = "Explore, then Advance journey → Plane west";
      break;
    case TransitLeg::PlaneWest:
      p.title = "Leg 2 — Plane west";
      p.summary = "Enter the Crossroads plane cabin transit interior.";
      p.console_hint = "coc CR_PlaneCabin";
      p.next_action = "Ride the sequence, then Advance journey → Mojave";
      break;
    case TransitLeg::MojaveArrive:
      p.title = "Leg 3 — Mojave arrival";
      p.summary = "Exit into the Mojave foothold (vanilla FNV cell preferred).";
      p.console_hint = state.mojave_cell_edid.empty() ? "coc Goodsprings"
                                                      : ("coc " + state.mojave_cell_edid);
      if (!state.mojave_world_edid.empty()) {
        p.console_hint += "\ncow " + state.mojave_world_edid + " -18 16";
      }
      p.next_action = "Roam the foothold, then Advance journey → Boat east";
      break;
    case TransitLeg::BoatEast:
      p.title = "Leg 4 — Boat east";
      p.summary = "Board the Crossroads river ferry transit interior.";
      p.console_hint = "coc CR_BoatCabin";
      p.next_action = "Complete the crossing, then Advance journey → Capital return";
      break;
    case TransitLeg::CapitalReturn:
      p.title = "Leg 5 — Capital return";
      p.summary =
          "Return via the Goodsprings Crossroads Capital map marker / exterior gate, or the "
          "Doc Mitchell house gate.";
      p.console_hint = "World map → Crossroads Capital\n"
                       "or Doc Mitchell door → CR_CapitalArrive\n"
                       "(optional) coc CR_CapitalArrive";
      p.next_action = "Confirm return, then Advance journey → mark loop complete";
      break;
    case TransitLeg::LoopComplete:
      p.title = "Loop complete";
      p.summary = "Journey header saved under %AppData%\\FalloutCrossroads\\journey.json.";
      p.console_hint = "(optional) coc CR_PlaneCabin  — restart via Advance";
      p.next_action = "Advance journey to start another loop, or Quit";
      break;
  }
  return p;
}

}  // namespace crossroads
