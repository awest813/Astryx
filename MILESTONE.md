# Starting Milestone

This document defines the project's first formal milestone: a concrete, achievable
set of goals that marks meaningful progress toward the full parity targets described
in [ROADMAP.md](ROADMAP.md).

**Goal:** Make the _Endar Spire_ tutorial in **Knights of the Old Republic** fully
playable from the opening cutscene through to the escape pod — the first time a
player can complete a contiguous segment of a targeted BioWare game inside xoreos.

---

## Scope

Only KotOR I's Endar Spire tutorial is in scope for this milestone.  All other
games and areas remain on the long-term roadmap.

---

## Acceptance Criteria

A player must be able to, without using the debug console or any manual module
overrides:

1. **Boot the game** to the main menu (already working).
2. **Create a character** — choose class (Soldier / Scout / Scoundrel), assign
   the opening attribute and skill points, and select an appearance.
3. **Receive starting equipment** from the footlocker at the start of the module
   and equip it from the inventory screen.
4. **Walk through the _Endar Spire_** using the existing movement and walkmesh
   systems.
5. **Interact with Trask Ulgo** — complete his introductory dialogue, have him
   open the first sealed door.
6. **Open the second door** independently (already partially working).
7. **Complete the mock combat encounter** — resolve at least one round of
   turn-based combat, applying attack rolls and hit/miss results.
8. **Reach the escape pod** — trigger the module-exit script and return to
   the main menu.

---

## Required Work

Items below are drawn from [ROADMAP.md](ROADMAP.md) and scoped to the minimum
needed for the acceptance criteria above.

### Character Creation

- [x] Complete character creation UI — class selection screen, attribute point
      allocation, skill point allocation, and appearance selection.
      *(Class selection, portrait, name, and Quick-char path were already done;
      `CharacterGenerationAbilitiesMenu` and `CharacterGenerationSkillsMenu` added
      for the Custom-char path.)*
- [x] Wire character creation output to a correctly-initialised `Creature` object
      that is used as the Player Character for the rest of the session.
      *(`initAsPC()` wired; starting HP now computed from class hit-die + Con
      modifier; skills forwarded from `CharacterGenerationInfo`.)*

### Inventory & Equipment

- [x] Implement item pickup from containers (footlockers) — `GetItemInSlot`,
      `CopyItem`, `AddItemToObject` NWScript functions, and the underlying
      container looting path.
      *(`CreateItemOnObject` / `GetItemInSlot` implemented; `openContainer()` in
      `Module` triggers the container GUI which transfers items to PC inventory.)*
- [x] Implement the equip-item flow from the inventory screen — map equipped
      slots to visible model attachments and stat adjustments on the `Creature`.
      *(`MenuEquipment` calls `Creature::equipItem()` which reloads the model.)*

### Dialogue & Script Gates

- [x] Implement attribute and skill checks inside dialogue nodes so that
      class-gated lines resolve correctly.
      *(`DLGFile` already runs the `Active` / `Active2` conditional scripts;
      `GetAbilityScore`, `GetSkillRank`, and `GetHasSkill` are all implemented.)*
- [x] Implement the NWScript functions needed by the Endar Spire module scripts;
      see the tracked list in [doc/endar_spire_functions.md](doc/endar_spire_functions.md).
      *(All functions in the tracker are now ✅ or ⚠️; `CancelCombat` (ID 54) and
      `GetLastAttacker` (ID 36) wired in previous session; `ActionMoveToLocation`
      (ID 21), `GetFirstObjectInArea` (ID 93), `GetNextObjectInArea` (ID 94),
      `GetIsEnemy` (ID 235), `GetIsFriend` (ID 236), `GetIsNeutral` (ID 237),
      `GetName` (ID 253), `SetIsDestroyable` (ID 323), and `GetIsInConversation`
      (ID 445) wired in this session; both kotor and kotor2 function tables updated.)*

### Combat

- [x] Implement a single round of turn-based combat: select target, roll
      `d20` + attack bonus vs. target AC, apply weapon damage on hit.
      *(`RoundController` ticks rounds; `executeAttack()` rolls d20 + Str/Dex
      modifier vs. target AC, applies weapon damage.)*
- [x] Play the correct hit / miss animation on attacker and target.
      *(`notifyCombatRoundBegan()` calls `playAttackAnimation()` /
      `playDodgeAnimation()` on attacker and target respectively.)*
- [x] Reduce target HP; detect and handle the downed state (enemy removed from
      active combat).
      *(`executeAttack()` reduces HP; `handleCreaturesDeath()` called each frame
      plays "die" animation and fires event 1007; dead creatures are skipped in
      subsequent combat rounds.)*

### Save / Load (in-session)

- [x] Persist global boolean and number variables across area transitions within
      the same session (already scaffolded; needs verification against Endar
      Spire flag checks).
      *(`GetGlobalBoolean` / `SetGlobalBoolean` / `GetGlobalNumber` /
      `SetGlobalNumber` are all implemented and stored on the `Module` object
      which persists across area loads.)*

### CI Test Coverage

- [x] Add unit tests for the core combat formulas (AC derivation, d20 hit/miss
      determination including natural-1 / natural-20 rules, ability-modifier
      integration) so that regressions in the combat path are caught
      automatically.
      *(`tests/engines/kotorbase/combat.cpp` added; wired into the Autotools
      `check_PROGRAMS` target via `tests/engines/kotorbase/rules.mk`.)*
- [x] Add unit tests for `CreatureInfo` ability-score storage, modifier
      calculation, and skill-rank round-trips.
      *(`tests/engines/kotorbase/creatureinfo.cpp` added.)*
- [x] Add unit tests for `Inventory` item add/remove/query operations (stacking,
      count tracking, empty-tag guards, assignment-operator independence).
      *(`tests/engines/kotorbase/inventory.cpp` added; wired into `rules.mk`.)*
- [x] Add unit tests for `Effect` (EffectHeal / EffectDamage type, amount, and
      damageType accessors; clone independence; HP arithmetic mirrors used by
      `applyEffectToObject`).
      *(`tests/engines/kotorbase/effect.cpp` added; wired into `rules.mk`.)*
- [x] Add unit tests for the NWScript faction-relationship helpers
      (`GetIsEnemy` / `GetIsFriend` / `GetIsNeutral`) covering all combinations
      of hostile, friendly, neutral, EndarSpire, and invalid factions plus a
      mutual-exclusivity property test.
      *(`tests/engines/kotorbase/nwscript_functions.cpp` added; wired into
      `rules.mk`.)*

---

## Out of Scope

The following items appear in ROADMAP.md but are **not** required for this
milestone:

- Flanking, cover, and line-of-sight calculations.
- Force powers and alignment tracking.
- Any area beyond the _Endar Spire_ (Taris, Dantooine, …).
- Pazaak, swoop racing, and other minigames.
- Full FMOD audio integration (ambient and VO may be absent).
- Bink/WebM cinematic playback for cutscenes (they may be skipped).
- Saving to disk and reloading a saved game.

---

## Success Metric

Milestone 1 is **complete** when a CI-reproducible automated playthrough script
(or a recorded manual walkthrough accepted by a maintainer) demonstrates all
eight acceptance criteria above passing without crashes or script errors.

---

---

# Milestone 2 — _Taris_ Beginnings: Alignment, XP & Party Joining

This milestone extends xoreos beyond the Endar Spire tutorial.  After the PC
escapes in the pod and crash-lands on **Taris**, Carth Onasi joins the party,
XP is awarded for kills and plot events, and alignment shifts begin to matter.

**Goal:** Wire and implement all NWScript engine functions needed for the first
Taris module (`tar_m02aa`) to load, hand control to the PC, allow Carth to join
the party, and start accumulating XP without crashing on script errors.

---

## Scope

Only KotOR I's Upper-City Taris entry area (`tar_m02aa`) is in scope.
KotOR II and other planets remain on the long-term roadmap.

---

## Acceptance Criteria

1. **Module transition** — the `end_m01aa` exit script fires `StartNewModule`
   and Taris loads without assertion failures or unhandled NWScript exceptions.
2. **Party formation** — `AddPartyMember` / `RemovePartyMember` / `IsNPCPartyMember`
   execute without crashing; Carth's `OnSpawn` script runs and he appears in
   the active party list.
3. **Alignment system** — `GetGoodEvilValue`, `SetGoodEvilValue`,
   `AdjustAlignment`, and `GetAlignmentGoodEvil` return correct clamped and
   bucketed values; the PC starts at 50 (neutral) and can be shifted.
4. **XP accumulation** — `GiveXPToCreature`, `SetXP`, and `GetXP` correctly
   track and expose the PC's accumulated XP; `GivePlotXP` awards plot XP to
   the party leader.
5. **Saving throws** — `FortitudeSave`, `ReflexSave`, and `WillSave` roll
   d20 + ability modifier + 10 vs DC; natural-1 always fails, natural-20 always
   succeeds, ties succeed.
6. **Save-game flag** — `GetLoadFromSaveGame` returns TRUE only when the module
   was entered via `loadSavedGame()`; returns FALSE after a normal transition.
7. **Graceful stubs** — `ShowLevelUpGUI` logs a warning and returns 0;
   `OpenStore` logs a warning; neither crashes the script system.
8. **Global string variables** — `GetGlobalString` and `SetGlobalString` round-
   trip correctly through the module's global-variable store.
9. **Party count query** — `GetPartyMemberCount` and `GetPartyMemberByIndex`
   reflect the live active-party size after `AddPartyMember` calls.
10. **Solo mode toggle** — `SetSoloMode` and `GetSoloMode` flip the module's
    solo-mode flag.

---

## Required Work

### NWScript Functions — newly wired (all ✅ this session)

- [x] `FortitudeSave` (ID 108), `ReflexSave` (ID 109), `WillSave` (ID 110) —
      saving-throw rolls with natural-1/20 rules; wired in kotor + kotor2 tables.
- [x] `GetGoodEvilValue` (ID 125), `GetAlignmentGoodEvil` (ID 127) — alignment
      accessors; wired in kotor + kotor2 tables.
- [x] `GetPartyMemberCount` (ID 126) — live party-size query; wired in both tables.
- [x] `AdjustAlignment` (ID 201) — apply alignment delta with clamping; wired.
- [x] `SetGlobalString` (ID 160), `GetGlobalString` (ID 194) — global string
      variable round-trip; wired in both tables.
- [x] `GetLoadFromSaveGame` (ID 251) — returns save-game entry flag; wired and
      implemented; `_loadedFromSaveGame` field set in `Module::loadSavedGame()`.
- [x] `ShowLevelUpGUI` (ID 265) — graceful stub; wired and implemented.
- [x] `OpenStore` (ID 378) — graceful stub; wired and implemented.
- [x] `GiveXPToCreature` (ID 393), `SetXP` (ID 394), `GetXP` (ID 395) — XP
      accumulation; wired and implemented on `Object::addPlotXP` / `getCurrentXP`.
- [x] `AddPartyMember` (ID 574), `RemovePartyMember` (ID 575) — active-party
      management; wired in kotor + kotor2 tables.
- [x] `IsNPCPartyMember` (ID 699) — NPC slot membership query; wired in both tables.
- [x] `GivePlotXP` (ID 714) — plot-XP award to party leader; wired in both tables.
- [x] `SetGoodEvilValue` (ID 750) — direct alignment setter; wired in both tables.
- [x] `SetSoloMode` (ID 753) — solo-mode toggle; wired in both tables.

### CI Test Coverage

- [x] Add unit tests for the alignment clamping / bucket logic (`GetGoodEvilValue`,
      `AdjustAlignment`, `GetAlignmentGoodEvil`) covering boundary values and
      overflow/underflow.
      *(`tests/engines/kotorbase/alignment_xp.cpp` added.)*
- [x] Add unit tests for XP accumulation (`GiveXPToCreature`, `SetXP`, `GetXP`)
      covering positive awards, zero/negative guards, and `SetXP` override.
      *(same file.)*
- [x] Add unit tests for the saving-throw d20 formula covering natural-1 auto-fail,
      natural-20 auto-success, exact-DC success, and insufficient roll failure.
      *(same file.)*

---

## Out of Scope for Milestone 2

- Level-up GUI implementation (ShowLevelUpGUI returns 0).
- Merchant/barter screen implementation (OpenStore is a stub).
- Force powers and Force alignment (separate track).
- Areas beyond Taris Upper-City entry.
- Saving to disk.

---

## Success Metric

Milestone 2 is **complete** when the Taris entry module loads and runs without
unhandled NWScript exceptions, Carth joins the active party, and all ten
acceptance criteria above are satisfied — verified by the unit tests and a
manual smoke-run of the module transition from Endar Spire.

---

---

# Milestone 3 — Taris Upper City: Combat Depth, Reputation & Item Queries

This milestone makes the **Taris Upper City** (`tar_m02aa`) fully explorable
with no unhandled NWScript exceptions.  After the Endar Spire → Taris
transition the player must be able to fight random gang members, query and
adjust inter-faction reputation, inspect item types from scripts, and advance
through the area without the engine crashing on unimplemented functions.

**Goal:** Implement the eight NWScript functions most frequently called by
Taris Upper-City scripts, extend the `Effect` engine type with four new
sub-types so that buff/debuff effects can be constructed without crashing, and
add a minimal (auto-assign) level-up path so that `ShowLevelUpGUI` no longer
silently discards the level.

---

## Scope

KotOR I's Taris Upper City area (`tar_m02aa`).  KotOR II and other planets
remain on the long-term roadmap.

---

## Acceptance Criteria

1. **GetDistanceBetween** — returns the Euclidean distance between two
   in-world objects; all NPC AI range checks stop throwing "unimplemented"
   warnings.
2. **GetReputation / AdjustReputation** — faction reputation round-trips
   through a module-level store; initial value derived from faction membership;
   `AdjustReputation` clamps the result to `[0, 100]`.
3. **EffectACIncrease / EffectAttackIncrease / EffectSkillIncrease** —
   effect-constructor functions return properly typed `Effect` objects; calling
   code can read back type, bonus value, and sub-type without crashing.
4. **EffectTemporaryHitpoints** — constructs a temporary-HP effect; applied via
   `applyEffectToObject` it adds to current HP (capped at max HP) on the target
   creature.
5. **GetBaseItemType** — returns the integer base-item index from an `Item`
   object; required by many inventory-management scripts.
6. **GetJournalQuestExperience** — returns the XP reward stored in the `qt`
   2DA row for a given plot ID; falls back to 0 gracefully if the 2DA is
   absent (asset-less CI environment).
7. **ShowLevelUpGUI (real path)** — when the PC has unspent level(s), auto-
   allocates skill points (max ranks in the PC's class-primary skills) and
   grants class hit-die HP; no longer a silent no-op.
8. **Item::getBaseItem accessor** — `Item` exposes a public `getBaseItem()`
   getter used by `GetBaseItemType` and future item-property checks.

---

## Required Work

### Effect Engine Type — new sub-types

- [x] Add `kEffectACIncrease`, `kEffectAttackIncrease`, `kEffectSkillIncrease`,
      and `kEffectTemporaryHitpoints` to the `EffectType` enum in `effect.h`.
      *(`effect.h` extended; `effect.cpp` unchanged — existing constructor
      and clone already handle arbitrary type/amount/damageType triples.)*
- [x] Extend `applyEffectToObject` to handle `kEffectTemporaryHitpoints`
      (adds amount to current HP, capped at max HP).
      *(handled in `functions_creatures.cpp`.)*

### Item — base-item accessor

- [x] Add `getBaseItem() const → int` to `Item` (header + translation unit).
      *(`item.h` gains the declaration; `item.cpp` gains the one-liner.)*

### Module — reputation store

- [x] Add `_reputations` map (`std::map<std::pair<int,int>, int>`) to
      `Module`; default value derived on first access from faction membership
      (hostile → 0, friendly → 100, neutral → 50).
- [x] Expose `getReputation(int sourceFaction, int targetFaction) → int` and
      `adjustReputation(int targetFaction, int sourceFaction, int delta)` on
      `Module`.

### NWScript Functions — newly wired

- [x] `GetDistanceBetween` (ID 151) — Euclidean distance between two objects.
- [x] `GetReputation` (ID 208) — delegates to `Module::getReputation`.
- [x] `AdjustReputation` (ID 209) — delegates to `Module::adjustReputation`.
- [x] `EffectACIncrease` (ID 115) — constructs `Effect(kEffectACIncrease, bonus)`.
- [x] `EffectAttackIncrease` (ID 118) — constructs `Effect(kEffectAttackIncrease, bonus)`.
- [x] `EffectSkillIncrease` (ID 351) — constructs `Effect(kEffectSkillIncrease, bonus, skillId)`.
- [x] `EffectTemporaryHitpoints` (ID 314) — constructs `Effect(kEffectTemporaryHitpoints, amount)`.
- [x] `GetBaseItemType` (ID 397) — calls `Item::getBaseItem()`.
- [x] `GetJournalQuestExperience` (ID 384) — reads XP from `questitems.2da`
      `xp` column; returns 0 if the 2DA is unavailable.

### Level-up auto-path

- [x] `ShowLevelUpGUI` (ID 265) — when the PC's accumulated XP exceeds the
      threshold for the next level, increment the class level, add hit-die HP,
      and distribute skill points to the top-ranked skills automatically;
      log the level-up so it is visible in debug output.

### CI Test Coverage

- [x] Unit tests for `GetDistanceBetween` covering zero distance, axis-aligned,
      and diagonal 3-D cases.
      *(`tests/engines/kotorbase/taris_upper_city.cpp` added.)*
- [x] Unit tests for `GetReputation` / `AdjustReputation`: initial faction
      derivation, clamping at 0 and 100, independent faction-pair storage.
      *(same file.)*
- [x] Unit tests for new `Effect` sub-types: type/amount/skill-id accessors
      and `clone` independence.
      *(same file.)*
- [x] Unit tests for `EffectTemporaryHitpoints` applied via the HP-cap logic.
      *(same file.)*

---

## Out of Scope for Milestone 3

- Full level-up GUI with feat selection and attribute point spending.
- Merchant/barter screen implementation (OpenStore remains a stub).
- Areas beyond Taris Upper City.
- Saving to disk.

---

## Success Metric

Milestone 3 is **complete** when Taris Upper City scripts run without
"unimplemented function" warnings for the eight functions listed above, the
new unit tests all pass under CI, and the PC can gain at least one level
automatically after accumulating sufficient XP in-session.

---

---

# Milestone 4 — Dantooine Arrival: Cinematic Scripting, Plot Flags & Camera

This milestone carries xoreos from the end of Taris through the galaxy map
and into the **Dantooine** entry module (`danm13`).  Dantooine's scripts make
heavy use of cinematic choreography functions (cutscene attacks, cutscene
crowd-control effects), camera mode control, plot flags, and difficulty
scaling.  None of these were implemented.

**Goal:** Wire and implement all NWScript functions needed for `danm13` to
load, play its arrival cutscene, and hand control to the PC without crashing
on script errors.

---

## Scope

KotOR I's Dantooine entry module (`danm13`) and the Taris → galaxy map →
Dantooine transition.  KotOR II and other planets remain on the long-term
roadmap.

---

## Acceptance Criteria

1. **Configurable start module** — `KOTOR_startModule` config key lets
   developers jump directly to any module (e.g. `danm13`) after character
   creation, enabling rapid iteration without replaying Endar Spire.
2. **GetGameDifficulty / GetDifficultyModifier** — return clamped integer
   difficulty (0–2) and float scaling factor (0.8 / 1.0 / 1.2) respectively;
   all combat AI range scripts stop throwing "unimplemented" warnings.
3. **CutsceneAttack / CutsceneMove** — choreographed attack and movement
   actions queue correctly on the target creature; arrival cutscene proceeds
   without script errors.
4. **EffectCutSceneHorrified / EffectCutSceneParalyze / EffectCutSceneStunned**
   — construct correctly typed `Effect` objects (Stunned / Paralyze / Stunned);
   `applyEffectToObject` handles them without crashing.
5. **GetPlotFlag / SetPlotFlag** — round-trip correctly on any `Object`;
   plot-critical NPCs (Jedi Masters, Bastila) are correctly flagged and immune
   to permanent death in scripted sequences.
6. **GetFactionEqual** — returns TRUE when both objects share the same faction
   ID; used by Dantooine AI to avoid friendly-fire.
7. **GetEffectType** — returns the integer `EffectType` stored in any `Effect`
   engine type; required by buff/debuff checking scripts.
8. **GetLastDamager** — returns the last creature that struck the target; used
   by `OnDamaged` handlers throughout Dantooine combat scripts.
9. **SetCameraMode** — logs the requested camera mode and returns without
   crashing; cutscene camera transitions no longer halt the script system.
10. **SetLockOrientationInDialog / SetLockHeadFollowInDialog** — both are safe
    no-ops; dialog sequencing scripts proceed without "unimplemented" warnings.

---

## Required Work

### Configurable start module

- [x] Add `KOTOR_startModule` config key written during `initGameConfig()`
      and read in `CharacterGenerationMenu::start()` so developers can jump
      to any module after char-gen.

### Difficulty system

- [x] `GetGameDifficulty` (ID 513) — reads `ConfigMan.getInt("difficulty", 1)`,
      clamps to `[0, 2]`; wired in kotor + kotor2 tables.
- [x] `GetDifficultyModifier` (ID 523) — returns `0.8f` / `1.0f` / `1.2f` per
      difficulty; wired in both tables.

### Cutscene action functions

- [x] `CutsceneAttack` (ID 503) — queues `kActionAttackObject` on the script
      caller targeting `oTarget`; choreography flags (point / modifier / miss)
      are logged but not yet modelled.
- [x] `CutsceneMove` (ID 507) — queues `kActionMoveToPoint` to `vDestination`
      on `oCreature`; movement-rate parameter accepted but not yet applied.

### Cutscene crowd-control effects

- [x] `EffectCutSceneHorrified` (ID 754) — returns `Effect(kEffectStunned, 0)`.
- [x] `EffectCutSceneParalyze` (ID 755) — returns `Effect(kEffectParalyze, 0)`.
- [x] `EffectCutSceneStunned` (ID 756) — returns `Effect(kEffectStunned, 0)`.

### Object — plot flag

- [x] Add `_plotFlag` (`bool`, default `false`) to `Object`.
- [x] Expose `getPlotFlag() const` and `setPlotFlag(bool)` on `Object`.
- [x] `GetPlotFlag` (ID 455) — delegates to `Object::getPlotFlag()`; wired in
      both tables.
- [x] `SetPlotFlag` (ID 456) — delegates to `Object::setPlotFlag()`; wired in
      both tables.

### Faction / effect introspection

- [x] `GetFactionEqual` (ID 172) — returns 1 if both objects share the same
      non-invalid faction; wired in both tables.
- [x] `GetEffectType` (ID 170) — dynamic-casts the `EngineType` parameter to
      `Effect` and returns `static_cast<int>(e->getType())`; wired in both
      tables.
- [x] `GetLastDamager` (ID 346) — returns `Creature::getLastHostileActor()` on
      the target; wired in both tables.

### Camera / dialog stubs

- [x] `SetCameraMode` (ID 504) — logs mode and returns; wired in both tables.
- [x] `SetLockOrientationInDialog` (ID 505) — safe no-op; wired in both tables.
- [x] `SetLockHeadFollowInDialog` (ID 506) — safe no-op; wired in both tables.

### CI Test Coverage

- [x] Unit tests for difficulty clamping and modifier scaling (`GetGameDifficulty`,
      `GetDifficultyModifier`).
      *(`tests/engines/kotorbase/dantooine_cinematic.cpp` added.)*
- [x] Unit tests for cutscene effect types (`EffectCutSceneHorrified`,
      `EffectCutSceneParalyze`, `EffectCutSceneStunned`).
      *(same file.)*
- [x] Unit tests for plot flag round-trip (`GetPlotFlag` / `SetPlotFlag`).
      *(same file.)*
- [x] Unit tests for `GetFactionEqual` covering same-faction, different-faction,
      and self-equality.
      *(same file.)*
- [x] Unit tests for `GetEffectType` accessor covering all Dantooine-relevant
      effect types.
      *(same file.)*
- [x] Unit tests for `GetLastDamager` null guard and post-hit state.
      *(same file.)*
- [x] Unit tests for camera stub behaviour (`SetCameraMode`,
      `SetLockOrientationInDialog`, `SetLockHeadFollowInDialog`).
      *(same file.)*

---

## Out of Scope for Milestone 4

- Dantooine Jedi Enclave interior scripting.
- Basilisk War Droid crash cutscene full fidelity (choreography flags).
- `SetCameraMode` runtime camera-mode switching.
- Force powers and Force alignment (separate track).
- Saving to disk.

---

## Success Metric

Milestone 4 is **complete** when `danm13` loads and runs its arrival scripts
without "unimplemented function" warnings for the functions listed above, all
new unit tests pass under CI, and the PC can walk through the Dantooine entry
area without the engine crashing on script errors.

---

---

# Milestone 6 — Dantooine Polish & Engine Fidelity

This milestone focuses on achieving mechanical parity and cinematic fluidity for the bridge from Taris to the completion of the Dantooine Jedi Trials. It moves the engine away from hardcoded logic and toward full data-driven resolution.

**Goal:** Implement data-driven Force Power resolution via `spells.2da`, integrate alignment-based resource scaling, polish combat reactions for cinematics, and verify the Dantooine Star Map reveal sequence.

---

## Scope

The bridge from the Endar Spire to the end of the Dantooine module (`danm13` through `danm17`).

---

## Acceptance Criteria

1.  **Data-Driven Spells** — Force Power costs and properties are loaded from `spells.2da`; hardcoded switch-cases in `ActionExecutor` are removed. ✅
2.  **Alignment Scaling** — Force Power costs scale based on the caster's alignment (opposite side penalty, same side discount); verified for Heal, Shock, and Plague. ✅
3.  **Lightsaber Deflection** — Implementation of opposed d20 + modifiers vs incoming blaster fire; natural-1 and natural-20 rules apply to the deflection roll. ✅
4.  **Skill Modifier Parity** — All skill checks (Computer Use, Repair, etc.) now correctly add Ability Modifiers (Int, Wis, etc.) to the base rank. ✅
5.  **Dynamic Cutscene Reactions** — `performCutsceneAttack` uses RNG-based flinching, dodging, and blocking animations to create fluid, non-repetitive duels. ✅
6.  **Smoothstep Camera** — Cinematic camera paths use a smoothstep (3t² - 2t³) interpolation instead of linear, providing professional, non-robotic movement. ✅
7.  **Dantooine Milestones** — 
    *   Mandalorian Ambush (Grove): Scripted entry and music stingers verified. ✅
    *   Star Map Reveal: Cinematic sweep and quest state update confirmed. ✅

---

## Required Work

### Combat & Force System

- [x] **spells.2da Integration**: parsed `forcepoints`, `hostile`, and `impactscript` from the 2DA table.
- [x] **Alignment Cost Logic**: implemented ±50%/25% resource scaling in `ActionExecutor::executeCastSpell`.
- [x] **Force Power Expansion**: added Shock, Lightning, Wound, Choke, and Plague effect archetypes.
- [x] **Lightsaber Deflection**: added `Creature::rollDeflection` hook to the blaster attack resolution path.

### Mechanical Polish

- [x] **Ability-Based Skills**: updated `Creature::getSkillModifier` to include Int for tech skills, Dex for stealth, etc.
- [x] **NPC Reactions**: added `Common::Random` hooks to `performCutsceneAttack` for dynamic animation selection.

### Cinematic System

- [x] **Fluid Interpolation**: updated `CameraController` to use smoothstep for all `cameraMoveAlongPath` calls.

---

## Out of Scope for Milestone 6

- Pazaak minigame (still a functional stub).
- Full 2DA lookup for feat costs and level-up requirements.
- Spline-based (Hermite/Catmull-Rom) camera control (smoothstep suffices for now).

---

## Success Metric

Milestone 6 is **complete** when the Dantooine Star Map sequence plays with full cinematic fluidity, Force Power resource management correctly reflects character alignment, and all planetary encounters through the Jedi Trials are verified stable.

---

# Milestone 7 — The Final Journey: Star Forge & Total Parity

This milestone marks the final phase of development for **Star Wars: Knights of the Old Republic** support. It encompasses the remaining planetary questlines (Tatooine, Kashyyyk, Manaan, Korriban), the Leviathan encounter, the Unknown World, and the ultimate assault on the **Star Forge**.

**Goal:** Achieve 100% NWScript functional coverage, complete all high-fidelity cinematic choreography systems, and ensure the entire campaign is playable from start to finish with full fidelity and parity with the original game.

---

## Scope

The complete Star Wars: Knights of the Old Republic I campaign.

---

## Acceptance Criteria

1.  **Full NWScript Parity** — All ~850 engine functions are implemented or safely stubbed; the engine no longer logs "unimplemented function" warnings for any KotOR I script.
2.  **Planetary Completion** — All major planetary hubs (Tatooine, Manaan, Kashyyyk, Korriban) load and resolve their primary quest arcs correctly.
3.  **Space Combat & Minigames** — Full implementation of SWMG (Turret) and Swoop Racing physics and logic systems.
4.  **The Leviathan & Unknown World** — Specialized cutscene and party-split logic for the mid-game and end-game transitions.
5.  **Star Forge Finale** — The final battle sequence, including the Bastila showdown and the Malak encounter, resolves with full mechanical and cinematic fidelity.
6.  **Persistence & Saving** — Full support for writing and reloading saved games to disk.

---

## Required Work

### NWScript & Engine
- [x] **Function Implementation Batch 1 & 2**: Completed ~50 critical functions for Taris/Dantooine. ✅
- [ ] **Systematic Stubbing**: Register the remaining ~300 unimplemented functions as safe stubs to ensure script stability.
- [x] **SWMG Minigame Hooks**: Implementation of speed, acceleration, and bullet tracking for space combat sequences. ✅

### Level-up & Character
- [ ] **Full Level-up GUI**: Replace the auto-assign path with a manual interface for feat and attribute selection.
- [ ] **Prestige & Influence**: (KotOR II focus) Prepare the engine foundation for secondary class transitions.

### Persistence
- [ ] **Serialization**: Implement the GFF-based save game format for player and world state.

---

## Success Metric

Milestone 7 is **complete** when a player can start a new game on the *Endar Spire* and reach the final credits on the *Star Forge* without utilizing external tools or experiencing game-breaking engine crashes.
