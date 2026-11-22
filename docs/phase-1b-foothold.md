# Phase 1b — Foothold cell bridge

## Goal

Stand in a Crossroads-controlled cell inside the FNV host, with BYO Fallout 3
data used as a probe/hint toward later Capital import.

## What this phase ships

1. **ESP builder** (`esp_writer.cpp`) writes `CrossroadsFoothold.esp`:
   - Master: `FalloutNV.esm`
   - One empty **interior** `CELL` with editor ID `CrossroadsFoothold`
2. **Deploy** (`bridge_foothold.cpp`) on Launch:
   - Stages under `%AppData%\FalloutCrossroads\bridge\`
   - Copies into `{FalloutNV}\Data\CrossroadsFoothold.esp`
   - Appends the plugin to `plugins.txt` when found (`%LocalAppData%\FalloutNV\` and/or `Data\`)
3. **F3 hint** — still probes BYO `Fallout3.esm` worldspaces and records a Capital-ish name when present
4. **Host** — injects `CrossroadsPlugin.dll` as in Phase 1

In-game check: open the console and run `coc CrossroadsFoothold`.

## Honesty / limits

This foothold CELL is **Crossroads-authored**, not a byte-import of a Capital
Wasteland exterior. Importing a real F3 cell (refs, pathing, land, BSA paths)
is the next bridge milestone and is much larger.

If you use a mod manager (MO2, Vortex), enable `CrossroadsFoothold.esp` there
instead of relying on `plugins.txt` edits.

## Verify

1. Win32 Release build
2. Launch from `FalloutCrossroads.exe` with both installs set
3. Confirm `Data\CrossroadsFoothold.esp` exists
4. `coc CrossroadsFoothold` in the FNV console
5. Read `%AppData%\FalloutCrossroads\crossroads.log`
