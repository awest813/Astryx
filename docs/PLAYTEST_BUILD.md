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

You need a retail KotOR I install (Steam/GOG `swkotor`): the folder that
contains `swkotor.exe`, `chitin.key`, `data/`, `modules/` and `rims/`.

The launcher window finds Steam and GOG installs, checks them, and shows the
engine's error if it stops:

```bash
python3 scripts/xoreos_launcher_gui.py          # Windows: py -3 scripts\xoreos_launcher_gui.py
python3 scripts/xoreos_launcher_gui.py --list   # what it finds, without a window
python3 scripts/xoreos_launcher_gui.py --check "/path/to/swkotor"
```

From a terminal:

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
| `--errordialog=false` | Don't open a dialog window on fatal errors (for scripted runs) |

If xoreos can't start, it says why in a dialog window and in the terminal,
and exits with code 1. The usual causes:

| Message starts with | Fix |
|---------------------|-----|
| `No game that xoreos supports was found` | Point it at the main install folder, not `data` or `modules` |
| `Failed to find the required game file/folder` | The install is incomplete: verify the files in Steam/GOG |
| `Could not open the game window with OpenGL graphics` | Update the graphics driver; OpenGL 2.1 is needed |
| `There is no target` | Use `-p <game folder>`, or one of the targets the message lists |

The log is `~/.local/share/xoreos/xoreos.log` on Linux and
`%APPDATA%\xoreos\xoreos.log` on Windows; the config file is
`~/.config/xoreos/xoreos.conf` or `%APPDATA%\xoreos\xoreos.conf`.

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
    `ActionCastSpellAtObject`.
  - Casting range: the caster first walks into range, `spells.2da` `range`
    S/M/L through `ranges.2da` `primaryrange` of the `SpellRngShrt/Med/Long`
    rows (by label, else rows 2-4; without the table KotOR.js' 10/15/28 m),
    `W` (thrown) 15 m. Personal and touch powers (as KotOR.js) and scripted
    `bCheat` casts (cutscenes) don't walk. A hostile power in range but out
    of sight walks to 2 m; a target that can't be reached drops the cast.
  - Force power cost: powers whose `goodevil` is the other side of the
    caster's alignment (light from 70, dark up to 30) cost 1.5x; no discount
    for the own side.
  - Linked effects apply and expire together. AC, attack, damage, ability,
    skill, save, Force resistance, deflection, immunity and invisibility
    effects stay for their duration and change the stats. Invisible
    creatures go unseen by enemies without True Seeing (`EffectTrueSeeing`
    / `EffectSeeInvisible`); friends always see them; they can still be
    heard. Attacking doesn't end an invisibility effect. Force shields absorb damage per
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
- **Combat sounds and visual effects** are untested with game data; confirm
  in play:
  - Model animation events (MDL event lists: `snd_footstep`, `Swingshort`,
    `Swinglong`, `Swingtwirl`, `HitParry`, `Clash`/`Contact`, `Hit`) are
    queued by the animation thread and played once a frame by the game
    thread (3D-positioned). Other event names (`blur_start`, `detonate`,
    `snd_hitground`, ...) are only logged (debug level 3); emitters aren't
    detonated.
  - Footsteps: `footstepsounds.2da` row = `appearance.2da` `footsteptype`,
    column by the walkmesh surface underfoot (surfacemat rows as KotOR.js:
    1 dirt, 3 grass, 4 stone, 5 wood, 6 water, 9 carpet, 10 metal, 11/13
    puddles, 14 leaves, anything else dirt), falling back to `rolling`.
    Multi-level walkmeshes take the first walkable face at the X/Y spot.
  - Weapon sounds: `weaponsounds.2da` row = the weapon's `baseitems.2da`
    `weaponmattype` (KotOR.js reads `powereditem`; used only when
    `weaponmattype` is empty — if lightsabers sound like clubs, this is
    why). Hits use `leather`, `metal` against droids and objects. The `Hit`
    sound plays on the swing even when the attack misses (damage is still
    resolved at the end of the round).
  - Barks from the soundset: a battle cry (1 in 3) on entering combat, an
    attack grunt (1 in 3) per round, pain when hurt, critical hit, death.
    A bark doesn't cut off a playing one, except death.
  - `EffectVisualEffect`: `visualeffects.2da` `imp_impact_node` goes on the
    `impact` node, `imp_headcon_node` on `headconjure` (else `headhook`),
    `imp_root_*_node` (by `sizecategory`) at the model's root; `soundimpact`
    plays. Instant ones play their `impact` animation once; duration (`D`)
    ones loop `duration` for as long as the effects they're linked to.
    Beams (`EffectBeam`, type `B`) and the `progfx_*` Force shield shells
    aren't shown. `ApplyEffectAtLocation` with a lone visual plays it at the
    place. Particle emitters in effect models are not rendered by the
    engine, so many effects may show little.
  - Casting a power shows the `spells.2da` conjure visuals (hand/head/ground,
    read as `conjhandvfx` or `conjhandvisual`; numbers are
    `visualeffects.2da` rows) and sounds (`conjsoundvfx`, by gender
    `conjsoundmale/female`), then the cast visuals and `castsound` after
    `conjtime` (at most 1.5 s). No projectiles (blaster bolts, thrown
    grenades) yet.
  - Attack animations: duelling melee fighters play `c<w>a1`-`5` and the
    defender the matching parry `c<w>p<n>`; others `g<w>a1`/`a2`, ranged
    `b<w>a1`. Animations are still chosen at the start of the round, before
    the attack is rolled, so there are no hit (`g<w>d1`) reactions yet.
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
  A detected mine offers Disarm/Recover Mine (Demolitions), and equipped
  items' powers (armbands) are on the self power panel. A locked door or
  container that isn't plot or `NotBlastable` offers Bash (`BTN_TARGET0`).
  Mine items (`baseitems.2da` row 58, as KotOR.js, or a Trap item property)
  are on the mine self panel. Not offered yet: security tunnelers, setting
  a mine on a locked door/container (KotOR.js offers it). Feat IDs are K1's nwscript `FEAT_*`
  values (`feat.2da` rows, e.g. Flurry 11, Power Attack 28); saves from
  builds before this one stored the old invented numbers for the PC's feats.
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
- **Triggers, heartbeats, mines and locks** follow KotOR.js and the d20
  rules; confirm in play:
  - every creature (companions and NPCs too) fires trigger `OnEnter`/`OnExit`
    with itself as `GetEnteringObject()`, also for overlapping triggers;
    creatures the GIT places in a trigger are inside without an `OnEnter`;
    destroyed creatures leave without an `OnExit`, the dead stay inside. Area
    transitions still only move the party when its leader walks in.
    `GetFirstPC()` is the party leader and `GetIsPC()` is true for it (plot
    triggers guarded by them now fire for a companion the player controls).
  - creatures (party included), placeables, doors and triggers get
    `OnHeartbeat` every **6 s** (KotOR.js uses 3 s), spread over the round.
    Companions run their `ScriptHeartbeat` (`k_hen_heartbt01`) instead of a
    direct `k_ai_master` call; a party member without one (the player
    character) runs `k_ai_master` as event 2001. Companions following the
    leader with `ActionForceFollowObject` still join fights.
  - mines (`Type` 2 / `TrapFlag` triggers) go off (`OnTrapTriggered`) for
    creatures their `Faction` is hostile to. Party members within **10 m**
    spot them with Awareness + 10 (a d20 each heartbeat in combat) against
    `TrapDetectDC`; the `traps.2da` model then appears. Disarming is
    Demolitions + 10 (d20 in combat) against `DisarmDC`, recovering
    `DisarmDC` + 5 (gives the `traps.2da` `resref` item); failing by 5 sets
    the mine off. Triggers (and so mines) now keep their state between visits.
  - setting a mine plays `animations.2da` row 52 (KotOR.js' SET_MINE) for
    1.5 s and lays a 1.5 m square trap trigger 1 m ahead of the leader (at
    the leader's feet when that spot isn't walkable), of the leader's
    faction, so enemies set it off. Its `traps.2da` row is the one whose
    `resref` is the mine item (else the Trap property subtype), its script
    `trapscript`; detect/disarm DCs are 10 + Demolitions + `detectdcmod` /
    `disarmdcmod` (column names to check). No Demolitions check to set it.
    The party sees its own mines and can recover them; armed ones are kept
    between visits and in saves.
  - bashing: the leader attacks the object in normal rounds (melee reach
    +1 m, as the object's position is its middle; AC 10); every blow loses
    the blueprint's `Hardness`; `OnMeleeAttacked`, `OnDamaged` and
    `OnDeath` run, then at 0 HP a door is destroyed — open for good, its
    walkmesh and line-of-sight block gone, playing `die` if the model has it
    (else `opening1`) — and a container is unlocked and left open with its
    contents (nothing is destroyed). The broken state is saved. Doors now
    read `HP`/`CurrentHP`, doors and placeables `Plot`, `Hardness` and
    `NotBlastable`; plot objects can't be bashed. `GetLastAttacker` /
    `GetLastDamager` work on bashed objects.
- **Computer terminals** (DLG `ConversationType` 1) run in the `computer`
  GUI over the `comp0` background (as KotOR.js' InGameComputer): the line
  in `LB_MESSAGE`, the replies in `LB_REPLIES`, no letterbox and no
  cinematic camera. `LBL_COMP_SPIKES_VAL` / `LBL_REP_UNITS_VAL` count the
  party's `K_COMPUTER_SPIKE` / `K_REPAIR_PART` items (tags to check),
  `LBL_COMP_SKILL_VAL` / `LBL_REP_SKILL_VAL` show the leader's Computer Use
  / Repair. Camera-view nodes (the `computercamera` GUI) aren't shown.
- **Stealth:** `TB_STEALTH` toggles the leader's stealth mode (no Stealth
  Field Generator needed, no slower movement, no stealth XP). Enemies see a
  sneaking creature only if their Awareness + 10 beats its Stealth + 10 +1
  per 4 m (no rolls); attacking or a hostile power ends stealth. NPCs lose
  a target that turns invisible or stealthy; the enemy-sighted auto-pause
  ignores what the leader can't see.
  - picking locks takes 1.5 s, Security + 20 out of combat (a d20 in combat)
    against the lock DC; failure runs `OnFailToOpen`, success `OnUnlock`.
    `ActionRandomWalk` strolls 2-8 m around its start, pausing 1-4 s.
- Area state from before this build: creatures in areas visited with an
  older build respawn once (older saves have no creature state).
