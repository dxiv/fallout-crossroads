# Phase 1d — Capital exterior slice (Phase 1 complete)

## Goal

Finish Phase 1 by importing a **Capital Wasteland exterior pocket** alongside the
interior foothold: one `WRLD` plus a small grid of exterior `CELL`s (with `LAND`
and capped `REFR`s) from BYO `Fallout3.esm` into `CrossroadsFoothold.esp`.

## What Launch does now

1. Index BYO `Fallout3.esm`
2. Prefer a Capital-flavored interior (`Vault101a`, Megaton house, …)
3. Prefer `Wasteland` / Capital-named `WRLD`, pick densest exterior cell, take a
   ~3×3 pocket around it (capped)
4. Remap form IDs into `0x01xxxxxx`, write GRUP trees (world + cell children)
5. Junction/hardlink/copy BYO `*.bsa` into FNV `Data\` as `CR_FO3_*`
6. Append those BSA names to `FalloutNV.ini` archives
7. Deploy ESP + enable in `plugins.txt`
8. Start `FalloutNV.exe` with `CrossroadsPlugin.dll`

In-game (hints shown in the launcher dialog / log):

- Interior: `coc <EDID>`
- Exterior: `cow <WRLD> <x> <y>`

Fallback order if the pack fails: Phase 1c interior-only import → empty foothold CELL.

## Limits (honest)

- Exterior coverage is a **slice**, not the full Capital map
- Ref / cell counts are capped for the first playable spike
- Form-ID remapping inside payloads is opportunistic (binary replace)
- Some base types are skipped; missing bases → empty/placeholder refs
- Mod managers may ignore `plugins.txt` / INI edits — enable manually if needed
- Symlinks may require Developer Mode on Windows; hardlink/copy is the fallback

## Verify

1. Win32 Release build
2. Launch with both installs set (indexing takes a bit)
3. Confirm `Data\CrossroadsFoothold.esp` and `Data\CR_FO3_*.bsa`
4. `coc` / `cow` using the reported hints
5. Check `%AppData%\FalloutCrossroads\crossroads.log`
