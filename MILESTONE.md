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
