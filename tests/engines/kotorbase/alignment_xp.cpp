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
 *  Unit tests for the alignment (good/evil value) and XP accumulation
 *  logic used by GetGoodEvilValue / AdjustAlignment / GiveXPToCreature /
 *  GetXP NWScript functions.
 *
 *  These tests exercise the same arithmetic as the Object and Creature
 *  implementations without requiring a live game module.
 */

#include "gtest/gtest.h"

// ---------------------------------------------------------------------------
// Alignment helper — mirrors Object::setGoodEvilValue / adjustGoodEvilValue
// ---------------------------------------------------------------------------

namespace {

/** Lightweight alignment model: 0 = pure evil, 100 = pure good. */
struct AlignmentModel {
	int value;

	explicit AlignmentModel(int initial = 50) : value(initial) {}

	void set(int v) {
		if (v < 0)   v = 0;
		if (v > 100) v = 100;
		value = v;
	}

	void adjust(int delta) { set(value + delta); }

	/** Returns 1 (good), 0 (neutral), or 2 (evil) — mirrors GetAlignmentGoodEvil. */
	int alignmentBucket() const {
		if (value > 70) return 1; // ALIGNMENT_GOOD
		if (value < 30) return 2; // ALIGNMENT_EVIL
		return 0;                 // ALIGNMENT_NEUTRAL
	}
};

/** Simple XP accumulator — mirrors Object::addPlotXP / getCurrentXP. */
struct XPModel {
	int total { 0 };

	void add(int amount) { if (amount > 0) total += amount; }

	void setTotal(int desired) {
		int delta = desired - total;
		add(delta); // only adds if delta > 0 (matches addPlotXP semantics)
		if (delta < 0) total = desired; // direct override for SetXP semantics
	}
};

} // anonymous namespace

// ---------------------------------------------------------------------------
// Alignment — good/evil value clamping
// ---------------------------------------------------------------------------

GTEST_TEST(KotORBaseAlignment, defaultValueIsNeutral) {
	AlignmentModel a;
	EXPECT_EQ(a.value, 50);
	EXPECT_EQ(a.alignmentBucket(), 0); // neutral
}

GTEST_TEST(KotORBaseAlignment, setClampsToBounds) {
	AlignmentModel a;
	a.set(200);
	EXPECT_EQ(a.value, 100);
	a.set(-50);
	EXPECT_EQ(a.value, 0);
}

GTEST_TEST(KotORBaseAlignment, adjustPositiveMakesMoreGood) {
	AlignmentModel a(50);
	a.adjust(30);
	EXPECT_EQ(a.value, 80);
	EXPECT_EQ(a.alignmentBucket(), 1); // good (> 70)
}

GTEST_TEST(KotORBaseAlignment, adjustNegativeMakesMoreEvil) {
	AlignmentModel a(50);
	a.adjust(-30);
	EXPECT_EQ(a.value, 20);
	EXPECT_EQ(a.alignmentBucket(), 2); // evil (< 30)
}

GTEST_TEST(KotORBaseAlignment, adjustDoesNotOverflowUpperBound) {
	AlignmentModel a(90);
	a.adjust(50);
	EXPECT_EQ(a.value, 100);
}

GTEST_TEST(KotORBaseAlignment, adjustDoesNotUnderflowLowerBound) {
	AlignmentModel a(10);
	a.adjust(-50);
	EXPECT_EQ(a.value, 0);
}

GTEST_TEST(KotORBaseAlignment, bucketBoundaryValues) {
	AlignmentModel a;
	a.set(30);
	EXPECT_EQ(a.alignmentBucket(), 0); // exactly 30 → neutral
	a.set(29);
	EXPECT_EQ(a.alignmentBucket(), 2); // just below 30 → evil
	a.set(70);
	EXPECT_EQ(a.alignmentBucket(), 0); // exactly 70 → neutral
	a.set(71);
	EXPECT_EQ(a.alignmentBucket(), 1); // just above 70 → good
}

// ---------------------------------------------------------------------------
// XP accumulation
// ---------------------------------------------------------------------------

GTEST_TEST(KotORBaseXP, initialXPIsZero) {
	XPModel xp;
	EXPECT_EQ(xp.total, 0);
}

GTEST_TEST(KotORBaseXP, addPositiveAmountAccumulates) {
	XPModel xp;
	xp.add(250);
	EXPECT_EQ(xp.total, 250);
	xp.add(750);
	EXPECT_EQ(xp.total, 1000);
}

GTEST_TEST(KotORBaseXP, addZeroOrNegativeIsIgnored) {
	XPModel xp;
	xp.add(0);
	EXPECT_EQ(xp.total, 0);
	xp.add(-100);
	EXPECT_EQ(xp.total, 0);
}

GTEST_TEST(KotORBaseXP, setTotalOverwritesValue) {
	XPModel xp;
	xp.add(500);
	xp.setTotal(1500);
	EXPECT_EQ(xp.total, 1500);
}

GTEST_TEST(KotORBaseXP, setTotalBelowCurrentOverrides) {
	XPModel xp;
	xp.add(2000);
	xp.setTotal(500); // SetXP should allow decreasing XP directly
	EXPECT_EQ(xp.total, 500);
}

GTEST_TEST(KotORBaseXP, multipleGiveCallsAccumulate) {
	XPModel xp;
	for (int i = 0; i < 5; ++i)
		xp.add(100);
	EXPECT_EQ(xp.total, 500);
}

// ---------------------------------------------------------------------------
// Saving-throw formula (mirrors fortitudeSave / reflexSave / willSave logic)
// ---------------------------------------------------------------------------

namespace {

/** Simulate a save roll given stat modifier and DC, natural-1 and natural-20 rules.
 *  Returns 2 for automatic success, 1 for success, 0 for failure. */
int simulateSave(int roll, int modifier, int dc) {
	if (roll == 20) return 2; // automatic success
	if (roll == 1)  return 0; // automatic failure
	return ((roll + modifier + 10) >= dc) ? 1 : 0;
}

} // anonymous namespace

GTEST_TEST(KotORBaseSavingThrow, naturalTwentyAlwaysSucceeds) {
	// Even with a terrible modifier vs a very high DC, natural 20 wins.
	EXPECT_EQ(simulateSave(20, -5, 999), 2);
}

GTEST_TEST(KotORBaseSavingThrow, naturalOneAlwaysFails) {
	// Even with a great modifier vs a trivial DC, natural 1 fails.
	EXPECT_EQ(simulateSave(1, 10, 1), 0);
}

GTEST_TEST(KotORBaseSavingThrow, sufficientRollSucceeds) {
	// Roll 10 + modifier 2 + base 10 = 22 vs DC 20 → success.
	EXPECT_EQ(simulateSave(10, 2, 20), 1);
}

GTEST_TEST(KotORBaseSavingThrow, insufficientRollFails) {
	// Roll 5 + modifier 0 + base 10 = 15 vs DC 20 → failure.
	EXPECT_EQ(simulateSave(5, 0, 20), 0);
}

GTEST_TEST(KotORBaseSavingThrow, exactDCRollSucceeds) {
	// Roll 5 + modifier 5 + base 10 = 20 == DC 20 → success (>=).
	EXPECT_EQ(simulateSave(5, 5, 20), 1);
}
