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
- **Minigames (swoop races, Ebon Hawk turret) are untested in play.** Areas
  with an ARE `MiniGame` struct now load the player/enemy/obstacle models on
  their LYT tracks; W/S/A/D (or arrows) steer, Space/Return/left mouse fire.
  A swoop moves along +Y at the SWMG speed and sideways within the tunnel;
  W runs the player's `OnAccelerate`, S `OnBrake`, fire `OnFire`. The turret
  turns at 1 rad/s inside its tunnel (degrees) and fires its gun banks;
  enemies ride their track's first animation, fire when aiming at the
  player, and `OnTrackLoop` runs each time it loops. Assumptions to check:
  the player `OnHeartbeat` runs **every frame** (as KotOR.js); swoop
  collisions are flat (X/Y only) — obstacles by their model's bounding box,
  enemies/triggers (the likely finish line) by `Sphere_Radius` (1.0 if 0);
  the camera is the `Camera` model's `camerahook`, else a chase camera;
  `Start_Offset`/`Target_Offset`, `UseInertia`, `MovementPerSec`, gun
  spread/inaccuracy, jumping, model animation events (`OnAnimEvent`,
  `SWMG_GetLastEvent`) and the swoop HUD aren't used. The race ends only
  when its scripts leave the module. Pazaak uses a simplified turn model.
- **Galaxy map:** destinations use the retail `planetary.2da` rows; the
  arrival modules (e.g. `tat_m17aa`, `danm13`) and the planet button tags
  are unverified against the retail GUI. Leviathan is script-driven only.
- **Ending:** `StartCreditSequence` is a no-op; `EndGame` returns to the
  main menu.
- **Script approximations:** `CutsceneAttack` applies its damage when
  queued, not on impact; SWMG bullet hit parts aren't tracked; many natives are simplified (see
  [FULL_PARITY_AUDIT_AND_PLAN.md](FULL_PARITY_AUDIT_AND_PLAN.md)).
- **Cutscene camera assumptions to confirm in play:** animated camera
  shots look for `CUT%03dW` (DLG `CameraAnimation` - 1200; possibly off by
  one — if animated shots never play, try - 1199) on the model's `camerahook`; GIT static camera orientations are read as (w, x, y, z);
  DLG `AnimList` numbers from 10000 are treated as looping animations.
  Shots that can't be resolved fall back to speaker/listener framing.
  DLG line fades play as KotOR.js reads them (`FadeType` 1 cut to
  `FadeColor`, 2 cut from it, 3 fade in, 4 fade out, over `FadeLength`
  after `FadeDelay`) — check the 1/2 vs 3/4 meaning in play. Stunt
  (`StuntList`) cutscenes are not played.
- **Level-up and XP** come from the game tables, read the way KotOR.js
  reads them; confirm in play:
  - next-level XP from `exptable.2da`; kill XP from `xptable.2da` (column
    `cr<UTC ChallengeRating, rounded>`, row = PC level — the least certain part; if kills give
    0 or absurd XP, this is why);
  - feats/powers per level from `featgain.2da` / `classpowergain.2da`;
    automatic class feats from `feat.2da` `<featstable>_granted`;
  - selectable feats from `feat.2da` (`_list`, prerequisites, `_recom`
    order) and Force powers from `spells.2da` (class column vs. character
    level, `usertype` 1, prerequisites), shown in the panels' `LB_FEATS` /
    `LB_POWERS` list boxes. Unspent picks go to the recommended choices.
- **Combat:** experience is shared by the PC and every companion met so far. Faction
  hostility comes from `repute.2da` (row 0 is the player; reputations are
  shared by both factions and saved as `repute.fac` in `SAVEGAME.sav`, read
  the way KotOR.js reads them — check against retail saves); timed
  ability/skill modifier effects and some talent/effect constants are
  approximated. NPCs now use the stats and Force powers of their `.utc`
  blueprint, so fights may be harder than in earlier builds.
- **Area encounters** (GIT `Encounter List`, `.ute`) now spawn their
  creatures when the party walks in. Confirm in play: they spawn
  `RecCreatures` creatures (at most `MaxCreatures`) chosen at random — the
  `encdifficulty.2da` / CR level budget is not applied; without spawn points
  creatures appear at the polygon's far corners; spawned creatures keep their
  `.utc` faction; `OnExhausted` runs once all spawned creatures are gone.
- **Movement** follows an A* route over the area walkmesh (xoreos'
  pathfinder), with each corner pushed 0.5 m off the wall; routes are redone
  when the goal moves 1 m, every 3 s, and 0.5 s after getting blocked.
  Without a route (an end off the walkmesh) creatures walk straight. A
  creature still stuck after 3 s abandons the move and carries on with its
  next action. Other creatures and placeables are only avoided by the local
  walkability check, not planned around.
- Area state from before this build: creatures in areas visited with an
  older build respawn once (older saves have no creature state).
