# Milestone A Smoke Verification

This is a fast verification path for the Endar Spire acceptance slice in `MILESTONE.md`.

## Scripted Smoke Check

From the repository root:

```powershell
powershell -ExecutionPolicy Bypass -File tests/smoke/milestone_a_smoke.ps1 -BuildDir build
```

This runs a compact regression set:
- `test_endar_spire_golden`
- `test_combat`
- `crash_regression`

## Manual Acceptance Walkthrough (Endar Spire)

Use this when validating behavior against live game data:

- [ ] Boot to main menu.
- [ ] Create a character (class, attributes, skills, appearance).
- [ ] Loot the opening footlocker and equip gear from inventory.
- [ ] Walk through Endar Spire without movement softlocks.
- [ ] Complete Trask intro dialogue and verify first sealed door opens.
- [ ] Open the second door independently.
- [ ] Resolve at least one mock combat round (hit/miss + HP change).
- [ ] Reach escape pod and trigger module-exit transition.

## Scope Guard

For this smoke pass, triage only:
- `crash`
- `progression blocker`

for the supported Taris entry path (`tar_m02aa`). Defer logic/visual/parity/QoL issues unless they block progression.
