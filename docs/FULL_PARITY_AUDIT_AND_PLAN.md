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
KotOR I NWScript is **100% registered** (786/786 named handlers, 0 generic/SWMG stubs);
many handlers remain thin vs original semantics and can still softlock plot gates.

| Claim (README / ROADMAP) | Reality |
|--------------------------|---------|
| “100% NWScript Coverage” | **100% registered** and **0 `stubFunction` / 0 `stubSWMGFunction` on KotOR I** (786/786 named handlers). Many handlers are still thin/simplified vs original; KotOR II still has stubs. |
| Core systems all ✅ (flanking, LOS, full combat AI, stores, minigames) | Partial. Flanking + closed-door LOS exist; Force uses spells.2da impact fallback; AI Force-casts when FP allows. Stores/minigames/live progression still incomplete |
| “Capable of full experience prologue → Star Forge” | **Aspirational** — Milestone 7 goal, not demonstrated |
| Milestone 6 complete / M7 active | Plausible as engineering intent; **manual smoke for Endar Spire → Dantooine is entirely unchecked** |
| Supported-slice docs (Endar + Taris entry) | Still the **correct conservative** product stance |

**One-liner:** Early-to-mid KotOR *systems* are real; full-campaign *playability* is not verified; deepen thin natives and prove live smoke before claiming parity.

---

## 2. Current status (code-backed)

### 2.1 Milestone checklist (from `MILESTONE.md`)

| Milestone | Theme | Doc status | Verification status |
|-----------|--------|------------|---------------------|
| M1 | Endar Spire tutorial | Acceptance items marked ✅ | Unit golden tests exist; **manual smoke unchecked** |
| M2 | Taris beginnings (party, XP, alignment) | ✅ | Unit tests; **manual smoke unchecked** |
| M3 | Taris Upper City depth | ✅ | Unit tests; area not live-smoke verified |
| M4 | Dantooine arrival / cinematic natives | ✅ | Unit tests; **manual path unchecked** |
| M5 | *(missing section in `MILESTONE.md`)* | Swoop/Brejik/Hawk claims live in M6/M7 engineering notes | No dedicated milestone section |
| M6 | Dantooine polish / Force / Star Map | Marked complete | Not end-to-end proven in smoke docs |
| M7 | Planetary hubs → Star Forge / total parity | Active; level-up + save still listed open | Save writers + level-up GUI exist but **campaign-unproven** |

### 2.2 NWScript coverage (KotOR I)

Source: `src/engines/kotor/script/function_tables.h`

| Metric | Count |
|--------|------:|
| Total bindings | **786** |
| Named real handlers | **786** |
| `stubFunction` | **0** (KotOR I) |
| `stubSWMGFunction` | **0** (KotOR I and KotOR II) |
| Thin / simplified handlers | Many (SWMG depth, audio, formation, late Force) |

KotOR II still routes **273** natives through generic `stubFunction` (886 total bindings).
Thin K1 handlers can still fail plot gates silently when semantics diverge from the original.

**Thin-handler risk by domain (approx.; formerly stubbed, now named but simplified):**

| Domain | Risk | Campaign impact |
|--------|------:|-----------------|
| SWMG / minigame | Live depth | K1 wired; deepen simulation + live smoke |
| Effects (immunity, regen, sleep, AoE, temp FP, …) | Medium | Boss fights, Force powers, scripted CC |
| AoE iterators / AI level / encounters | Medium | Broad quest/AI breakage |
| Faction aggregates + surrender-by-faction | Medium | Gang wars, mass AI behavior |
| Spell cast context (`GetSpellId`, DC, target, …) | Medium | Impact scripts / Force resolution |
| Audio object fine control | Low–med | Atmosphere / VO hooks |
| Economy / item events | Medium | Activated items, cost modifiers |
| Listening patterns | Medium | Bark / ambient NPC reactions |
| Combat queries (last killer, damage dealt, …) | Medium | OnDeath rewards, quest triggers |
| Formation / dialog one-liners | Low–med | Cutscene barks / stand-downs |

**KotOR II note (out of primary scope):** Still has hundreds of generic stubs. Do not chase K2 until K1 campaign is verified.

### 2.3 Subsystem matrix

| Subsystem | Status | Evidence / gap |
|-----------|--------|----------------|
| Character creation | **Implemented** | `src/engines/kotor/gui/chargen/` |
| Level-up GUI | **Partial → mostly present** | `levelup*.cpp` + `Game::showLevelUpGUI`; docs still list as M7 open — needs fidelity audit vs original |
| Combat (d20 / saves / some feats) | **Partial** | Attack rolls, saves, deflection; flanking + closed-door LOS; Force partly hardcoded + spells.2da impact fallback |
| Combat AI | **Partial** | Nearest-target attack; Force cast when FP ≥ 10; `SetAILevel` thin |
| Party / companions | **Partial** | `AddPartyMember` path wired; party add/remove named but needs live verification |
| Dialogue / journal | **Partial** | DLG + journal APIs; listening named; some speak/anim paths thin |
| Cutscenes / camera | **Partial** | CutsceneAttack/Move, smoothstep camera; many locks are no-ops |
| Inventory / equipment / stores | **Partial** | Early-game looting/equip; `OpenStore` + Store GUI exist; cost modifiers thin |
| Save / load | **Partial** | Module save/load + unit round-trips (incl. journal/planet flags); GFF 16-char labels fixed; full world fidelity unverified |
| Pazaak | **Partial** | Engine + GUI shell; needs live merchant/quest verification |
| Swoop | **Partial** | Expanded `SwoopMinigame` state; K1 SWMG natives wired (simulation-backed / thin) |
| Turret / SWMG space | **Partial** | Encounter orchestration + movie; K1 SWMG API wired, live fidelity unproven |
| Galaxy map / travel | **Partial** | Hubs + Unknown World / Star Forge destinations in `galaxymap.cpp` (indices 6/7); Leviathan remains script-triggered |
| Alignment / reputation / plot flags | **Partial** | Core APIs for M2–M4; faction aggregates named but thin |
| Audio / video | **Partial** | Movie queue + SoundMan; many sound-object natives named but thin |

### 2.4 Verification reality

**Automated (unit / model):** `tests/engines/kotorbase/` — **20** suites covering formulas, party/globals models, cinematic stubs, swoop kinematics, save serialization, flanking/LOS, and Phase-0 natives. Most **do not require live game archives** and therefore cannot prove module playthroughs.

**Scripted smoke:** `tests/smoke/*.ps1` runs unit binaries only.

**Manual smoke (all boxes still `[ ]`):**

- `docs/MILESTONE_A_SMOKE.md` — Endar Spire acceptance walkthrough
- `docs/KOTOR_PROGRESS_TO_DANTOOINE_SMOKE.md` — Main menu → Dantooine

**Supported content policy** (`docs/SUPPORTED_CONTENT.md`, `docs/PLAYABILITY_DEFINITION.md`) correctly still prioritizes Endar Spire, then Taris entry — **prefer this over README optimism when triaging**.

---

## 3. Top blockers for full campaign parity

Ordered by severity for an Endar → Star Forge run:

1. **No verified contiguous play path** past early modules (manual smoke unchecked; needs game data).
2. **Thin / simplified natives** (K1 is 786/786 registered, 0 stubs) — audio, formation, late Force, and some SWMG handlers can still softlock if semantics diverge.
3. **SWMG / turret / swoop live fidelity** — named object getters exist; physics/scoring/encounter scripts unproven live.
4. **Force still partly hardcoded** when `spells.2da` impact scripts are missing; C++ fallbacks remain.
5. **Save/load not campaign-proven** across modules, party, world locals, and creature area persistence.
6. **Combat AI / LOS limited** — flanking + closed-door LOS exist; no full cover geometry or advanced AI.
7. **Effect list / resistance depth** — iterators/clear exist for ActiveEffect list; Force resistance / immunities are not fully duration-tracked.
8. **Level-up GUI fidelity** vs original feat/Force tables still needs an audit pass.
9. **KotOR II** still has hundreds of generic stubs (out of K1 primary scope).
10. **Late-game modules** (Leviathan / Unknown World / Star Forge) loadable via galaxy map flags but not live-smoke verified.

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
- [x] Retail K1 native IDs restored after table repair (`GetSkillRank`@315, `GetLastAttackType`@317, `GetIsInCombat`@320, `SetLocked`@324); do not re-remap those slots.
- [x] Creature GFF load / combat / AI statements un-swallowed from comment corruption (2026-07-29).
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
| **P2** | Faction aggregate queries, listening API | Quality + secondary quests — **wired 2026-07-29** |
| **P2** | `GetModuleFileName`, `GetReflexAdjustedDamage`, `GetIsPlayableRacialType`, `GetTargetLocation` | Misc campaign helpers — **wired 2026-07-29** |
| **P3** | Remaining audio/item/event stubs, SWMG burn-down | Fidelity / minigames — **wired 2026-07-29** (K1: 0 generic / 0 SWMG stubs; many thin) |

**Method:** For each band, (1) grep game `.ncs`/decompiled usage where available, (2) implement minimal correct semantics, (3) add focused unit tests, (4) re-smoke Phase 1 path.

**Exit:** Stub hit rate on Phase 1 path is zero for P0; P1 stubs either implemented or proven unused on critical path.

---

### Phase 3 — Combat & Force fidelity

**Goal:** Match original d20 + Force behavior enough that scripted and open combat remain fair and progressive.

- [x] Implement real flanking (area context; opposite-side attackers) — replace `isFlankedBy` false stub — **2026-07-29**
- [x] Add cover / LOS checks where combat and Force targeting depend on them — **2026-07-29** (closed doors block ranged + hostile Force)
- [x] Expand Force-user AI beyond nearest-target melee (cast known powers when FP available) — **2026-07-29**
- [x] Unknown powers fall through to `spells.2da` impact scripts — **2026-07-29**
- [x] Prefer `spells.2da` impact scripts over hardcoded C++ when present — **2026-07-29**
- [x] `GetSpellTargetObject` / `GetLastSpellCaster` / `GetLastSpell` / `GetCasterLevel` use cast context — **2026-07-29**
- [x] `ResistForce` (spell immunity + Force resistance check); `Effect` carries spell id; Force resistance tracking — **2026-07-29**
- [x] `GetWasForcePowerSuccessful` reads unsuccessful flag; `ApplyEffectToObject` duration/spellId; effect list natives — **2026-07-29**
- [ ] Finish remaining hardcoded Force switches → full data-driven only (retire C++ fallbacks)
- [x] `AddMultiClass` increments class levels (Dantooine Jedi transition path) — **2026-07-29**
- [ ] Wire remaining combat natives as needed by live encounters.

**Exit:** Endar mock combat + Taris street fights + Dantooine Jedi trials behave correctly under unit + manual combat smoke.

---

### Phase 4 — Persistence & character progression

**Goal:** Campaign-safe save/load and trustworthy level-up.

- [ ] Audit GFF/ERF save contents vs original SAVEGAME expectations (PC, party, inventory, equipment, globals, locals, journal, explored map, module/area).
- [x] Persist galaxy-map planet availability / selectability / selected planet in module save state — **2026-07-29**
- [x] GFF3 label round-trip: truncate/lookup at 16 chars so journal & return-dest fields load — **2026-07-29**
- [x] Persist Force power list across `CreatureInfo` assignment + XP in party/PC saves — **2026-07-29**
- [x] Persist area doors/placeables (incl. placeable `ItemList`) + object local vars — **2026-07-29**
- [ ] Round-trip tests: save in module A → load → transition → save in module B.
- [ ] Level-up GUI fidelity pass: attributes, skills, feats, Force powers vs `feat.2da` / class tables; keep autolevel as debug fallback only.
- [x] Implement `AddMultiClass` / Jedi class transition path used on Dantooine — **2026-07-29**

**Exit:** Save/load and level-up used successfully across at least Endar → Taris → Dantooine without state loss.

---

### Phase 5 — Minigames (progression-critical)

**Goal:** Quest-gated minigames completable.

| Minigame | Work |
|----------|------|
| **Pazaak** | Verify `PlayPazaak` / result globals against Taris (and later) quest scripts; fix GUI/rules gaps |
| **Swoop** | Obstacle/enemy object resolution wired; deepen physics/scoring vs Taris scripts + live modules |
| **Turret / Hawk combat** | SWMG named object getters + last-hit tracking wired; exercise vs encounter scripts + live coverage |

Track SWMG stub burn-down (**done for KotOR I tables**). Remaining work is fidelity + KotOR II stubs + live coverage.

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
- [x] Unknown World + Star Forge galaxy map destinations (`BTN_UNK` / `BTN_STA`, indices 6/7) — **2026-07-29**
- [ ] Unknown World modules + travel hooks under live data.
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
| 1 | Phase 1 Endar Spire live smoke | Checked `MILESTONE_A_SMOKE.md` + bugfix PRs |
| 2 | Phase 1 Taris → Dantooine smoke | Checked progression smoke + fixes |
| 3 | Phase 4 save round-trip on early path | Serialization fidelity PR |
| 4 | Phase 5 swoop/turret live fidelity | Minigame progression PR |
| 5 | Phase 3 remaining Force data-driven cleanup | Retire C++ Force fallbacks |
| 6 | Then Phases 6 → 7 in order | Planet/finale milestones |

*(Phase 0 doc honesty + Phase 2 K1 stub burn-down + Phase 3 flanking/LOS/Force wiring landed 2026-07-29; remaining work is live proof + depth.)*

---

## 8. Key source pointers

| Topic | Path |
|-------|------|
| Function table | `src/engines/kotor/script/function_tables.h` |
| Stub runtime | `src/engines/kotorbase/script/functions_stub.cpp` |
| Stub verifier | `scripts/verify_kotor_nwscript_stubs.py` |
| Galaxy map planets | `src/engines/kotor/gui/ingame/galaxymap.cpp` |
| Flanking / LOS | `src/engines/kotorbase/creature.cpp`, `area.cpp`, `actionexecutor.cpp` |
| GFF3 labels (16-char) | `src/aurora/gff3writer.cpp`, `src/aurora/gff3file.cpp` |
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
