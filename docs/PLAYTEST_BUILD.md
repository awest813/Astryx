# KotOR I Playtest Build

How to build Astryx for a full KotOR I playtest, run it against a retail
install, and report what breaks. For the long-term plan see
[FULL_PARITY_AUDIT_AND_PLAN.md](FULL_PARITY_AUDIT_AND_PLAN.md).

## 1. Build

Linux (system packages, same as CI):

```bash
scripts/build_ci_linux.sh --install-deps --config Release --test
# binary: build-linux-ci/bin/xoreos
```

Linux (vcpkg) and Windows: `scripts/bootstrap_linux.sh` /
`scripts/bootstrap_windows.ps1` (see `README.md`).

Debug builds (the CMake default) keep assertions on, which is useful for
playtesting: they catch table/signature mistakes early.
`python3 scripts/verify_kotor_nwscript_stubs.py` checks the NWScript tables
(names, ID alignment, default-argument types) without building.

## 2. Run

You need a retail KotOR I install (Steam/GOG `swkotor`).

```bash
scripts/run_xoreos_linux.sh --game-path "/path/to/swkotor" --windowed
# or directly:
build-linux-ci/bin/xoreos -p "/path/to/swkotor" --logfile=astryx.log
```

Useful options:

| Option | Purpose |
|--------|---------|
| `--logfile=FILE` | Keep the full log for bug reports |
| `--debug=EScripts:1,ELogic:1` | Log script and game-logic decisions |
| `--skipvideos=true` | Skip movies (faster iteration; not for acceptance runs) |

Saves are written to `<game path>/saves/`. Autosaves go to
`000001 - AUTOSAVE` when scripts request them (and the Gameplay option is on).

## 3. What to test

Work through the checklists in order and tick boxes only with evidence:

1. [MILESTONE_A_SMOKE.md](MILESTONE_A_SMOKE.md) — Endar Spire
2. [KOTOR_PROGRESS_TO_DANTOOINE_SMOKE.md](KOTOR_PROGRESS_TO_DANTOOINE_SMOKE.md) — Taris → Dantooine
3. Then the planet arcs (Tatooine, Kashyyyk, Manaan, Korriban), Leviathan,
   Unknown World, Star Forge.

At every module change and before every boss, also try **save → quit to
menu → load** and check party, inventory, journal, opened containers and
doors.

## 4. Reporting a problem

Classify it with the tags from [SUPPORTED_CONTENT.md](SUPPORTED_CONTENT.md)
(`crash`, `progression blocker`, `logic bug`, …) and include:

- module/area (`LASTMODULE` from the save, or the log's "Entering module")
- what you did just before, and a save from just before if you have one
- the log file (`--logfile`) — look for `WARNING:` lines near the failure

## 5. Known gaps (as of 2026-10-17)

These are known and do not need new reports unless they block progress in a
new way:

- **No live playthrough has been verified yet.** Everything below comes from
  code review and unit tests without game data.
- **Creatures are not persisted per area.** Re-entering a module reloads its
  creatures from the GIT, so killed enemies return and OnSpawn runs again.
  Doors, placeables, container contents and object locals *are* persisted.
- **Benched companions' progression** (companions not in the active party)
  is kept for the session but is not written to save files.
- **The PC cannot drop below 1 HP** (`initAsPC` sets min-one-HP), so the
  death screen never appears for the PC.
- **Script timing approximations:** `ActionDoCommand` runs immediately
  instead of after earlier queued actions; `SignalEvent` runs user-defined
  events synchronously; `CreateObject` only creates creatures.
- **Companions added via `AddToParty`** use slot -1 and are treated like the
  PC when the party is rebuilt after a transition.
- **Level-up:** ability and skill choices are applied before the final step,
  so backing out of a level-up keeps them. The feat/Force-power steps fall
  back to the first eligible choice when the retail panel has no matching
  button.
- **Minigames:** swoop SWMG offsets model only the x axis and bullet hit
  parts aren't tracked; Pazaak uses a simplified turn model;
  `CutsceneAttack` applies its damage when queued, not on impact.
- **Ending:** `StartCreditSequence` is a no-op; `EndGame` returns to the
  main menu.
- Movement is straight-line; a creature that is stuck for 3 s abandons the
  move and carries on with its next action.
