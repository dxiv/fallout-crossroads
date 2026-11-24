# Mod policy (conservative)

Crossroads is verified against **vanilla** BYO Fallout 3 + New Vegas installs.

- Treat third-party mods (TTW, gameplay overhauls, ENBs, script extenders beyond
  what Crossroads injects) as **untested**.
- When reporting bugs, disable Mod Organizer / Vortex overlays first.
- Do not load other plugins that remap the same form-id bands Crossroads uses
  (`0x01xxxxxx` foothold/transit/spine records).
- Never pack or share Bethesda archives with Crossroads builds.
- If a prior Launch left `CR_FO3_*.bsa` in FNV `Data\` / `FalloutNV.ini` and
  saves crash on load, run Launch again (empty-foothold path clears them) or
  remove those `CR_FO3_*` archive entries manually.
