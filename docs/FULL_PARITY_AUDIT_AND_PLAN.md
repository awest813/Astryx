# KotOR I Full Parity — Status Audit & Plan

**Date:** 2026-07-29  
**Scope:** Star Wars: Knights of the Old Republic I campaign parity in Astryx  
**Audience:** Maintainers and coding agents executing Milestone 7 work

This document is the honest baseline for “full parity.” It reconciles marketing docs
(`README.md`, `ROADMAP.md`) with code reality and defines a phased plan to reach a
verified Endar Spire → Star Forge playthrough.

---

## 1. Executive verdict

**Astryx is not at full campaign parity.** Early systems engineering is substantial
(chargen, d20 combat core, party/globals, dialog/journal, partial save, Dantooine-
oriented cinematics, Pazaak shell). Mid/late campaign content is largely unproven.
About **31% of KotOR I NWScript bindings are stubs** that return safe defaults and
can fail plot gates silently.

| Claim (README / ROADMAP) | Reality |
|--------------------------|---------|
| “100% NWScript Coverage” | **100% registered** (no null pointers). Behavior: **538/785 real**, **169 `stubFunction`**, **78 `stubSWMGFunction`** ≈ **31.5% incomplete** |
| Core systems all ✅ (flanking, LOS, full combat AI, stores, minigames) | Mixed / partial. `Creature::isFlankedBy` always returns `false`. Cover/LOS not found. AI often queues nearest-target attack only |
| “Capable of full experience prologue → Star Forge” | **Aspirational** — Milestone 7 goal, not demonstrated |
| Milestone 6 complete / M7 active | Plausible as engineering intent; **manual smoke for Endar Spire → Dantooine is entirely unchecked** |
| Supported-slice docs (Endar + Taris entry) | Still the **correct conservative** product stance |

**One-liner:** Early-to-mid KotOR *systems* are real; full-campaign *playability* is not verified, and stub density will softlock mid/late quests unless replaced with behavioral implementations.

---

## 2. Current status (code-backed)

### 2.1 Milestone checklist (from `MILESTONE.md`)

| Milestone | Theme | Doc status | Verification status |
|-----------|--------|------------|---------------------|
| M1 | Endar Spire tutorial | Acceptance items marked ✅ | Unit golden tests exist; **manual smoke unchecked** |
| M2 | Taris beginnings (party, XP, alignment) | ✅ | Unit tests; **manual smoke unchecked** |
| M3 | Taris Upper City depth | ✅ | Unit tests; area not live-smoke verified |
| M4 | Dantooine arrival / cinematic natives | ✅ | Unit tests; **manual path unchecked** |
| M5 | *(missing section in `MILESTONE.md`)* | README claims swoop/Brejik/Hawk | No dedicated milestone section |
| M6 | Dantooine polish / Force / Star Map | Marked complete | Not end-to-end proven in smoke docs |
| M7 | Planetary hubs → Star Forge / total parity | Active; level-up + save still listed open | Save writers + level-up GUI exist but **campaign-unproven** |

### 2.2 NWScript coverage (KotOR I)

Source: `src/engines/kotor/script/function_tables.h`

| Metric | Count |
|--------|------:|
| Total bindings | **785** |
| Named real handlers | **538** |
| `stubFunction` | **169** |
| `stubSWMGFunction` | **78** |
| Incomplete total | **247 (31.5%)** |

Stub runtime (`src/engines/kotorbase/script/functions_stub.cpp`) logs at debug level and
returns typed defaults (0 / empty / pass-through Effect). Scripts **continue**, so
failures often look like softlocks, not crashes.

**Stub density by domain (approx.):**

| Domain | Stubs | Campaign impact |
|--------|------:|-----------------|
| SWMG / minigame | 78 | Blocks Hawk turret + swoop fidelity |
| Effects (immunity, regen, sleep, AoE, temp FP, …) | ~35 | Boss fights, Force powers, scripted CC |
| Other (AoE iterators, AI level, encounters, …) | ~64 | Broad quest/AI breakage |
| Faction aggregates + surrender-by-faction | 14 | Gang wars, mass AI behavior |
| Spell cast context (`GetSpellId`, DC, target, …) | 9 | Impact scripts / Force resolution |
| Audio object fine control | 9 | Atmosphere / VO hooks |
| Economy / item events | 9 | Activated items, cost modifiers |
| Listening patterns | 7 | Bark / ambient NPC reactions |
| Combat queries (last killer, damage dealt, …) | 5 | OnDeath rewards, quest triggers |
| Surrender / formation | 4 | Scripted stand-downs |
| Dialog one-liners / conversation events | 4 | Cutscene barks |
| Party (`AddToParty` / `RemoveFromParty`) | 3 | Distinct from implemented `AddPartyMember` |
| Force / multiclass | 3 | Jedi class transition risk |
| Day/night | 2 | Time-gated content |
| RevealMap | 1 | Exploration scripts |

**KotOR II note (out of primary scope):** 886 bindings; **439 + 103 stubs** (~61% incomplete). Do not chase K2 until K1 campaign is verified.

### 2.3 Subsystem matrix

| Subsystem | Status | Evidence / gap |
|-----------|--------|----------------|
| Character creation | **Implemented** | `src/engines/kotor/gui/chargen/` |
| Level-up GUI | **Partial → mostly present** | `levelup*.cpp` + `Game::showLevelUpGUI`; docs still list as M7 open — needs fidelity audit vs original |
| Combat (d20 / saves / some feats) | **Partial** | Attack rolls, saves, deflection hooks exist; flanking stubbed false; Force effects still partly hardcoded |
| Combat AI | **Partial** | Nearest-target attack archetypes; `SetAILevel` stubbed |
| Party / companions | **Partial** | `AddPartyMember` path wired; `AddToParty`/`RemoveFromParty` stubbed |
| Dialogue / journal | **Partial** | DLG + journal APIs; listening API stubbed; some speak/anim paths thin |
| Cutscenes / camera | **Partial** | CutsceneAttack/Move, smoothstep camera; many locks are no-ops |
| Inventory / equipment / stores | **Partial** | Early-game looting/equip; `OpenStore` + Store GUI exist; `ChangeItemCost` stubbed |
| Save / load | **Partial** | `Module::saveGame` / load path + unit round-trips; full world fidelity unverified |
| Pazaak | **Partial** | Engine + GUI shell; needs live merchant/quest verification |
| Swoop | **Partial** | Lightweight `SwoopMinigame` state; most SWMG natives stubbed |
| Turret / SWMG space | **Stub-heavy** | Encounter orchestration + movie; ~80% SWMG API stubbed |
| Galaxy map / travel | **Partial** | Five planets only (`tat`, `kas`, `man`, `kor`, `dan`) — **no Unknown World / Star Forge / Leviathan destinations** in `galaxymap.cpp` |
| Alignment / reputation / plot flags | **Partial** | Core APIs for M2–M4; faction aggregates stubbed |
| Audio / video | **Partial** | Movie queue + SoundMan; many sound-object natives stubbed |

### 2.4 Verification reality

**Automated (unit / model):** `tests/engines/kotorbase/` — 17 suites covering formulas, party/globals models, cinematic stubs, swoop kinematics, save serialization writers. Most **do not require live game archives** and therefore cannot prove module playthroughs.

**Scripted smoke:** `tests/smoke/*.ps1` runs unit binaries only.

**Manual smoke (all boxes still `[ ]`):**

- `docs/MILESTONE_A_SMOKE.md` — Endar Spire acceptance walkthrough
- `docs/KOTOR_PROGRESS_TO_DANTOOINE_SMOKE.md` — Main menu → Dantooine

**Supported content policy** (`docs/SUPPORTED_CONTENT.md`, `docs/PLAYABILITY_DEFINITION.md`) correctly still prioritizes Endar Spire, then Taris entry — **prefer this over README optimism when triaging**.

---

## 3. Top blockers for full campaign parity

Ordered by severity for an Endar → Star Forge run:

1. **No verified contiguous play path** past early modules (manual smoke unchecked).
2. **247 stubbed natives with silent wrong defaults** → quiet softlocks.
3. **SWMG / turret / swoop incomplete** → Hawk travel combat sequences fail or fake-complete.
4. **Effect / immunity / Force context stubs** → boss and Jedi content incorrect.
5. **Surrender / last-killer / damage-dealt stubs** → common quest resolution patterns break.
6. **Combat AI + flanking overclaimed** → tactical/scripted fights diverge from original.
7. **Force resolution still partly hardcoded** vs full `spells.2da` behavior.
8. **Save/load not campaign-proven** across modules, party, and world locals.
9. **Galaxy map / late destinations incomplete** (no Star Forge / Unknown World / Leviathan entries).
10. **`AddMultiClass` / class-transition foundation weak** → Jedi Enclave / prestige-adjacent paths risky.

---

## 4. Definition of “full parity” (acceptance bar)

Full KotOR I parity means a player can, **without debug console or module overrides**:

1. Complete **Endar Spire → Taris → Dantooine → Tatooine / Kashyyyk / Manaan / Korriban** (any legal order after Dantooine unlocks).
2. Complete **Leviathan**, **Unknown World**, and **Star Forge** finale (Bastila + Malak).
3. Play required **minigames** (Pazaak where quest-gated, swoop races, Hawk turret) to mechanical success — not movie skip alone.
4. **Save and reload** mid-campaign without losing plot globals, party, inventory, or area state needed for progression.
5. Experience **no crash** and **no softlock** on critical path; remaining stub natives must be proven unused on that path or replaced.
6. NWScript: either **behavioral implementation** or an **explicit documented no-op** with evidence the campaign never depends on it for progression.

Non-goals for this plan (track separately): KotOR II campaign parity, mobile/console ports, pixel-perfect renderer parity, full VO/FMOD polish beyond progression needs.

---

## 5. Phased plan

### Phase 0 — Honesty & instrumentation (foundation)

**Goal:** Stop overclaiming; make gaps visible.

- [x] Treat this document as the working M7 plan; update `README.md` / `ROADMAP.md` badges and ✅ lists to match stub counts and unverified smoke.
- [ ] Fill `docs/SCRIPT_COVERAGE.log` usage: enable coverage logging in playtests; publish per-module stub hit lists.
- [x] Add CI job (or extend `scripts/verify_kotor_nwscript_stubs.py`) that reports **stub vs real counts** and fails on regressions that *increase* stubs for previously-real functions.
- [ ] Reconcile `MILESTONE.md`: add missing M5 section or fold its claims into M6/M7 with honest checkboxes.

**Exit:** Docs match code; every agent works from one parity definition.

---

### Phase 1 — Verify the supported early path

**Goal:** Prove Endar Spire → Taris entry → Dantooine arrival without crash/softlock.

1. Run and check off `docs/MILESTONE_A_SMOKE.md` against real game data.
2. Run and check off `docs/KOTOR_PROGRESS_TO_DANTOOINE_SMOKE.md`.
3. For each failure, classify per `docs/SUPPORTED_CONTENT.md`: `crash` | `progression blocker` | `logic bug` | `visual bug` | `missing parity` | `qol`.
4. Fix only `crash` / `progression blocker` until both checklists pass.
5. Capture stub hits from `SCRIPT_COVERAGE.log` on this path → **Phase 2 priority queue**.

**Exit:** Maintainer-accepted smoke of Main Menu → Dantooine with zero critical blockers.

---

### Phase 2 — Critical NWScript de-stubbing (campaign blockers)

**Goal:** Replace silent stubs that gate quests, combat resolution, and scripted AI.

Work in priority bands (implement + unit test + wire kotor *and* kotor2 tables when shared):

| Band | Functions / themes | Why |
|------|--------------------|-----|
| **P0** | `GetLastKiller`, `GetDamageDealtByType`, `GetTotalDamageDealt`, `GetLastAttackResult`, `GetLastWeaponUsed` | OnDeath / combat quest hooks — **wired 2026-07-29** |
| **P0** | `SurrenderToEnemies`, `ActionSurrenderToEnemies`, `SurrenderByFaction`, `SurrenderRetainBuffs` | Stand-down sequences — **wired 2026-07-29** |
| **P0** | `GetSpellId`, `GetSpellTarget`, `GetSpellSaveDC`, `GetLastSpellHarmful`, `EventSpellCastAt` (+ related) | Force/item impact scripts — **wired 2026-07-29** |
| **P0** | `EffectImmunity`, `GetIsImmune`, `EffectDamageImmunityIncrease`, `EffectSpellImmunity` (+ Sleep/Regenerate/TempFP) | Boss / Jedi immunity — **wired 2026-07-29** |
| **P1** | AoE + persistent-object iterators, remaining Effect*, Magical/Supernatural/Extraordinary | Mid-game combat & areas — **wired 2026-07-29** |
| **P1** | `SpeakOneLinerConversation`, `GetLastConversation` | Cutscene / bark flow — **wired 2026-07-29** |
| **P1** | `AddToParty` / `RemoveFromParty` (align with party member APIs) | Companion scripts — **wired 2026-07-29** |
| **P1** | `GetIsDay` / `GetIsNight` (real time or module clock) | Time-gated content — **wired 2026-07-29** |
| **P1** | `RevealMap`, `SetAILevel`, `ApplyEffectAtLocation` | Exploration / AI / AoE apply — **wired 2026-07-29** |
| **P2** | Faction aggregate queries, listening API | Quality + secondary quests |

**Method:** For each band, (1) grep game `.ncs`/decompiled usage where available, (2) implement minimal correct semantics, (3) add focused unit tests, (4) re-smoke Phase 1 path.

**Exit:** Stub hit rate on Phase 1 path is zero for P0; P1 stubs either implemented or proven unused on critical path.

---

### Phase 3 — Combat & Force fidelity

**Goal:** Match original d20 + Force behavior enough that scripted and open combat remain fair and progressive.

- [ ] Implement real flanking (area context; opposite-side attackers) — replace `isFlankedBy` false stub.
- [ ] Add cover / LOS checks where combat and Force targeting depend on them.
- [ ] Expand AI beyond nearest-target attack (ability selection, Force use, flee/surrender hooks).
- [ ] Finish data-driven Force resolution: drive effects from `spells.2da` / impact scripts; remove remaining hardcoded switches where possible.
- [ ] Wire remaining combat natives (`EffectAssuredHit`, deflection effects, `GetReflexAdjustedDamage`, etc.) as needed by live encounters.

**Exit:** Endar mock combat + Taris street fights + Dantooine Jedi trials behave correctly under unit + manual combat smoke.

---

### Phase 4 — Persistence & character progression

**Goal:** Campaign-safe save/load and trustworthy level-up.

- [ ] Audit GFF/ERF save contents vs original SAVEGAME expectations (PC, party, inventory, equipment, globals, locals, journal, explored map, module/area).
- [ ] Round-trip tests: save in module A → load → transition → save in module B.
- [ ] Level-up GUI fidelity pass: attributes, skills, feats, Force powers vs `feat.2da` / class tables; keep autolevel as debug fallback only.
- [ ] Implement `AddMultiClass` / Jedi class transition path used on Dantooine.

**Exit:** Save/load and level-up used successfully across at least Endar → Taris → Dantooine without state loss.

---

### Phase 5 — Minigames (progression-critical)

**Goal:** Quest-gated minigames completable.

| Minigame | Work |
|----------|------|
| **Pazaak** | Verify `PlayPazaak` / result globals against Taris (and later) quest scripts; fix GUI/rules gaps |
| **Swoop** | Flesh `SwoopMinigame` + SWMG natives needed for race modules; obstacle/finish events must set plot globals correctly |
| **Turret / Hawk combat** | Replace movie-only orchestration with SWMG bullet/follower/HP natives required by encounter scripts |

Track SWMG stub burn-down (78 → near 0 for used natives). Unused SWMG entries may remain stubs if coverage logs prove they are never called.

**Exit:** Taris swoop + at least one Hawk turret sequence completable without cheat skips.

---

### Phase 6 — Planetary hubs (content expansion)

**Goal:** Primary quest arcs on each planet load and resolve.

Order (matches galaxy unlock logic; adjust if smoke reveals harder blockers earlier):

1. **Tatooine** (`tat_*`) — dune sea, Sand People, Star Map
2. **Kashyyyk** (`kas_*`) — Czerka / Wookiee questline
3. **Manaan** (`man*`) — Ahto / Hrakert
4. **Korriban** (`korr_*`) — academy / Star Map
5. Cross-cutting: Ebon Hawk interior, companion personal quests as they unlock

Per planet:

1. Module load smoke (no unimplemented fatal paths).
2. Stub-hit harvest → implement P0/P1 natives for that planet.
3. Main quest arc manual checklist (new doc under `docs/smoke/`).
4. Store, Pazaak, and combat encounters on critical path.

Extend `galaxymap.cpp` planet table as destinations become reachable (keep Dantooine + four hubs; add late destinations in Phase 7).

**Exit:** All four outer hubs’ primary Star Map quests completable.

---

### Phase 7 — Mid/late game & finale

**Goal:** Leviathan → Unknown World → Star Forge credits.

- [ ] Leviathan party-split / capture logic and related natives.
- [ ] Unknown World modules + galaxy map / travel hooks.
- [ ] Star Forge assault, Bastila confrontation, Malak fight (immunities, Force, cutscene choreography).
- [ ] Ending movies / module exit to credits.
- [ ] Full-campaign save/load spots before each act transition.

**Exit:** New game → credits without external tools; critical-path stub hits = 0.

---

### Phase 8 — Parity polish (post-credits bar)

Only after Phase 7 exit:

- Remaining visual/audio parity, optional side quests, formation AI, listening patterns, economic balance.
- KotOR II campaign track (separate milestone doc).
- Replace remaining “architectural stubs” with either real impl or `UNSUPPORTED_BY_DESIGN` documentation.

---

## 6. Working rules for agents

1. **One supported slice at a time** — do not open Tatooine work while Endar/Taris/Dantooine smoke is red (`PLAYABILITY_DEFINITION.md`).
2. **Prefer de-stubbing natives that smoke/logs prove are hit** over speculative API completion.
3. **Never claim ✅ in ROADMAP/README without** unit coverage *and* smoke evidence.
4. **Classify bugs** with the supported-content tags before fixing.
5. **Wire kotor + kotor2** function tables together when the implementation lives in `kotorbase`.
6. **No exploit/debug dependency** in acceptance — config shortcuts (`KOTOR_startModule`) are for development only.

---

## 7. Suggested near-term execution order (next agents)

| Order | Task | Deliverable |
|------:|------|-------------|
| 1 | Phase 0 doc honesty pass | README/ROADMAP aligned with stub counts |
| 2 | Phase 1 Endar Spire live smoke | Checked `MILESTONE_A_SMOKE.md` + bugfix PRs |
| 3 | Phase 1 Taris → Dantooine smoke | Checked progression smoke + fixes |
| 4 | Phase 2 P0 combat/quest natives | Stub burn-down PR + tests |
| 5 | Phase 4 save round-trip on early path | Serialization fidelity PR |
| 6 | Phase 5 swoop/turret critical natives | Minigame progression PR |
| 7 | Then Phases 3 → 6 → 7 in order | Planet/finale milestones |

---

## 8. Key source pointers

| Topic | Path |
|-------|------|
| Function table | `src/engines/kotor/script/function_tables.h` |
| Stub runtime | `src/engines/kotorbase/script/functions_stub.cpp` |
| Stub verifier | `scripts/verify_kotor_nwscript_stubs.py` |
| Galaxy map planets | `src/engines/kotor/gui/ingame/galaxymap.cpp` |
| Flanking stub | `src/engines/kotorbase/creature.cpp` (`isFlankedBy`) |
| Save write/load | `src/engines/kotorbase/module.cpp` |
| Level-up GUI | `src/engines/kotor/gui/ingame/levelup*.cpp` |
| Milestone claims | `MILESTONE.md`, `README.md`, `ROADMAP.md` |
| Conservative support policy | `docs/SUPPORTED_CONTENT.md`, `docs/PLAYABILITY_DEFINITION.md` |
| Smoke checklists | `docs/MILESTONE_A_SMOKE.md`, `docs/KOTOR_PROGRESS_TO_DANTOOINE_SMOKE.md` |

---

## 9. Success metric

**Full parity is done when** a maintainer-recorded playthrough (or automated module-sequence smoke with game data) completes the KotOR I campaign from Endar Spire to Star Forge credits with:

- zero critical crashes / softlocks,
- required minigames completable,
- save/load usable across act boundaries,
- and NWScript stub hits on the critical path eliminated or explicitly waived with evidence.
