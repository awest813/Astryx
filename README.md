# Astryx

A high-fidelity fork of [xoreos](https://github.com/xoreos/xoreos) focused on
**Knights of the Old Republic I** campaign parity (Endar Spire → Star Forge),
with shared KotOR II engine foundations.

<p align="center">
  <img src="https://img.shields.io/badge/Status-Milestone%207%20In%20Progress-yellow?style=for-the-badge" alt="Status">
  <img src="https://img.shields.io/badge/NWScript-100%25%20registered%20(K1)-blue?style=for-the-badge" alt="Coverage">
  <img src="https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-orange?style=for-the-badge" alt="Platforms">
</p>

## Honest status

Astryx has substantial early-campaign systems (chargen, d20 combat core, party,
dialog/journal, partial save, Dantooine-oriented cinematics). It is **not** yet
at verified full-campaign parity.

| Metric (KotOR I) | Value |
|------------------|------:|
| NWScript bindings | 786 |
| Named handlers | 786 (0 `stubFunction` / 0 `stubSWMGFunction`) |
| Thin / simplified semantics | Many (esp. late SWMG, audio, formation) |

“100% registered” means every KotOR I native points at a named handler (no silent
generic/SWMG stubs). It does **not** mean full original fidelity or a verified
Endar→Star Forge playthrough. Live smoke still needs game data. Supported slice
(Endar → Taris entry) is narrower than the M6/M7 engineering track.

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
- [x] GetWasForcePowerSuccessful, effect iterators/clear, poison/debilitate queries
- [x] Door/placeable visual restore on load; SWMG named object getters
- [x] Audit polish: K1 NWScript signature table repair; Force LOS/FP order; effect instant duration
- [x] Retail K1 IDs restored (`GetSkillRank` 315, `GetLastAttackType` 317, `GetIsInCombat` 320, `SetLocked` 324); creature GFF load/combat statements un-swallowed
- [x] Playable-build audit: crash/softlock/save fixes across natives, module flow, combat, GUI (see [docs/PLAYTEST_BUILD.md](docs/PLAYTEST_BUILD.md))
- [ ] Verified Endar Spire → Dantooine live smoke
- [ ] SWMG / swoop / turret live progression fidelity
- [ ] Full GFF save/load campaign round-trips
- [ ] Unknown World & Star Forge live modules

## Quick start

You need your own copy of KotOR; no game data is included. Prerequisite
install commands for each OS, and details on both build paths, are in
[BUILDDEP.md](BUILDDEP.md).

### Linux (system packages, fastest)
```bash
scripts/build_ci_linux.sh --install-deps --test   # apt/dnf/pacman; drop --install-deps if already installed
```

### Linux / macOS (self-contained vcpkg build; slow first run)
```bash
scripts/bootstrap_linux.sh --test
```

### Windows 10/11 (PowerShell, needs Git, CMake and Visual Studio C++ tools)
```powershell
powershell -ExecutionPolicy Bypass -File scripts\bootstrap_windows.ps1 -Test
```

Each script prints the binary location and the exact launch command when it
finishes. Re-run the unit tests at any time with
`cmake --build <build-dir> --target check`. If CMake stops because a library is
missing, its message names the package to install.

## Playing

```bash
python3 scripts/xoreos_launcher_gui.py     # Windows: py -3 scripts\xoreos_launcher_gui.py
```

The launcher finds your Steam or GOG copy of KotOR, checks that the install is
complete, remembers it, and starts the game. If the game stops with an error,
the launcher shows the message. Without a window, use
`scripts/run_xoreos_linux.sh` or `scripts\run_xoreos_windows.ps1`, or run
`xoreos -p <game folder>` directly. See
[docs/PLAYTEST_BUILD.md](docs/PLAYTEST_BUILD.md) for options and common errors.

## Links

- [Detailed Roadmap](ROADMAP.md)
- [Acceptance Criteria](MILESTONE.md)
- [Full Parity Audit & Plan](docs/FULL_PARITY_AUDIT_AND_PLAN.md)
- [Playtest Build Guide](docs/PLAYTEST_BUILD.md)
- [Upstream xoreos](https://github.com/xoreos/xoreos)
