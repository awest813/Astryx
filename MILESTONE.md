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

# Milestone 3 — Geometry, Item Stacks, Faction Transfers & Module Queries

This milestone adds a new batch of NWScript utility functions that are called
by common KotOR I and KotOR II scripts across many modules and quest areas.

**Goal:** Implement and wire the next set of frequently-used NWScript functions
so that scripts relying on object-to-object distance queries, faction changes,
item stack sizes, and module filename detection run without falling through to
the `unimplementedFunction` warning path.

---

## Scope

KotOR I and KotOR II kotorbase layer.  Functions wired in both
`kotor/script/function_tables.h` and `kotor2/script/function_tables.h`.

---

## Acceptance Criteria

1. **`GetDistanceBetween` (ID 151)** — returns 3D Euclidean distance between two
   objects; negative on invalid input; wired in both tables.
2. **`SetFacingPoint` (ID 143)** — rotates OBJECT_SELF to face a world-space
   vector point; uses atan2 of (target − caller) in the XY plane.
3. **`AngleToVector` (ID 144)** — converts an angle in degrees to a 2D unit
   vector (x = cos(angle), y = sin(angle), z = 0).
4. **`VectorToAngle` (ID 145)** — converts a vector to an angle in degrees via
   atan2(y, x); round-trips with AngleToVector.
5. **`ActionGiveItem` (ID 135)** — transfers one item from the caller's inventory
   to the target creature's inventory.
6. **`ActionTakeItem` (ID 136)** — transfers one item from a source creature's
   inventory into the caller's inventory.
7. **`GetItemStackSize` (ID 138)** — returns the `_stackSize` of an Item object
   (1 for non-stackable, ≥ 1 for stacked consumables).
8. **`SetItemStackSize` (ID 150)** — sets the `_stackSize`; clamps values < 1 to 1.
9. **`GetFactionEqual` (ID 172)** — returns TRUE if both objects share the same
   non-invalid standard faction.
10. **`ChangeFaction` (ID 173)** — sets the first object's faction to match the
    second object's faction.
11. **`GetModuleFileName` (ID 210)** — returns the resource-ref name of the
    currently loaded module (e.g. `"end_m01aa"`).
12. **`GetGoingToBeAttackedBy` (ID 211)** — returns the first creature in the
    current area whose `getAttackTarget()` matches the given object.

---

## Required Work

### NWScript Functions — newly implemented and wired (all ✅ this session)

- [x] `GetDistanceBetween` (ID 151) — implementation already existed in
      `functions_object.cpp`; wired in kotor + kotor2 tables.
- [x] `SetFacingPoint` (ID 143) — implemented in `functions_action.cpp`; computes
      atan2 of (target − caller) in 2D and calls `setOrientation`; wired.
- [x] `AngleToVector` (ID 144) — implemented in `functions_math.cpp`; uses
      `cos`/`sin` with `Common::deg2rad`; wired in both tables.
- [x] `VectorToAngle` (ID 145) — implemented in `functions_math.cpp`; uses
      `atan2` with `Common::rad2deg`; wired in both tables.
- [x] `ActionGiveItem` (ID 135) — implemented in `functions_action.cpp`; removes
      item from caller inventory and adds to target inventory; wired.
- [x] `ActionTakeItem` (ID 136) — implemented in `functions_action.cpp`; removes
      item from source inventory and adds to caller inventory; wired.
- [x] `GetItemStackSize` (ID 138) / `SetItemStackSize` (ID 150) — `Item` class
      extended with `_stackSize` field and `getStackSize()` / `setStackSize()`;
      `CreateItemOnObject` now propagates `count` as the initial stack size; both
      NWScript functions wired in kotor + kotor2 tables.
- [x] `GetFactionEqual` (ID 172) — implemented in `functions_object.cpp`; checks
      non-invalid faction equality; wired in both tables.
- [x] `ChangeFaction` (ID 173) — implemented in `functions_object.cpp`; calls
      `setFaction(factionSrc->getFaction())`; wired in both tables.
- [x] `GetModuleFileName` (ID 210) — implemented in `functions_module.cpp`;
      returns `Module::getResRef()` (new getter for `_module` field); wired.
- [x] `GetGoingToBeAttackedBy` (ID 211) — implemented in `functions_object.cpp`;
      iterates area creatures and returns the first whose attack target is the
      given object; wired in both tables.

### Supporting changes

- [x] `Module::getResRef()` added (module.h / module.cpp) — returns `_module`.
- [x] `Item::getStackSize()` / `Item::setStackSize()` added (item.h / item.cpp).
- [x] `createItemOnObject` updated to propagate `count` to `item->setStackSize()`.
- [x] Pre-existing tests fixed: `TEST()` macro replaced with `GTEST_TEST()` in
      `tests/engines/kotorbase/endar_spire_golden.cpp` and
      `tests/smoke/crash_regression.cpp` (project uses `-DGTEST_DONT_DEFINE_TEST=1`).
- [x] `SUCCEED()` replaced with `GTEST_SUCCEED()` in `crash_regression.cpp`.

### CI Test Coverage

- [x] Add unit tests for 3D/2D distance arithmetic
      (`distanceBetweenOriginIs0`, `distanceBetweenAlongXAxis`,
      `distanceBetween3DPythagorean`, `distanceBetween2DIgnoresZ`).
- [x] Add unit tests for `AngleToVector` / `VectorToAngle` covering 0°, 90°,
      180°, unit-length invariant, and round-trip property.
- [x] Add unit tests for item stack-size clamp logic (default = 1, positive
      values pass through, values < 1 clamp to 1).
- [x] Add unit tests for `GetFactionEqual` covering same-faction, different-
      faction, and invalid-faction cases.
      *(`tests/engines/kotorbase/milestone3_functions.cpp` added; wired into
      `tests/engines/kotorbase/rules.mk`.)*

---

## Out of Scope for Milestone 3

- Full effect inspection (`GetFirstEffect`, `GetNextEffect`, `GetIsEffectValid`).
- Spell system functions (`GetSpellId`, `GetCasterLevel`, etc.).
- Advanced faction queries (`GetFactionWeakestMember`, `AdjustReputation`, etc.).
- Listen/pattern matching functions.
- Areas beyond Taris.

---

## Success Metric

Milestone 3 is **complete** when all 92 unit tests pass (verified by `make check`)
and the 12 newly implemented NWScript functions no longer appear in the
`unimplementedFunction` warning path during typical Taris script execution.
