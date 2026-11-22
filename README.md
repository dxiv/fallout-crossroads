# Fallout: Crossroads

Unofficial Windows fan project: one continuous Fallout journey from the
**Capital Wasteland** to the **Mojave** and back (plane west, boat east).

**Phase 1 (current):** boot launcher + `CrossroadsPlugin.dll` injected into a
BYO `FalloutNV.exe` host, plus a Fallout 3 ESM worldspace probe. There is not
yet a playable Capital cell.

## Bring your own games

You must own legal PC copies of:

| Game | Expected build |
|---|---|
| Fallout 3 | **1.7.0.3-class** (GOG 1.7.0.3, or Steam downgraded to 1.7.0.3) |
| Fallout: New Vegas | Steam **1.4.0.525** |

The launcher only stores paths under `%AppData%\FalloutCrossroads\`. It
**never** copies Bethesda executables, BSAs, ESMs, or other game assets into
git or into the project tree.

## Build (Windows 10/11 + MSVC)

Requirements:

- Visual Studio 2022 (or Build Tools) with **Desktop development with C++**
- CMake 3.24+

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32
cmake --build build --config Release
```

Outputs (same folder):

- `FalloutCrossroads.exe` — settings UI + install gate + host launcher
- `CrossroadsPlugin.dll` — loaded into `FalloutNV.exe`

Keep the DLL next to the EXE. Win32 (x86) matches the game executables.

## Run

1. Start Steam if your New Vegas install is Steam-owned.
2. Run `FalloutCrossroads.exe`.
3. Point it at both install folders, **Save settings**.
4. **Launch** — validates installs, probes `Fallout3.esm` worldspaces, starts
   `FalloutNV.exe` suspended, injects `CrossroadsPlugin.dll`, then resumes.

Plugin log: `%AppData%\FalloutCrossroads\crossroads.log`

This does **not** bypass DRM or ship game files. If injection fails, check
bitness (must be Win32) and antivirus rules.

## Roadmap

| Phase | Goal |
|---|---|
| 0 | Boot UI + BYO install gate |
| 1 (now) | FNV host + plugin attach + F3 ESM probe |
| 1b | One Capital foothold cell via content bridge |
| 2 | Plane/boat travel loop Capital ↔ Mojave |
| 3+ | Expand toward full both-maps play |

## Reference decomps

See [`reference/README.md`](reference/README.md) for optional read-only clones of
public decompilation trees (reference only; not linked into these binaries).

## License

Original work in this repository is dedicated to the public domain under
**CC0 1.0** (`LICENSE`). That grant does **not** include Fallout 3, Fallout:
New Vegas, Bethesda assets, or trademarks.
