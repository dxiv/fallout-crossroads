# Phase 4 — Polish & release hygiene

## Ships

- Process crash guard → `crash.log`
- **Doctor** checklist → `doctor.txt` (ownership, plugins, mod/perf notes)
- Docs: ownership, mod policy, performance
- Credits/legal refreshed for the full loop build

## Verify

1. Win32 Release build
2. Launch once → confirm doctor.txt / crash guard log line
3. Intentionally leave a bad path → doctor reports MISSING
4. Complete Advance loop still works
