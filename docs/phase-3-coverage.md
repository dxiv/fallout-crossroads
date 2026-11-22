# Phase 3 — Both-maps coverage + spine stubs

## Goal

Expand toward **both wastelands explorable** (still not every vanilla cell) and
land a **Crossroads spine** stub set so movement / combat readiness / travel
beats have concrete EDIDs.

## What ships

| Piece | Role |
|---|---|
| Expanded Capital import | Up to ~5×5 exterior pocket, higher ref budget |
| Coverage policy | Only one wasteland is **hot** per travel leg (Capital / Mojave / transit) |
| Mojave `cow` hint | Host-native exterior grid near Goodsprings |
| `CrossroadsSpine.esp` | `CR_Spine*` NOTE stubs + `CR_SpineStage` / `CR_SpineCombatReady` GLOBs |

## Priorities (this phase)

1. Movement between Capital ↔ Mojave via the Phase 2 loop (kept)
2. Larger Capital exterior slice + Mojave exterior console targets
3. Spine stubs (airfield → plane → touchdown → ferry → return)
4. Combat/loot/merchant parity remains host-native where content exists

## Limits (honest)

- Not a full Capital or Mojave map dump
- Hot/cold policy is Crossroads session guidance + logging; engine unload hooks are still partial
- Spine NOTEs are stubs, not full dialogue trees
- Vanilla main quests are not fully enabled

## Verify

1. Win32 Release build
2. Launch → three ESPs in `Data\`
3. Capital `cow` + Mojave `cow` hints present in log
4. Advance loop still completes
