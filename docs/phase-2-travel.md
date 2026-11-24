# Phase 2 — Capital ↔ Mojave travel loop

## Goal

A reviewer with both legal installs can complete the **journey**:

Capital foothold → plane west → Mojave foothold → boat east → Capital again → save.

## What ships

| Piece | Role |
|---|---|
| `CrossroadsFoothold.esp` | Capital pack (Phase 1d) |
| `CrossroadsTransit.esp` | `CR_PlaneCabin` + `CR_BoatCabin` interiors + `CR_TransitLeg` / `CR_TransitLoops` GLOBs |
| Mojave foothold resolve | Preferred vanilla FNV cell EDID (Goodsprings / Novac / …) — no Bethesda copy |
| `journey.json` | Crossroads-owned session (leg, level, caps, inventory flags) |
| **Advance** button | Steps the loop; Launch deploys content and starts the host |
| Plugin | Logs current leg + console playbook on attach |

## Reviewer loop

1. Set both install paths → **Launch**
2. Pip-Boy → Quests → **Crossroads: Capital Foothold** (start-game enabled)
3. Enter the **Doc Mitchell house gate**, or world-map fast-travel to **Crossroads Capital**
   (marker near Goodsprings; exterior gate beside it). Lands in `CR_CapitalArrive`.
4. Optional journal: **Advance** → plane / Mojave / boat legs (cabins still console-optional)
5. **Advance** → Loop complete (journey header persisted)

## Limits (honest)

- Transit cabins are Crossroads-authored empty interiors (placeholders until meshes kitbash from BYO)
- Capital Pip-Boy path is door + map marker (no console). Other legs may still use `coc` if desired.
- Inventory/level in `journey.json` is the Crossroads seam stub — not a full FNV save rewrite
- Mod managers may need manual plugin enable for both ESPs
- UK keyboards: Capital travel does not need the console key

## Verify

1. Win32 Release build
2. Launch → confirm both ESPs in `Data\`
3. Step Advance through all five legs
4. Confirm `%AppData%\FalloutCrossroads\journey.json`
