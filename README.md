# Fallout: Crossroads

Unofficial Windows fan project: one continuous Fallout journey from the
**Capital Wasteland** to the **Mojave** and back (plane west, boat east).

**Phase 3 (current):** expanded Capital exterior coverage, hot-wasteland
streaming policy, Mojave exterior `cow` hints, and `CrossroadsSpine.esp`
quest stubs — on top of the Phase 2 travel loop.

## Bring your own games

You must own legal PC copies of:

| Game | Expected build |
|---|---|
| Fallout 3 | **1.7.0.3-class** (GOG 1.7.0.3, or Steam downgraded to 1.7.0.3) |
| Fallout: New Vegas | Steam **1.4.0.525** |

The launcher only stores paths under `%AppData%\FalloutCrossroads\`. It
**never** copies Bethesda game assets into git.

## Build (Windows 10/11 + MSVC)

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32
cmake --build build --config Release
```

Outputs: `FalloutCrossroads.exe` and `CrossroadsPlugin.dll` (Win32).

## Run

1. Set both install paths → **Launch** (deploys Foothold + Transit + Spine ESPs).
2. **Advance** through Capital → Plane → Mojave → Boat → Capital.
3. Use Capital / Mojave `coc`/`cow` hints from the dialog and log.

Details: [`docs/phase-3-coverage.md`](docs/phase-3-coverage.md).

## Roadmap

| Phase | Goal |
|---|---|
| 0–1 | Boot + Capital foothold |
| 2 | Travel loop |
| 3 (now) | Coverage + spine stubs |
| 4 | Polish, crash hygiene, ownership docs |

## License

Original work is CC0 1.0 (`LICENSE`). That does not include Bethesda games,
assets, or trademarks.
