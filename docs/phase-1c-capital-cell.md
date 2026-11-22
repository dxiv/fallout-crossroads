# Phase 1c — Import a real Fallout 3 cell

## Goal

Pull one **real interior CELL** from the player’s BYO `Fallout3.esm` into
`CrossroadsFoothold.esp`, remap form IDs for the FNV host, wire Fallout 3 BSAs
into FNV `Data\` so meshes can resolve, and deploy/enable the plugin.

## What Launch does now

1. Index BYO `Fallout3.esm` (record locations + interior CELL EDIDs)
2. Prefer Capital-flavored interiors (`Vault101a`, `MegatonPlayerHouse`, …)
3. Collect cell child `REFR`/`ACHR`/`ACRE` (capped) and copyable base objects
4. Remap form IDs into `0x01xxxxxx` and write GRUP-wrapped children
5. Junction/hardlink/copy BYO `*.bsa` into FNV `Data\` as `CR_FO3_*`
6. Append those BSA names to `Documents\My Games\FalloutNV\FalloutNV.ini`
7. Deploy ESP + enable in `plugins.txt`
8. Start `FalloutNV.exe` with `CrossroadsPlugin.dll`

In-game: `coc <imported EDID>` (shown in the launcher dialog and log).

## Limits (honest)

- Exterior Capital Wasteland worldspace/land is **not** imported yet
- Ref count is capped for the first spike
- Form-ID remapping inside payloads is opportunistic (binary replace)
- Some base types are skipped; missing bases → empty/placeholder refs
- Mod managers may ignore `plugins.txt` / INI edits — enable manually if needed
- Symlinks may require Developer Mode on Windows; hardlink/copy is the fallback

## Verify

1. Win32 Release build
2. Launch with both installs set (first import can take a bit while indexing)
3. Confirm `Data\CrossroadsFoothold.esp` and `Data\CR_FO3_*.bsa`
4. `coc` to the reported EDID
5. Check `%AppData%\FalloutCrossroads\crossroads.log`
