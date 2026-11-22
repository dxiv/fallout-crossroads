# Phase 1 — FNV host + plugin

## What landed

- `CrossroadsPlugin.dll` — logs on attach inside `FalloutNV.exe`
- `host_launch.cpp` — `CreateProcess` (suspended) + `LoadLibraryW` injection
- `esm_probe.cpp` — streams BYO `Data\Fallout3.esm` and lists `WRLD` editor IDs
- Boot UI **Launch** runs probe + host start when both installs validate

## What is not done yet (Phase 1b)

- Remapping F3 forms into the FNV data handler
- Streaming a real Capital cell/navmesh into the host
- Pip-Boy / save header / movement proof beyond “host + plugin alive”

## Local verification

1. Build Win32 Release (see README).
2. Configure both game paths in the launcher.
3. Launch; confirm New Vegas starts and `crossroads.log` shows plugin attach.
4. Confirm the probe lists worldspaces (expect names like `WastelandMaster` among others, depending on the ESM).

## Safety

- BYO installs only; no Bethesda assets in git
- No DRM circumvention helpers
- Plugin and games are 32-bit; a 64-bit DLL will fail to inject
