# Performance notes

- **One hot wasteland:** coverage policy keeps Capital or Mojave (or transit)
  hot per journey leg. Avoid buffering both huge exteriors at once.
- **LOD / meshes:** only from BYO packages via `CR_FO3_*` links — no extra LOD
  packs are shipped.
- **Import caps:** Capital exterior pocket is budgeted (radius/refs). Raising
  budgets increases index/import time and memory.
- **Crash log:** `%AppData%\FalloutCrossroads\crash.log` on unhandled faults.
