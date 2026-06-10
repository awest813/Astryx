# Merged Pull Requests — KotOR Endar Spire Milestone

**Date:** 2026-06-10  
**Base:** `master` @ `c286e3668`  
**Merged tip:** `9b8d00932` (`cursor/endar-spire-perfect-7ac0`)

This document records the consolidation of four related draft PRs into `master`. All branches were linear ancestors of `cursor/endar-spire-perfect-7ac0`, so a single fast-forward merge brought in the full stack (18 commits, +5,549 / −2,517 lines across 78 files).

---

## Pull Requests Merged

| PR | Branch | Title | Commits |
|----|--------|-------|---------|
| [#35](https://github.com/awest813/Astryx/pull/35) | `cursor/nwscript-audit-functions-7ac0` | NWScript parity — implementations, stubs, audit polish | 6 |
| [#36](https://github.com/awest813/Astryx/pull/36) | `cursor/nwscript-high-impact-7ac0` | High-impact NWScript — doors, items, effects, Jedi | 1 |
| [#37](https://github.com/awest813/Astryx/pull/37) | `cursor/cutscene-cinematic-7ac0` | Cutscene camera, choreography, and cinematic natives | 2 |
| [#38](https://github.com/awest813/Astryx/pull/38) | `cursor/endar-spire-perfect-7ac0` | Endar Spire milestone — gameplay systems polish | 9 |

---

## Commit History (oldest → newest)

1. `14db885c3` — feat(nwscript): wire 28 high-priority KotOR I NWScript functions
2. `ca4521c87` — feat(nwscript): register remaining 284 KotOR I functions as safe stubs
3. `e34440fcd` — feat(nwscript): register all 600 KotOR II functions as safe stubs
4. `700db5ddd` — feat(nwscript): harden KotOR stubs with SWMG category and verification
5. `6bf088da1` — feat(kotor): implement tier-1 NWScript functions and minimal save writing
6. `5b68838cf` — fix(kotor): audit polish for NWScript stubs and save helpers
7. `9021da2da` — feat(kotor): implement high-impact NWScript door, item, and effect APIs
8. `485b9a745` — feat(kotor): improve cutscene camera, choreography, and cinematic natives
9. `215f16dcb` — feat(kotor): wire movie playback, camera restore blend, and dialog locks
10. `036ebe37d` — feat(kotor): polish Endar Spire tutorial flow and NWScript gaps
11. `94403ba6f` — fix(kotor): repair corrupted getNearestCreature after getNearestObject edit
12. `4b04638f9` — feat(kotor): complete Endar Spire start-to-finish progression fixes
13. `5b85eb5c3` — chore(kotor): audit polish for Endar Spire and NWScript spatial queries
14. `04af9cf7f` — feat(kotor): full SAVEGAME.sav serialization and trap/item-property combat
15. `6c25052d7` — feat(kotor): speech bubbles, animation 2da, explore map, load game
16. `45f759152` — feat(kotor): pause menu, cutscene combat polish, combat NWScript
17. `b9a956464` — feat(kotor): fix Endar Spire opening cutscene timing
18. `9b8d00932` — feat(kotor): defer HUD during Endar Spire opening beat

---

## Feature Summary by Area

### NWScript Engine (#35, #36)

- Registered all remaining KotOR I (284) and KotOR II (600) function slots as safe stubs
- Added `scripts/verify_kotor_nwscript_stubs.py` CI guard (null=0 for both games)
- Implemented tier-1 functions: spatial queries, reputation, journal, party, module transitions
- High-impact APIs: doors (`ActionOpenDoor`), item properties, situated objects, encounter hooks
- KotOR I: 174 generic stubs + 82 SWMG stubs remain; KotOR II: 446 generic + 103 SWMG

### Cinematic & Camera (#37)

- `CutsceneAttack`, `CutsceneMove`, camera restore blend, `SetCutsceneMode`
- Movie queue playback (`PlayMovie`, `PlayMovieQueue`)
- Dialog camera locks and cinematic focus during conversations
- `runCinematicBeat()` — fixed-duration render loop for shake/animation without player input

### Endar Spire Gameplay (#38)

**Opening cutscene (two-phase flow)**

1. Main menu → `signalEncounter("end_opening")` → `swlogo` + `openingcrawl` → load `end_m01aa`
2. `Module::enter()` → defer HUD → `performEndarSpireOpeningBeat()` → shake, `mus_bat_ship`, journal entry, restore area music

**Encounter hooks (`SignalEncounter`)**

| ID | Handler |
|----|---------|
| `end_opening` | Logo/crawl + module load; attack beat on enter |
| `end_trask` | Trask Ulgo reveal cinematic |
| `end_sith_board` | Sith boarding in Sector 2 |

**Other systems**

- Floating text HUD (`ActionSpeakString`, `FloatingTextStringOnCreature`)
- `animations.2da` lookup for `ActionPlayAnimation`
- `ExploreAreaForPlayer` + map explored tile persistence (17×9 grid)
- Full `SAVEGAME.sav` serialization; main-menu and in-game load wired
- ESC / pause menu, PC death → options flow
- `SetPlayerRestrictMode`, `GetGoingToBeAttackedBy`, `GetLastAttackMode`
- Trap and item-property combat integration

---

## New & Modified Key Files

| Path | Purpose |
|------|---------|
| `src/engines/kotor/encounters_end.{h,cpp}` | Endar Spire opening, Trask, Sith boarding cinematics |
| `src/engines/kotorbase/gui/floatingtext.{h,cpp}` | Speech-bubble HUD overlay |
| `src/engines/kotorbase/animationnames.{h,cpp}` | `animations.2da` ID → name lookup |
| `src/engines/kotorbase/script/functions_stub.cpp` | Safe no-op stubs for unimplemented natives |
| `src/engines/kotorbase/script/functions_encounter.cpp` | Encounter / signal hooks |
| `src/engines/kotorbase/script/functions_item.cpp` | Item property natives |
| `scripts/verify_kotor_nwscript_stubs.py` | Stub table verification script |
| `tests/engines/kotorbase/endar_spire_golden.cpp` | Golden-path regression tests |
| `tests/engines/kotorbase/save_serialization.cpp` | Save/load round-trip tests |
| `tests/engines/kotorbase/dantooine_cinematic.cpp` | Cinematic helper tests |

---

## Verification

```bash
python3 scripts/verify_kotor_nwscript_stubs.py
# kotor: null=0 generic_stub=174 swmg_stub=82
# kotor2: null=0 generic_stub=446 swmg_stub=103
```

Manual smoke (requires KotOR game data):

1. New Game → character creation → logo/crawl → Endar Spire attack shake
2. Trask dialogue and footlocker loot
3. Combat round vs Sith trooper
4. Save / load from main menu and in-game
5. Escape pod → module transition to Taris

---

## Known Remaining Gaps

- Minimap fog-of-war visual (exploration data persists; reveal not rendered)
- ~174 generic NWScript stubs in KotOR I still need real implementations for Taris+
- Dedicated death screen GUI (currently routes to options menu)
- `Module::showGUIPanel()` still empty
- Bink/movie fidelity depends on asset availability

See [MILESTONE.md](../MILESTONE.md) and [endar_spire_functions.md](endar_spire_functions.md) for ongoing tracking.
