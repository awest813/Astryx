# KotOR Progress-to-Dantooine Smoke

This checklist tracks fast regression coverage for the current supported progression path.

## Scripted Smoke

From repo root:

```powershell
powershell -ExecutionPolicy Bypass -File tests/smoke/kotor_progression_smoke.ps1 -BuildDir build
```

Current scripted slice:
- `test_endar_spire_golden`
- `test_taris_progression`
- `test_alignment_xp`
- `crash_regression`

## Manual Progression Path (Main Menu to Dantooine)

Use this walkthrough when validating with live game content:

- [ ] Main menu loads and starts a new game.
- [ ] Endar Spire is completable through escape pod transition.
- [ ] `tar_m02aa` loads without script halts and Carth party-join path executes.
- [ ] Party operations (add/remove/count/index/solo mode) remain stable after transitions.
- [ ] Global string and boolean/number script gates survive module changes.
- [ ] Taris progression path reaches departure sequence without crash/progression blocker.
- [ ] Galaxy map flow allows travel selection toward Dantooine path (GUI or scripted fallback selection).
- [ ] Dantooine arrival transition completes without crash or hard script stop.

## Triage Policy

For this slice, treat only these categories as immediate blockers:
- `crash`
- `progression blocker`

Defer other classes until the path above is stable end-to-end.
