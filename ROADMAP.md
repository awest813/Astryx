# KotOR Engine Parity Roadmap

> **Status note (2026-07-29):** Items below mark *systems scaffolding*, not verified
> full-campaign playability. Live Endar Spire → Star Forge smoke is still unchecked.
> For stub counts, subsystem gaps, and the phased plan, see
> **[docs/FULL_PARITY_AUDIT_AND_PLAN.md](docs/FULL_PARITY_AUDIT_AND_PLAN.md)**.
>
> KotOR I NWScript: **786/786** named handlers (0 generic / 0 SWMG stubs). Many
> handlers remain thin vs original semantics. KotOR II still has ~273 generic stubs.

## Core Engine Parity

Legend: ✅ = present as scaffolding / unit-covered · 🔶 = partial / thin · ☐ = unverified live

#### Combat System
- [x] Turn-based/real-time combat resolution (attack rolls, saves, feat/Force hooks) ✅
- [x] Flanking + closed-door LOS (not full cover geometry) 🔶
- [x] Combat animations driven by attack results ✅
- [x] Enemy AI (nearest-target attack; Force cast when FP allows) 🔶

#### Character System
- [x] Character creation (class, attributes, skills, feats, appearance) ✅
- [x] Level-up GUI path present; fidelity vs original tables still needs audit 🔶
- [x] Attribute and skill checks in dialogue / world interactions 🔶
- [x] Force alignment (Light/Dark) tracking 🔶

#### Dialogue & Quests
- [x] Branching dialogue with skill/alignment gates 🔶
- [x] Quest journal updates tied to script events 🔶
- [x] Cutscene playback integrated with dialogue 🔶
- [x] KotOR I NWScript table fully registered (named handlers; many thin) ✅

#### Inventory & Economy
- [x] Item use, equip, and drop logic 🔶
- [x] Container looting and inventory management 🔶
- [x] Merchant/Store GUI path present; pricing/modifiers thin 🔶
- [ ] Economic balance for credits and high-tier equipment ☐

#### Minigames & Extras
- [x] Pazaak engine + GUI shell 🔶
- [x] Ebon Hawk interior / travel hooks 🔶
- [x] Swoop racing engine hooks (K1 SWMG natives named) 🔶
- [x] Turret minigame transition / SWMG object getters 🔶

#### Audio / Video
- [x] Audio framework for music, VO, and SFX 🔶
- [x] In-engine video playback (Bink/Movie support) 🔶

## Status Summary
**Milestone 6:** Engineering systems for Dantooine polish are largely present;
**not** verified end-to-end with live game data.

**Milestone 7 — The Road to Malak (active):** Deepen thin natives, prove live smoke
Endar → Dantooine, then planetary hubs → Unknown World / Star Forge. Do **not** claim
full-campaign capability until smoke checklists pass.
