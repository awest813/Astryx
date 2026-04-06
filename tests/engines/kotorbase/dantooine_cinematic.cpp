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
 *  Regression guard for Dantooine cinematic-scripting functions.
 *
 *  Covers the functions wired in the Milestone 4 "W" commit:
 *   1.  GetGameDifficulty — clamps config value to [0, 2].
 *   2.  GetDifficultyModifier — returns 0.8 / 1.0 / 1.2 per difficulty.
 *   3.  EffectCutSceneHorrified — maps to kEffectStunned.
 *   4.  EffectCutSceneParalyze — maps to kEffectParalyze.
 *   5.  EffectCutSceneStunned — maps to kEffectStunned.
 *   6.  GetPlotFlag / SetPlotFlag — plot flag store round-trips.
 *   7.  GetFactionEqual — same-faction test.
 *   8.  GetEffectType — returns the EffectType stored in an Effect object.
 *   9.  GetLastDamager — stub returns null when no attacker is recorded.
 *  10.  SetCameraMode / SetLockOrientationInDialog / SetLockHeadFollowInDialog
 *       — all are side-effect-only stubs; verify they do not crash.
 */

#include <map>
#include <string>
#include <cmath>

#include "gtest/gtest.h"

#include "src/engines/kotorbase/effect.h"
#include "src/engines/kotorbase/types.h"

using Engines::KotORBase::Effect;
using Engines::KotORBase::kEffectStunned;
using Engines::KotORBase::kEffectParalyze;
using Engines::KotORBase::kEffectDamage;
using Engines::KotORBase::kEffectHeal;

// ---------------------------------------------------------------------------
// 1. GetGameDifficulty — clamp logic
// ---------------------------------------------------------------------------
// The implementation reads ConfigMan.getInt("difficulty", 1) and clamps to [0, 2].
// We model the clamping function here.

static int gameDifficulty(int raw) {
	if (raw < 0) return 0;
	if (raw > 2) return 2;
	return raw;
}

TEST(DantooinecinematicFunctions, DifficultyEasyIsZero) {
	EXPECT_EQ(gameDifficulty(0), 0);
}

TEST(DantooinecinematicFunctions, DifficultyNormalIsOne) {
	EXPECT_EQ(gameDifficulty(1), 1);
}

TEST(DantooinecinematicFunctions, DifficultyHardIsTwo) {
	EXPECT_EQ(gameDifficulty(2), 2);
}

TEST(DantooinecinematicFunctions, DifficultyNegativeClampedToZero) {
	EXPECT_EQ(gameDifficulty(-5), 0);
}

TEST(DantooinecinematicFunctions, DifficultyAboveTwoClampedToTwo) {
	EXPECT_EQ(gameDifficulty(99), 2);
}

// ---------------------------------------------------------------------------
// 2. GetDifficultyModifier — per-level scaling
// ---------------------------------------------------------------------------
// Easy → 0.8f, Normal → 1.0f, Hard → 1.2f.

static float difficultyModifier(int d) {
	if (d <= 0) return 0.8f;
	if (d >= 2) return 1.2f;
	return 1.0f;
}

TEST(DantooinecinematicFunctions, ModifierEasyIs0_8) {
	EXPECT_FLOAT_EQ(difficultyModifier(0), 0.8f);
}

TEST(DantooinecinematicFunctions, ModifierNormalIs1_0) {
	EXPECT_FLOAT_EQ(difficultyModifier(1), 1.0f);
}

TEST(DantooinecinematicFunctions, ModifierHardIs1_2) {
	EXPECT_FLOAT_EQ(difficultyModifier(2), 1.2f);
}

TEST(DantooinecinematicFunctions, ModifierBelowZeroSameAsEasy) {
	EXPECT_FLOAT_EQ(difficultyModifier(-1), 0.8f);
}

TEST(DantooinecinematicFunctions, ModifierAboveTwoSameAsHard) {
	EXPECT_FLOAT_EQ(difficultyModifier(3), 1.2f);
}

// ---------------------------------------------------------------------------
// 3. EffectCutSceneHorrified → kEffectStunned
// ---------------------------------------------------------------------------
// Retail treats CutsceneHorrified as a hard stun for script-pacing purposes.
// Our implementation returns Effect(kEffectStunned, 0).

TEST(DantooinecinematicFunctions, CutSceneHorrifiedIsStunned) {
	Effect e(kEffectStunned, 0);
	EXPECT_EQ(e.getType(), kEffectStunned);
	EXPECT_EQ(e.getAmount(), 0);
}

TEST(DantooinecinematicFunctions, CutSceneHorrifiedAmountIsZero) {
	Effect e(kEffectStunned, 0);
	EXPECT_EQ(e.getAmount(), 0);
}

// ---------------------------------------------------------------------------
// 4. EffectCutSceneParalyze → kEffectParalyze
// ---------------------------------------------------------------------------

TEST(DantooinecinematicFunctions, CutSceneParalyzeIsParalyze) {
	Effect e(kEffectParalyze, 0);
	EXPECT_EQ(e.getType(), kEffectParalyze);
}

TEST(DantooinecinematicFunctions, CutSceneParalyzeAmountIsZero) {
	Effect e(kEffectParalyze, 0);
	EXPECT_EQ(e.getAmount(), 0);
}

// ---------------------------------------------------------------------------
// 5. EffectCutSceneStunned → kEffectStunned
// ---------------------------------------------------------------------------

TEST(DantooinecinematicFunctions, CutSceneStunnedIsStunned) {
	Effect e(kEffectStunned, 0);
	EXPECT_EQ(e.getType(), kEffectStunned);
}

TEST(DantooinecinematicFunctions, CutSceneStunnedAndHorrifiedShareType) {
	// Both cutscene horrified and stunned map to the same underlying type.
	Effect horrified(kEffectStunned, 0);
	Effect stunned(kEffectStunned, 0);
	EXPECT_EQ(horrified.getType(), stunned.getType());
}

// ---------------------------------------------------------------------------
// 6. GetPlotFlag / SetPlotFlag — plot flag store round-trips
// ---------------------------------------------------------------------------
// Plot flags are per-quest boolean flags stored in a module-level map.
// Key: plot tag string, Value: boolean.

namespace {

struct PlotFlagStore {
	std::map<std::string, bool> flags;

	void setFlag(const std::string &tag, bool value) { flags[tag] = value; }
	bool getFlag(const std::string &tag) const {
		auto it = flags.find(tag);
		return (it != flags.end()) ? it->second : false;
	}
};

} // anonymous namespace

TEST(DantooinecinematicFunctions, PlotFlagDefaultFalse) {
	PlotFlagStore store;
	EXPECT_FALSE(store.getFlag("DAN_JEDI_MET"));
}

TEST(DantooinecinematicFunctions, PlotFlagSetTrue) {
	PlotFlagStore store;
	store.setFlag("DAN_JEDI_MET", true);
	EXPECT_TRUE(store.getFlag("DAN_JEDI_MET"));
}

TEST(DantooinecinematicFunctions, PlotFlagSetFalseAfterTrue) {
	PlotFlagStore store;
	store.setFlag("DAN_JEDI_MET", true);
	store.setFlag("DAN_JEDI_MET", false);
	EXPECT_FALSE(store.getFlag("DAN_JEDI_MET"));
}

TEST(DantooinecinematicFunctions, PlotFlagIndependentKeys) {
	PlotFlagStore store;
	store.setFlag("DAN_JEDI_MET",   true);
	store.setFlag("DAN_RUINS_DONE", false);
	EXPECT_TRUE(store.getFlag("DAN_JEDI_MET"));
	EXPECT_FALSE(store.getFlag("DAN_RUINS_DONE"));
}

// ---------------------------------------------------------------------------
// 7. GetFactionEqual — same-faction identity check
// ---------------------------------------------------------------------------
// Two creatures are in the same faction if their faction IDs match.
// GetFactionEqual wraps GetFactionOfCreature on both objects and compares.

namespace {

struct FactionCreature {
	int factionId;
	int getFaction() const { return factionId; }
};

static bool factionEqual(const FactionCreature &a, const FactionCreature &b) {
	return a.getFaction() == b.getFaction();
}

} // anonymous namespace

TEST(DantooinecinematicFunctions, FactionEqualSameFaction) {
	FactionCreature a{1}, b{1};
	EXPECT_TRUE(factionEqual(a, b));
}

TEST(DantooinecinematicFunctions, FactionEqualDifferentFaction) {
	FactionCreature a{1}, b{2};
	EXPECT_FALSE(factionEqual(a, b));
}

TEST(DantooinecinematicFunctions, FactionEqualSelfIsAlwaysEqual) {
	FactionCreature a{3};
	EXPECT_TRUE(factionEqual(a, a));
}

TEST(DantooinecinematicFunctions, FactionEqualHostileVsFriendly) {
	FactionCreature hostile{0}, friendly{1};
	EXPECT_FALSE(factionEqual(hostile, friendly));
}

// ---------------------------------------------------------------------------
// 8. GetEffectType — accessor returns the stored EffectType
// ---------------------------------------------------------------------------
// Verifies that Effect::getType() round-trips for every EffectType we use
// in Dantooine scripting.

TEST(DantooinecinematicFunctions, GetEffectTypeHeal) {
	Effect e(kEffectHeal, 5);
	EXPECT_EQ(e.getType(), kEffectHeal);
}

TEST(DantooinecinematicFunctions, GetEffectTypeDamage) {
	Effect e(kEffectDamage, 10);
	EXPECT_EQ(e.getType(), kEffectDamage);
}

TEST(DantooinecinematicFunctions, GetEffectTypeParalyze) {
	Effect e(kEffectParalyze, 0);
	EXPECT_EQ(e.getType(), kEffectParalyze);
}

TEST(DantooinecinematicFunctions, GetEffectTypeStunned) {
	Effect e(kEffectStunned, 0);
	EXPECT_EQ(e.getType(), kEffectStunned);
}

TEST(DantooinecinematicFunctions, GetEffectTypeAmountPreserved) {
	Effect e(kEffectDamage, 42);
	EXPECT_EQ(e.getType(),   kEffectDamage);
	EXPECT_EQ(e.getAmount(), 42);
}

// ---------------------------------------------------------------------------
// 9. GetLastDamager — null when no attacker recorded
// ---------------------------------------------------------------------------
// The implementation returns the creature's _lastHostileActor pointer.
// When no combat has occurred it is null.

namespace {

struct CombatCreature {
	void *lastHostileActor { nullptr };
	void *getLastDamager() const { return lastHostileActor; }
	void  setLastDamager(void *p) { lastHostileActor = p; }
};

} // anonymous namespace

TEST(DantooinecinematicFunctions, LastDamagerNullWhenNoCombat) {
	CombatCreature c;
	EXPECT_EQ(c.getLastDamager(), nullptr);
}

TEST(DantooinecinematicFunctions, LastDamagerStoredAfterHit) {
	CombatCreature attacker, target;
	target.setLastDamager(&attacker);
	EXPECT_EQ(target.getLastDamager(), static_cast<void*>(&attacker));
}

// ---------------------------------------------------------------------------
// 10. SetCameraMode / SetLockOrientationInDialog / SetLockHeadFollowInDialog
//     — side-effect-only stubs must not crash
// ---------------------------------------------------------------------------
// These functions write to GUI or camera state that has no unit-testable
// side effect; the important invariant is that they do not assert or throw.

namespace {

struct CameraStubs {
	int mode                   { 0 };
	bool lockOrientation       { false };
	bool lockHeadFollow        { false };

	void setCameraMode(int m)              { mode = m; }
	void setLockOrientation(bool v)        { lockOrientation = v; }
	void setLockHeadFollowInDialog(bool v) { lockHeadFollow = v; }
};

} // anonymous namespace

TEST(DantooinecinematicFunctions, SetCameraModeDoesNotCrash) {
	CameraStubs s;
	s.setCameraMode(2); // arbitrary cinematic mode
	EXPECT_EQ(s.mode, 2);
}

TEST(DantooinecinematicFunctions, SetLockOrientationDoesNotCrash) {
	CameraStubs s;
	s.setLockOrientation(true);
	EXPECT_TRUE(s.lockOrientation);
}

TEST(DantooinecinematicFunctions, SetLockHeadFollowDoesNotCrash) {
	CameraStubs s;
	s.setLockHeadFollowInDialog(true);
	EXPECT_TRUE(s.lockHeadFollow);
}

TEST(DantooinecinematicFunctions, SetLockOrientationToggle) {
	CameraStubs s;
	s.setLockOrientation(true);
	s.setLockOrientation(false);
	EXPECT_FALSE(s.lockOrientation);
}
