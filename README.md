# Astryx

A high-fidelity fork of [xoreos](https://github.com/xoreos/xoreos) focused on
**Knights of the Old Republic I** campaign parity (Endar Spire → Star Forge),
with shared KotOR II engine foundations.

<p align="center">
  <img src="https://img.shields.io/badge/Status-Milestone%207%20In%20Progress-yellow?style=for-the-badge" alt="Status">
  <img src="https://img.shields.io/badge/NWScript-100%25%20wired%20(K1)-blue?style=for-the-badge" alt="Coverage">
  <img src="https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-orange?style=for-the-badge" alt="Platforms">
</p>

## Honest status

Astryx has substantial early-campaign systems (chargen, d20 combat core, party,
dialog/journal, partial save, Dantooine-oriented cinematics). It is **not** yet
at verified full-campaign parity.

| Metric (KotOR I) | Value |
|------------------|------:|
| NWScript bindings | 785 |
| Behavioral handlers | 785 (0 `stubFunction` / 0 `stubSWMGFunction`) |
| Thin / simplified semantics | Many (esp. late SWMG, audio, formation) |

“100% wired” means every KotOR I native points at a named handler (no silent
generic/SWMG stubs). It does **not** mean full original fidelity or a verified
Endar→Star Forge playthrough. Live smoke still needs game data.

**Working plan:** [docs/FULL_PARITY_AUDIT_AND_PLAN.md](docs/FULL_PARITY_AUDIT_AND_PLAN.md)

**Supported slice (conservative):** Endar Spire, then Taris entry — see
[docs/SUPPORTED_CONTENT.md](docs/SUPPORTED_CONTENT.md).

### Milestone snapshot

- **M1–M4 systems:** Implemented with unit coverage; live smoke checklists still open
- **M6 systems:** Force/spells.2da / cinematic polish present; not end-to-end proven
- **M7 (active):** Planetary hubs → Star Forge; deepen thin natives; save/load fidelity; minigames

### In progress (M7)

- [x] Phase 2 P0 combat/quest/spell/immunity natives
- [x] Phase 2 P1 effects / AoE iterators / party / day-night / RevealMap
- [x] Phase 2 P2 faction aggregates / listening API
- [x] Phase 2 P3 remaining generic stubs + SWMG table burn-down (K1)
- [x] Phase 3 flanking + Force-user AI + AddMultiClass
- [x] Phase 3 LOS/cover + spells.2da impact fallback
- [x] Galaxy map Unknown World / Star Forge destinations
- [x] Galaxy map planet flags in save/load
- [x] GFF3 16-char label truncation round-trip (journal / return dest)
- [x] Force impact-script-first + spell target/caster/level context natives
- [x] CreatureInfo force-power assignment + XP persistence
- [x] Area doors/placeables marked persistent; placeable inventory + object locals in saveState
- [x] ResistForce + Effect spellId + Force resistance tracking
- [ ] Verified Endar Spire → Dantooine live smoke
- [ ] SWMG / swoop / turret live progression fidelity
- [ ] Full GFF save/load campaign round-trips
- [ ] Unknown World & Star Forge live modules

## Quick start

### Windows 10/11 (PowerShell)
```powershell
powershell -ExecutionPolicy Bypass -File scripts/bootstrap_windows.ps1
powershell -ExecutionPolicy Bypass -File scripts/run_xoreos_windows.ps1
```

### Linux (Bash)
```bash
./scripts/bootstrap_linux.sh
./scripts/run_xoreos_linux.sh
```

## Links

- [Detailed Roadmap](ROADMAP.md)
- [Acceptance Criteria](MILESTONE.md)
- [Full Parity Audit & Plan](docs/FULL_PARITY_AUDIT_AND_PLAN.md)
- [Upstream xoreos](https://github.com/xoreos/xoreos)
