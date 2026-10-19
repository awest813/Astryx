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

## 5. Known gaps (as of 2026-10-19)

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
- **Script approximations:** `CutsceneAttack` damage lands when the
  attacker reaches its target and starts the swing, not at the animation's
  hit frame; SWMG bullet hit parts aren't tracked; many natives are simplified (see
  [FULL_PARITY_AUDIT_AND_PLAN.md](FULL_PARITY_AUDIT_AND_PLAN.md)).
- **Cutscene assumptions to confirm in play:** animated camera shots and
  cutscene participant animations (DLG numbers from 1200) are named
  `CUT%03dW` of the number - 1199 (as KotOR.js names them), falling back to
  - 1200; cameras use the model's `camerahook`. GIT static camera
  orientations are read as (w, x, y, z); DLG `AnimList` numbers from 10000
  are looping animations. Shots that can't be resolved fall back to
  speaker/listener framing. DLG line fades play as KotOR.js reads them
  (`FadeType` 1 cut to `FadeColor`, 2 cut from it, 3 fade in, 4 fade out,
  over `FadeLength` after `FadeDelay`). Stunt cutscenes: each `StuntList`
  participant plays animations from its `StuntModel` (looked up before its
  own) for the conversation; actors are not repositioned (KotOR.js faces
  them to 0 for `AnimatedCut` dialogs — check placement in the Endar Spire
  and Leviathan cutscenes).
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
  the way KotOR.js reads them — check against retail saves). NPCs now use
  the stats and Force powers of their `.utc` blueprint, so fights may be
  harder than in earlier builds.
- **Force powers, talents, effects and items** follow the rules but are
  untested with game data; confirm in play:
  - AI talents (`GetCreatureTalentBest/Random`) pick from known powers the
    creature has the Force points for, equipped items' powers, consumable
    items' spells (medpacs, grenades), feats and trained skills, matching the
    `category` column of `spells/feat/skills.2da` as a bitmask (as KotOR.js)
    and taking the highest `maxcr` not above nCRMax (KotOR.js takes the
    lowest). If Dark Jedi or companions never use powers or items, check the
    categories the AI scripts pass. Spell talents are cast like
    `ActionCastSpellAtObject`; there's no casting range check.
  - Force power cost: powers whose `goodevil` is the other side of the
    caster's alignment (light from 70, dark up to 30) cost 1.5x; no discount
    for the own side.
  - Linked effects apply and expire together. AC, attack, damage, ability,
    skill, save, Force resistance, deflection, immunity and invisibility
    effects stay for their duration and change the stats; invisibility only
    sets a flag (perception ignores it). Force shields absorb damage per
    `forceshields.2da` (`damageflags`, `resistance` per hit, `amount` in
    total); without it a shield soaks 10 damage per row. Force push throws
    the target up to 5 m away from the caster (walkmesh permitting), plays
    `animations.2da` row 84 and knocks it down for 3 s.
  - Using an item with a CastSpell property runs its `spells.2da` impact
    script (no Force point cost; `GetSpellCastItem` is the item); grenades
    are cast at the target location. Without the script the old built-in
    heal/stim/explosion applies. Equipped items' charges aren't counted.
  - Item Ability Bonus properties raise the ability scores; item bonus
    amounts are read from the property's `CostValue`.
  - Combat feats use the feat.2da rows; Power Attack/Power Blast give -3
    attack and +5/+8/+10 damage, Flurry/Rapid Shot -4/-2/-1 attack and
    Defense for an extra attack, Critical Strike -5 Defense, Critical
    Strike/Sniper Shot a +1/+2/+3 critical threat range (Sniper Shot's
    effect is assumed to mirror Critical Strike's).
- **Action menus** (built as KotOR.js' `ActionMenuManager`): above a
  hostile target, `BTN_TARGET0` offers attack and the leader's melee or
  ranged combat feats (`feat.2da` `category` 0x1104 / 0x1111, only the last
  known feat of a `successor` chain), `BTN_TARGET1` hostile Force powers
  (`spells.2da` `forcehostile`) or Security on a locked door/placeable, and
  `BTN_TARGET2` grenades. The self buttons `BTN_ACTION0`-`3` offer friendly
  powers (`forcefriendly`), medical items, other usable items, and (empty)
  mines — the order follows KotOR.js' tooltips; KotOR.js itself fills
  friendly powers into the second panel, so check the order against retail.
  Icons go on the target buttons and fill `LBL_ACTION0`-`3`; the up/down
  arrows cycle options and `LBL_ACTIONDESC` names the chosen one for 2 s.
  Not offered yet: bashing locked objects, security tunnelers, setting,
  disarming or recovering mines.
- **Pause:** Space (or `TB_PAUSE`) pauses; `PauseGame` pauses/unpauses.
  While paused, creature actions, combat rounds, heartbeats, effects,
  `DelayCommand` timers and model animation time stop (timers resume where
  they were); the camera turns and orders are queued. Item uses from the
  action menus wait for the unpause. The reason shows in `LBL_CMBTMODEMSG`
  (`dialog.tlk` 1508/42432/42396/42397/42481/42482 as KotOR.js uses them —
  check the texts). Auto-pause follows the options: enemy sighted (a hostile
  within 16 m and in line of sight of the leader while out of combat),
  party member down, end of combat round, new target after the old one
  died, action menu used; "mine sighted" isn't detected yet. Never during
  conversations, cutscenes, movies or minigames. Opening a menu (Esc,
  inventory) still lets script timers run on, as before.
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
