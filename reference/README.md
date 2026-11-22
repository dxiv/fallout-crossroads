# Reference decompilations (read-only)

Fallout: Crossroads treats the public Gamebryo Fallout decompilations as a
**behavioral encyclopedia**, not as something to link into the Phase 0 boot
app or to ship as game content.

## Repositories

| Game | Upstream (read-only) |
|---|---|
| Fallout 3 | https://github.com/momusdigital/f3_decomp |
| Fallout: New Vegas | https://github.com/momusdigital/fnv_decomp |

Do **not** push to those remotes from this project. Do **not** vendor Bethesda
binaries or `Data/` trees here.

## Documented clones (preferred)

Keeping this repo lean: clone references beside the tree when you need them.

```bat
cd reference
git clone --depth 1 https://github.com/momusdigital/f3_decomp.git
git clone --depth 1 https://github.com/momusdigital/fnv_decomp.git
```

The local folders `reference/f3_decomp/` and `reference/fnv_decomp/` are
gitignored so accidental `git add` cannot swell the Crossroads history.

Pinned SHAs observed while planning Phase 0 (2026-11-22):

- `f3_decomp` @ `f5bac1b54fc8fde16294c1d6779f27407d0a117b`
- `fnv_decomp` @ `2b07cb3ec4b7aa11be96c7567914e11731aebeaf`

## Why not git submodules?

Full decomp trees are large and change often. Documented shallow clones avoid
submodule pain for contributors who only need the boot launcher. If the team
later wants pinned submodules for CI oracle tests, add them deliberately.
