# Fallout: Crossroads

Unofficial Windows fan project: one continuous Fallout journey from the
**Capital Wasteland** to the **Mojave** and back (plane west, boat east).

**Phase 2 (current):** Capital pack + **Crossroads Transit** (plane/boat cabins)
+ Mojave foothold resolve + `journey.json` travel loop. Launch deploys plugins;
**Advance** steps Capital → Plane → Mojave → Boat → Capital return.

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
3. **Launch** — deploys `CrossroadsFoothold.esp` + `CrossroadsTransit.esp`, wires BSAs, starts host.
4. Follow console hints; use **Advance** to step the travel loop.

Details: [`docs/phase-2-travel.md`](docs/phase-2-travel.md).

## Roadmap

| Phase | Goal |
|---|---|
| 0 | Boot UI + BYO install gate |
| 1 | Capital foothold (interior + exterior slice) |
| 2 (now) | Plane/boat travel loop Capital ↔ Mojave |
| 3 | Expand both-maps coverage + spine stubs |
| 4 | Polish, crash hygiene, ownership docs |

## License

Original work is CC0 1.0 (`LICENSE`). That does not include Bethesda games,
assets, or trademarks.
