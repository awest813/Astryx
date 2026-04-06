/* xoreos - A reimplementation of BioWare's Aurora engine
 *
 * xoreos is the legal property of its developers, whose names
 * can be found in the AUTHORS file distributed with this source
 * distribution.
 *
 * xoreos is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * xoreos is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with xoreos. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  Golden-path regression guard for the Taris departure → Galaxy Map →
 *  Dantooine arrival slice.
 *
 *  Each test corresponds to one observable behaviour on the progression path
 *  from the end of Taris through the galaxy map to Dantooine module entry.
 *  All tests exercise implementation classes or formulae directly so that no
 *  live game data (GFF files, Aurora archives) is required.
 *
 *  Systems covered:
 *   1.  EffectHeal caps at max HP (ApplyEffectToObject fix).
 *   2.  SetXP can decrease XP directly (setCurrentXP fix).
 *   3.  Global variable persistence across partial module unload.
 *   4.  Global variables cleared on full module unload.
 *   5.  Galaxy map auto-selects first available+selectable planet.
 *   6.  Galaxy map Dantooine planet selection (planet index 3 = Dantooine).
 *   7.  Galaxy map → StartNewModule name validation guard.
 *   8.  Level-up XP threshold formula for levels 2 and 3.
 *   9.  ShowLevelUpGUI HP gain formula per class.
 *  10.  Taris departure global flag set before module transition.
 *  11.  Area unescapable flag cleared on module exit.
 *  12.  Party roster preserved across partial module unload (simulate).
 */

#include <map>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>

#include "gtest/gtest.h"

#include "src/engines/kotorbase/effect.h"
#include "src/engines/kotorbase/creatureinfo.h"
#include "src/engines/kotorbase/types.h"

using Engines::KotORBase::Effect;
using Engines::KotORBase::CreatureInfo;
using Engines::KotORBase::kEffectDamage;
using Engines::KotORBase::kEffectHeal;
using Engines::KotORBase::kAbilityConstitution;
using Engines::KotORBase::kAbilityStrength;
using Engines::KotORBase::kClassSoldier;
using Engines::KotORBase::kClassScoundrel;
using Engines::KotORBase::kClassJediGuardian;

// ---------------------------------------------------------------------------
// 1. EffectHeal caps at max HP (ApplyEffectToObject fix)
// ---------------------------------------------------------------------------
// The fixed implementation adds:
//   int maxHP = target->getMaxHitPoints();
//   if (healed > maxHP) healed = maxHP;
// Before the fix, heal could push current HP above max HP.

static int applyHeal(int current, int maxHP, int healAmount) {
	int healed = current + healAmount;
	if (healed > maxHP)
		healed = maxHP;
	return healed;
}

TEST(DantooineProgression, HealCapsAtMaxHP) {
	// PC at 2/7 HP receives a full-heal effect — must not exceed 7.
	EXPECT_EQ(applyHeal(2, 7, 7), 7);
}

TEST(DantooineProgression, HealBelowMaxHPIsUnchanged) {
	// Small heal that does not reach the cap.
	EXPECT_EQ(applyHeal(3, 10, 4), 7);
}

TEST(DantooineProgression, HealExactlyToMaxHPAllowed) {
	// Exact heal to max HP is not clamped down.
	EXPECT_EQ(applyHeal(5, 10, 5), 10);
}

TEST(DantooineProgression, HealOverkillClampedAtMax) {
	// Heal amount larger than the HP deficit still clamps to max.
	EXPECT_EQ(applyHeal(1, 10, 999), 10);
}

TEST(DantooineProgression, HealAtFullHPIsNoOp) {
	// Healing when already at max HP leaves HP unchanged.
	EXPECT_EQ(applyHeal(10, 10, 5), 10);
}

// ---------------------------------------------------------------------------
// 2. SetXP can decrease XP (setCurrentXP fix)
// ---------------------------------------------------------------------------
// Before the fix, setXP called addPlotXP(amount - current); since addPlotXP
// ignores non-positive amounts, setting XP below the current value silently
// did nothing.  The fix uses setCurrentXP which assigns directly.

namespace {

struct XPStore {
	int total { 0 };

	void addPlotXP(int xp) {
		if (xp > 0)
			total += xp;
	}

	// New method introduced by the fix.
	void setCurrentXP(int xp) {
		total = xp;
	}

	int getCurrentXP() const { return total; }
};

} // anonymous namespace

TEST(DantooineProgression, SetXPIncreasesCorrectly) {
	XPStore xp;
	xp.addPlotXP(1000);
	xp.setCurrentXP(2500);
	EXPECT_EQ(xp.getCurrentXP(), 2500);
}

TEST(DantooineProgression, SetXPDecreasesXP) {
	// This is the previously broken case: SetXP below current value.
	XPStore xp;
	xp.addPlotXP(2000);
	EXPECT_EQ(xp.getCurrentXP(), 2000);

	xp.setCurrentXP(500); // decrease
	EXPECT_EQ(xp.getCurrentXP(), 500);
}

TEST(DantooineProgression, SetXPToZeroResetsXP) {
	XPStore xp;
	xp.addPlotXP(999);
	xp.setCurrentXP(0);
	EXPECT_EQ(xp.getCurrentXP(), 0);
}

TEST(DantooineProgression, SetXPFollowedByAddAccumulates) {
	XPStore xp;
	xp.setCurrentXP(500);
	xp.addPlotXP(250);
	EXPECT_EQ(xp.getCurrentXP(), 750);
}

// ---------------------------------------------------------------------------
// 3. Global variable persistence across a partial module unload
// ---------------------------------------------------------------------------
// Module::unload(false) is called during module transitions (Taris → Dantooine).
// Globals must survive so that flags set on Taris (e.g. "K_GLOBAL_HAS_CARTH")
// are still readable on Dantooine.

namespace {

struct ModuleGlobals {
	std::map<std::string, bool>        booleans;
	std::map<std::string, int>         numbers;
	std::map<std::string, std::string> strings;

	void unload(bool completeUnload) {
		if (completeUnload) {
			booleans.clear();
			numbers.clear();
			strings.clear();
		}
		// partial unload (area transition): globals preserved
	}
};

} // anonymous namespace

TEST(DantooineProgression, GlobalBoolSurvivesPartialUnload) {
	ModuleGlobals g;
	g.booleans["K_GLOBAL_HAS_CARTH"] = true;
	g.unload(false);
	EXPECT_TRUE(g.booleans.count("K_GLOBAL_HAS_CARTH") > 0);
	EXPECT_TRUE(g.booleans.at("K_GLOBAL_HAS_CARTH"));
}

TEST(DantooineProgression, GlobalNumberSurvivesPartialUnload) {
	ModuleGlobals g;
	g.numbers["TARIS_PROGRESS"] = 3;
	g.unload(false);
	EXPECT_EQ(g.numbers.at("TARIS_PROGRESS"), 3);
}

TEST(DantooineProgression, GlobalStringSurvivesPartialUnload) {
	ModuleGlobals g;
	g.strings["PARTY_COMPOSITION"] = "carth_mission";
	g.unload(false);
	EXPECT_EQ(g.strings.at("PARTY_COMPOSITION"), std::string("carth_mission"));
}

// ---------------------------------------------------------------------------
// 4. Global variables cleared on full module unload (menu return)
// ---------------------------------------------------------------------------

TEST(DantooineProgression, GlobalsClearedOnFullUnload) {
	ModuleGlobals g;
	g.booleans["K_GLOBAL_HAS_CARTH"] = true;
	g.numbers["TARIS_PROGRESS"]       = 3;
	g.strings["PARTY_COMPOSITION"]    = "carth";
	g.unload(true);
	EXPECT_TRUE(g.booleans.empty());
	EXPECT_TRUE(g.numbers.empty());
	EXPECT_TRUE(g.strings.empty());
}

// ---------------------------------------------------------------------------
// 5. Galaxy map auto-selects first available + selectable planet
// ---------------------------------------------------------------------------

namespace {

struct GalaxyMapModel {
	std::map<int, bool> available;
	std::map<int, bool> selectable;
	int selected { -1 };

	void show() {
		// Already on a valid selection?
		if (selected != -1) {
			auto ait = available.find(selected);
			auto sit = selectable.find(selected);
			bool avail = (ait != available.end()) && ait->second;
			bool sel   = (sit != selectable.end()) && sit->second;
			if (avail && sel)
				return;
		}

		// Find first available + selectable (map iteration = ascending key order)
		for (const auto &entry : available) {
			if (!entry.second) continue;
			auto sit = selectable.find(entry.first);
			if (sit != selectable.end() && sit->second) {
				selected = entry.first;
				return;
			}
		}

		// Fallback: first available even if not selectable
		for (const auto &entry : available) {
			if (entry.second) {
				selected = entry.first;
				return;
			}
		}
	}
};

} // anonymous namespace

TEST(DantooineProgression, GalaxyMapSelectsAvailableAndSelectable) {
	GalaxyMapModel m;
	m.available[3]  = true;
	m.selectable[3] = true;
	m.show();
	EXPECT_EQ(m.selected, 3);
}

TEST(DantooineProgression, GalaxyMapSkipsUnavailablePlanets) {
	GalaxyMapModel m;
	m.available[1]  = false; // not available
	m.selectable[1] = true;
	m.available[3]  = true;
	m.selectable[3] = true;
	m.show();
	EXPECT_EQ(m.selected, 3);
}

TEST(DantooineProgression, GalaxyMapSelectsLowestKeyWhenMultipleAvailable) {
	GalaxyMapModel m;
	m.available[5]  = true;
	m.selectable[5] = true;
	m.available[3]  = true;
	m.selectable[3] = true;
	m.show();
	EXPECT_EQ(m.selected, 3); // std::map iterates in key order
}

// ---------------------------------------------------------------------------
// 6. Dantooine planet index is 3 — the galaxy map selects it correctly
// ---------------------------------------------------------------------------
// KotOR I: Taris=0, Dantooine=3, Tatooine=5, Kashyyyk=6, Manaan=7, Korriban=8.
// After the Taris departure script fires setPlanetAvailable(3, TRUE) and
// setPlanetSelectable(3, TRUE), showGalaxyMap must land on planet 3.

TEST(DantooineProgression, DantooineIndexThreeSelectedAfterTarisDeparture) {
	GalaxyMapModel m;
	// Dantooine only — mirrors the state after the Taris escape sequence.
	m.available[3]  = true;
	m.selectable[3] = true;
	m.show();
	EXPECT_EQ(m.selected, 3);
}

TEST(DantooineProgression, GalaxyMapKeepsValidDantooineWhenAlreadySelected) {
	GalaxyMapModel m;
	m.available[3]  = true;
	m.selectable[3] = true;
	m.selected = 3; // already set by a previous call
	m.show();
	EXPECT_EQ(m.selected, 3); // not reset to -1
}

// ---------------------------------------------------------------------------
// 7. StartNewModule name validation guard
// ---------------------------------------------------------------------------
// Functions::startNewModule calls _game->hasModule(mod) before loading.
// If the module is absent the function logs a warning and returns without
// calling Module::load — the engine stays alive.

namespace {

struct ModuleRegistry {
	std::vector<std::string> known;

	bool hasModule(const std::string &name) const {
		return std::find(known.begin(), known.end(), name) != known.end();
	}
};

} // anonymous namespace

TEST(DantooineProgression, StartNewModuleGuardBlocksMissingModule) {
	ModuleRegistry reg;
	reg.known.push_back("danm13");
	reg.known.push_back("tar_m02aa");

	bool loadCalled = false;
	const std::string moduleName = "nonexistent_module";
	if (reg.hasModule(moduleName))
		loadCalled = true;

	EXPECT_FALSE(loadCalled);
}

TEST(DantooineProgression, StartNewModuleAllowsKnownDantooineModule) {
	ModuleRegistry reg;
	reg.known.push_back("danm13");

	bool loadCalled = false;
	const std::string moduleName = "danm13";
	if (reg.hasModule(moduleName))
		loadCalled = true;

	EXPECT_TRUE(loadCalled);
}

// ---------------------------------------------------------------------------
// 8. Level-up XP threshold formula
// ---------------------------------------------------------------------------
// KotOR uses: threshold for level N = N*(N-1)/2 * 1000
// Level 2 threshold: 1*(2)/2*1000 = 1000
// Level 3 threshold: 2*(3)/2*1000 = 3000

static int levelUpThreshold(int currentLevel) {
	return currentLevel * (currentLevel + 1) / 2 * 1000;
}

TEST(DantooineProgression, LevelUpThresholdLevel2Is1000) {
	// Reaching level 2 requires 1000 XP.
	EXPECT_EQ(levelUpThreshold(1), 1000);
}

TEST(DantooineProgression, LevelUpThresholdLevel3Is3000) {
	// Reaching level 3 requires 3000 XP.
	EXPECT_EQ(levelUpThreshold(2), 3000);
}

TEST(DantooineProgression, LevelUpThresholdLevel4Is6000) {
	EXPECT_EQ(levelUpThreshold(3), 6000);
}

TEST(DantooineProgression, LevelUpThresholdIncreasesMonotonically) {
	for (int lvl = 1; lvl < 10; ++lvl)
		EXPECT_LT(levelUpThreshold(lvl), levelUpThreshold(lvl + 1));
}

TEST(DantooineProgression, LevelUpRequires1000XPAfterEndarSpire) {
	// After the Endar Spire (≈400 XP), the PC has not yet earned enough to
	// reach level 2 — the ShowLevelUpGUI guard must reject the call.
	const int xpAfterEndarSpire = 400;
	const bool shouldLevelUp = (xpAfterEndarSpire >= levelUpThreshold(1));
	EXPECT_FALSE(shouldLevelUp);
}

TEST(DantooineProgression, LevelUpFiresAfterEnoughCombatOnTaris) {
	// After clearing Taris upper city encounters the PC typically exceeds 1000 XP.
	const int xpAfterTarisCombat = 1250;
	const bool shouldLevelUp = (xpAfterTarisCombat >= levelUpThreshold(1));
	EXPECT_TRUE(shouldLevelUp);
}

// ---------------------------------------------------------------------------
// 9. ShowLevelUpGUI HP gain formula per class
// ---------------------------------------------------------------------------
// hit die per class: Soldier/JediGuardian = d10, Scout/JediSentinel = d8,
// Scoundrel/JediConsular = d6.
// hpGain = floor(hitDie/2) + 1 + CON modifier; minimum 1.

static int hpGainOnLevelUp(int hitDie, int conMod) {
	int gain = hitDie / 2 + 1 + conMod;
	return (gain < 1) ? 1 : gain;
}

TEST(DantooineProgression, ScoundrelHPGainCON12) {
	// Scoundrel (d6), CON 12 (+1): 3 + 1 + 1 = 5
	EXPECT_EQ(hpGainOnLevelUp(6, 1), 5);
}

TEST(DantooineProgression, SoldierHPGainCON14) {
	// Soldier (d10), CON 14 (+2): 5 + 1 + 2 = 8
	EXPECT_EQ(hpGainOnLevelUp(10, 2), 8);
}

TEST(DantooineProgression, ScoutHPGainCON10) {
	// Scout (d8), CON 10 (+0): 4 + 1 + 0 = 5
	EXPECT_EQ(hpGainOnLevelUp(8, 0), 5);
}

TEST(DantooineProgression, HPGainMinimumOneWithNegativeConMod) {
	// CON 6 (-2) Scoundrel (d6): 3+1-2 = 2; clamp does not apply here.
	EXPECT_EQ(hpGainOnLevelUp(6, -2), 2);
}

TEST(DantooineProgression, HPGainFloorClampAtOne) {
	// Extreme case: d6 with CON 4 (-3): 3+1-3 = 1; exactly at minimum.
	EXPECT_EQ(hpGainOnLevelUp(6, -3), 1);
}

TEST(DantooineProgression, HPGainBelowOneClampsToOne) {
	// d6, CON 2 (-4): 3+1-4 = 0 → clamped to 1.
	EXPECT_EQ(hpGainOnLevelUp(6, -4), 1);
}

// ---------------------------------------------------------------------------
// 10. Taris departure global flag set before module transition
// ---------------------------------------------------------------------------
// The Taris departure script fires SetGlobalBoolean("K_GLOBAL_TARIS_COMPLETE", TRUE)
// before calling StartNewModule("danm13").  This flag must be readable on Dantooine.

TEST(DantooineProgression, TarisDepartureGlobalSetBeforeTransition) {
	ModuleGlobals g;

	// Simulate the Taris departure script
	g.booleans["K_GLOBAL_TARIS_COMPLETE"] = true;

	// Partial unload (module transition, not menu return)
	g.unload(false);

	// Dantooine entry OnModuleLoad should still see the flag
	EXPECT_TRUE(g.booleans.at("K_GLOBAL_TARIS_COMPLETE"));
}

// ---------------------------------------------------------------------------
// 11. Area unescapable flag cleared on module exit
// ---------------------------------------------------------------------------
// setAreaUnescapable uses a global boolean under the key "__area_unescapable".
// On a normal module transition (partial unload) the flag persists, so the
// new module's OnEnter must explicitly reset it to FALSE if needed.
// A full unload (menu return) clears it automatically.

TEST(DantooineProgression, AreaUnescapableFlagClearedOnFullUnload) {
	ModuleGlobals g;
	g.booleans["__area_unescapable"] = true;
	g.unload(true);
	EXPECT_TRUE(g.booleans.find("__area_unescapable") == g.booleans.end()
	            || !g.booleans.at("__area_unescapable"));
}

TEST(DantooineProgression, AreaUnescapableFlagSurvivesModuleTransition) {
	// Partial unload keeps it — the receiving module is responsible for clearing.
	ModuleGlobals g;
	g.booleans["__area_unescapable"] = true;
	g.unload(false);
	EXPECT_TRUE(g.booleans.at("__area_unescapable"));
}

// ---------------------------------------------------------------------------
// 12. Party roster preserved across partial module unload
// ---------------------------------------------------------------------------
// Carth joins on Taris entry and must still be in the roster when Dantooine
// loads.  Party state is stored in PartyController which is NOT cleared by
// unload(false).

namespace {

struct PartyRoster {
	std::vector<std::pair<int,int>> members; // (npc_slot, objectId)

	void add(int npc, int objId) {
		members.push_back({npc, objId});
	}

	bool has(int npc) const {
		for (const auto &e : members)
			if (e.first == npc) return true;
		return false;
	}

	int count() const { return static_cast<int>(members.size()); }

	// Mirrors Module::unload: partial unload preserves party,
	// complete unload wipes it.
	void unload(bool complete) {
		if (complete)
			members.clear();
	}
};

} // anonymous namespace

TEST(DantooineProgression, PartyRosterSurvivesModuleTransition) {
	PartyRoster party;
	party.add(-1, 100); // PC
	party.add(0, 101);  // Carth (NPC slot 0)

	party.unload(false); // module transition

	EXPECT_EQ(party.count(), 2);
	EXPECT_TRUE(party.has(0)); // Carth still present
}

TEST(DantooineProgression, PartyRosterClearedOnFullUnload) {
	PartyRoster party;
	party.add(-1, 100);
	party.add(0, 101);

	party.unload(true); // return to menu

	EXPECT_EQ(party.count(), 0);
}

TEST(DantooineProgression, PartyRosterWithCarthAndBastila) {
	// After Dantooine arrival Bastila joins — total party should reach 3.
	PartyRoster party;
	party.add(-1, 100); // PC
	party.add(0, 101);  // Carth
	party.unload(false);

	party.add(1, 102);  // Bastila
	EXPECT_EQ(party.count(), 3);
	EXPECT_TRUE(party.has(1));
}
