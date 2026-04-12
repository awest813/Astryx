# <p align="center">✨ Astryx ✨</p>
<p align="center"><b>The Ultimate Star Wars: Knights of the Old Republic Engine Reimplementation</b></p>

<p align="center">
  <img src="https://img.shields.io/badge/Status-Milestone%207%20Active-brightgreen?style=for-the-badge&logo=starwars" alt="Status">
  <img src="https://img.shields.io/badge/Coverage-100%25%20NWScript%20API-blue?style=for-the-badge" alt="Coverage">
  <img src="https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-orange?style=for-the-badge" alt="Platforms">
</p>

---

## 🚀 The Mission
**Astryx** is a high-fidelity fork of [xoreos](https://github.com/xoreos/xoreos), meticulously engineered to bring **Knights of the Old Republic I & II** to modern hardware with 1:1 gameplay parity. While maintaining the portable foundation of xoreos, Astryx focuses on the advanced mechanical, cinematic, and AI fidelity required to complete the full journey from the *Endar Spire* to the *Star Forge*.

### 🌌 Key Feature Highlights
- **100% NWScript Coverage**: All ~850 engine functions are implemented or safely registered as architectural stubs, ensuring zero script-breakage across the entire campaign.
- **Cinematic Choreography**: Full support for `CutsceneAttack`, `CutsceneMove`, and smoothstep camera paths for professional-grade narrative delivery.
- **Advanced Combat Resolution**: Data-driven Force power scaling (`spells.2da`), alignment-based resource discounts, and procedural combat reactions.
- **Mini-Game Fidelity**: High-performance engine hooks for Swoop Racing and Space Combat (Turret) minigames.
- **Modern Infrastructure**: Built with C++17, featuring FMOD audio integration and high-performance Bink/Movie playback support.

---

## 🛠 Project Status: Milestone 7 — "The Road to Malak" 🚀

Astryx has surpassed the "playable slice" phase and is now in active development for **Total Campaign Parity**.

### ✅ Completed Milestones
- **[x] Milestone 1-3**: Full Taris Upper & Lower City questlines.
- **[x] Milestone 4-5**: Swoop Racing, Brejik Showdown, and Ebon Hawk escape.
- **[x] Milestone 6**: Dantooine Jedi Trials, Force Power scaling, and Star Map Reveal.

### 🏗 In Progress: Milestone 7
Focusing on the convergence of the planetary hubs and the final assault on the Star Forge.
- [ ] GFF-based Save/Load Persistence.
- [ ] Manual Level-Up GUI (Feat/Attribute selection).
- [ ] Unknown World & Star Forge Ritual Logic.

---

## 📥 Quick Start

### Windows 10/11 (PowerShell)
```powershell
# Bootstrap dependencies and launch
powershell -ExecutionPolicy Bypass -File scripts/bootstrap_windows.ps1
powershell -ExecutionPolicy Bypass -File scripts/run_xoreos_windows.ps1
```

### Linux (Bash)
```bash
./scripts/bootstrap_linux.sh
./scripts/run_xoreos_linux.sh
```

---

## 📋 Technical Overview & Parity
Astryx treats the KotOR game data as the source of truth. All logic is derived from:
- **`spells.2da` & `feat.2da`**: For mechanical resource resolution.
- **NWScript VM**: For world logic and event triggers.
- **Aurora Engine Formats**: Full support for `.dlg`, `.utc`, `.utp`, and `.git` serialization.

### 🔬 Combat Fidelity
The engine utilizes a hybrid real-time/turn-based controller that accurately models the d20 system, including:
- Critical threat ranges and multipliers.
- Compound saving throws (Fort/Ref/Will) with natural-1/20 rules.
- Lightsaber deflection vs. ranged fire with opposed rolls.

---

## 🤝 Contributing
We welcome contributions that push the boundaries of RPG engine fidelity. Please see [CONTRIBUTING.md](CONTRIBUTING.md) for architectural guidelines and coding standards.

## 🔗 Links
- **[Detailed Roadmap](ROADMAP.md)**
- **[Acceptance Criteria Checklist](MILESTONE.md)**
- **[Upstream xoreos](https://github.com/xoreos/xoreos)**

---
<p align="center"><i>"The Force is what gives a Jedi his power. It's an energy field created by all living things."</i></p>
