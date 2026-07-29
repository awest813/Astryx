# Astryx

A high-fidelity fork of [xoreos](https://github.com/xoreos/xoreos) focused on
**Knights of the Old Republic I** campaign parity (Endar Spire → Star Forge),
with shared KotOR II engine foundations.

<p align="center">
  <img src="https://img.shields.io/badge/Status-Milestone%207%20In%20Progress-yellow?style=for-the-badge" alt="Status">
  <img src="https://img.shields.io/badge/NWScript-~72%25%20behavioral-blue?style=for-the-badge" alt="Coverage">
  <img src="https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-orange?style=for-the-badge" alt="Platforms">
</p>

## Honest status

Astryx has substantial early-campaign systems (chargen, d20 combat core, party,
dialog/journal, partial save, Dantooine-oriented cinematics). It is **not** yet
at verified full-campaign parity.

| Metric (KotOR I) | Value |
|------------------|------:|
| NWScript bindings | ~785 |
| Behavioral handlers | ~563 |
| Generic stubs | ~144 |
| SWMG stubs | ~78 |

“100% NWScript coverage” in older docs meant **every slot is registered** (no
null pointers), not that every native has full original behavior. Stubs return
safe defaults and can softlock quests silently.

**Working plan:** [docs/FULL_PARITY_AUDIT_AND_PLAN.md](docs/FULL_PARITY_AUDIT_AND_PLAN.md)

**Supported slice (conservative):** Endar Spire, then Taris entry — see
[docs/SUPPORTED_CONTENT.md](docs/SUPPORTED_CONTENT.md).

### Milestone snapshot

- **M1–M4 systems:** Implemented with unit coverage; live smoke checklists still open
- **M6 systems:** Force/spells.2da / cinematic polish present; not end-to-end proven
- **M7 (active):** Planetary hubs → Star Forge; de-stub campaign blockers; save/load fidelity; minigames

### In progress (M7)

- [x] Phase 2 P0 combat/quest/spell/immunity natives (started)
- [ ] Verified Endar Spire → Dantooine live smoke
- [ ] SWMG / swoop / turret progression fidelity
- [ ] GFF save/load campaign round-trips
- [ ] Unknown World & Star Forge

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
