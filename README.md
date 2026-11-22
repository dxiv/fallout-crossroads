# Fallout: Crossroads

Unofficial Windows fan project: one continuous Fallout journey from the
**Capital Wasteland** to the **Mojave** and back (plane west, boat east).

This repository is **Phase 0** — a CMake + MSVC boot launcher with logging,
settings, install-path picking, and a hard **BYO install gate**. There is no
playable world yet.

## Bring your own games

You must own legal PC copies of:

| Game | Expected build |
|---|---|
| Fallout 3 | **1.7.0.3-class** (GOG 1.7.0.3, or Steam downgraded to 1.7.0.3) |
| Fallout: New Vegas | Steam **1.4.0.525** |

The launcher only stores paths under `%AppData%\FalloutCrossroads\`. It
**never** copies Bethesda executables, BSAs, ESMs, or other game assets into
git or into the project tree. Do not commit anything from a game `Data\`
folder.

## Roadmap

Phase 0 is this boot launcher. Later phases target an FNV-hosted bridge into
the Capital Wasteland, then in-world travel to the Mojave and back (plane west,
boat east). See in-app Legal / Credits for the BYO and trademark notices.

## Build (Windows 10/11 + MSVC)

Requirements:

- Visual Studio 2022 (or Build Tools) with the **Desktop development with C++** workload
- CMake 3.24+

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32
cmake --build build --config Release
```

The Win32 (x86) target matches the historical Fallout PC executables we will
host against in later phases. Output: `build\Release\FalloutCrossroads.exe`.

Run the EXE, point it at both install folders, **Save settings**, then
**Continue** only becomes available when both installs validate.

## Repository visibility

Hosted at [dxiv/fallout-crossroads](https://github.com/dxiv/fallout-crossroads)
(**public**). BYO legal installs only — never commit Bethesda game files.

## Reference decomps

See [`reference/README.md`](reference/README.md) for documented read-only
clones of `momusdigital/f3_decomp` and `momusdigital/fnv_decomp`. They are
not linked into this binary.

## License

Contributors’ original work in this repository is dedicated to the public
domain under **CC0 1.0** (`LICENSE`). That grant does **not** include Fallout
3, Fallout: New Vegas, Bethesda assets, or trademarks.
