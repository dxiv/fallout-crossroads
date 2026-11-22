# Fallout: Crossroads

Unofficial Windows fan project: one continuous Fallout journey from the
**Capital Wasteland** to the **Mojave** and back (plane west, boat east).

**Phase 1c (current):** imports a real Fallout 3 **interior CELL** from your BYO
`Fallout3.esm` into `CrossroadsFoothold.esp`, remaps form IDs, wires F3 BSAs into
the FNV `Data\` folder, then launches `FalloutNV.exe` with `CrossroadsPlugin.dll`.

## Bring your own games

You must own legal PC copies of:

| Game | Expected build |
|---|---|
| Fallout 3 | **1.7.0.3-class** (GOG 1.7.0.3, or Steam downgraded to 1.7.0.3) |
| Fallout: New Vegas | Steam **1.4.0.525** |

The launcher only stores paths under `%AppData%\FalloutCrossroads\`. It
**never** copies Bethesda game assets into git. Runtime may junction/hardlink
your own F3 BSAs into FNV `Data\` as `CR_FO3_*` so meshes can resolve.

## Build (Windows 10/11 + MSVC)

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32
cmake --build build --config Release
```

Outputs (same folder): `FalloutCrossroads.exe` and `CrossroadsPlugin.dll` (Win32).

## Run

1. Start Steam if needed for New Vegas.
2. Run `FalloutCrossroads.exe`, set both install paths, **Save settings**.
3. **Launch** — indexes F3, imports a preferred interior (Vault/Megaton/…),
   deploys ESP + BSA links, starts the host with the plugin.
4. Console: `coc <EDID>` using the editor ID shown in the dialog/log.

Details: [`docs/phase-1c-capital-cell.md`](docs/phase-1c-capital-cell.md).

## Roadmap

| Phase | Goal |
|---|---|
| 0 | Boot UI + BYO install gate |
| 1 | FNV host + plugin + F3 ESM probe |
| 1b | Empty foothold ESP deploy |
| 1c (now) | Import real F3 interior + BSA wiring |
| 1d | Exterior Capital worldspace / land |
| 2 | Plane/boat travel loop Capital ↔ Mojave |
| 3+ | Expand toward full both-maps play |

## License

Original work is CC0 1.0 (`LICENSE`). That does not include Bethesda games,
assets, or trademarks.
