# Fallout: Crossroads

Unofficial Windows fan project: one continuous Fallout journey from the
**Capital Wasteland** to the **Mojave** and back (plane west, boat east).

**Current build (v0.5):** Phase 0–4 plus Pip-Boy Capital travel — boot gate,
Capital pack, travel loop, coverage/spine stubs, crash guard + doctor checklist,
and a start-game quest with Doc Mitchell gate + world-map fast-travel marker.

## Bring your own games

See [`docs/ownership.md`](docs/ownership.md). You must own:

| Game | Expected build |
|---|---|
| Fallout 3 | **1.7.0.3-class** |
| Fallout: New Vegas | Steam **1.4.0.525** |

No Bethesda assets in git. Mod policy: [`docs/mod-policy.md`](docs/mod-policy.md).

## Build (Windows 10/11 + MSVC)

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32
cmake --build build --config Release
```

Outputs: `FalloutCrossroads.exe` and `CrossroadsPlugin.dll` (Win32).

## Run

1. Set both install paths → **Launch** (deploys Foothold + Transit + Spine ESPs).
2. In New Vegas: Pip-Boy → **Quests → Crossroads: Capital Foothold**.
3. Use the **door in Doc Mitchell's house**, or world-map fast-travel to
   **Crossroads Capital** (no console key needed).
4. Optional: **Advance** still steps the journey journal (Capital → Plane → Mojave → Boat → Capital).
5. Check `%AppData%\FalloutCrossroads\` for `journey.json`, `doctor.txt`, logs.

## Docs

| Doc | Topic |
|---|---|
| [phase-2-travel.md](docs/phase-2-travel.md) | Travel loop |
| [phase-3-coverage.md](docs/phase-3-coverage.md) | Coverage + spine |
| [phase-4-polish.md](docs/phase-4-polish.md) | Crash/doctor |
| [performance.md](docs/performance.md) | Perf notes |

## Roadmap status

| Phase | Status |
|---|---|
| 0 Boot + BYO gate | done |
| 1 Capital foothold | done |
| 2 Travel loop | done |
| 3 Coverage + spine | done |
| 4 Polish / hygiene | done |

## License

Original work is CC0 1.0 (`LICENSE`). That does not include Bethesda games,
assets, or trademarks.
