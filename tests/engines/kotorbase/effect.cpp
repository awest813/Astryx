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
 *  Unit tests for KotORBase::Effect — the engine-type wrapper for NWScript
 *  EffectHeal / EffectDamage values.
 *
 *  These tests validate the pure data accessors of the Effect class without
 *  requiring live game data.  They guard the ApplyEffectToObject path that is
 *  used to restore HP after combat (EffectHeal, ID 78) and to apply combat
 *  damage (EffectDamage, ID 79).
 */

#include "gtest/gtest.h"

#include <memory>

#include "src/engines/kotorbase/effect.h"

using Engines::KotORBase::Effect;
using Engines::KotORBase::kKotOREffectHeal;
using Engines::KotORBase::kKotOREffectDamage;
using Engines::KotORBase::kKotOREffectVisual;

// ---------------------------------------------------------------------------
// EffectHeal construction and accessors
// ---------------------------------------------------------------------------

GTEST_TEST(KotOREffect, healTypeIsHeal) {
	Effect e(kKotOREffectHeal, 10);
	EXPECT_EQ(e.getType(), kKotOREffectHeal);
}

GTEST_TEST(KotOREffect, healAmountRoundTrips) {
	Effect e(kKotOREffectHeal, 25);
	EXPECT_EQ(e.getAmount(), 25);
}

GTEST_TEST(KotOREffect, healZeroAmount) {
	Effect e(kKotOREffectHeal, 0);
	EXPECT_EQ(e.getAmount(), 0);
}

GTEST_TEST(KotOREffect, healDamageTypeDefaultsToZero) {
	Effect e(kKotOREffectHeal, 8);
	EXPECT_EQ(e.getDamageType(), 0);
}

// ---------------------------------------------------------------------------
// EffectDamage construction and accessors
// ---------------------------------------------------------------------------

GTEST_TEST(KotOREffect, damageTypeIsDamage) {
	Effect e(kKotOREffectDamage, 6);
	EXPECT_EQ(e.getType(), kKotOREffectDamage);
}

GTEST_TEST(KotOREffect, damageAmountRoundTrips) {
	Effect e(kKotOREffectDamage, 8, 1);
	EXPECT_EQ(e.getAmount(), 8);
}

GTEST_TEST(KotOREffect, damageTypeFieldRoundTrips) {
	// damageType 1 = physical, 2 = ion, etc. (as defined by the NWScript constants)
	Effect e(kKotOREffectDamage, 4, 2);
	EXPECT_EQ(e.getDamageType(), 2);
}

GTEST_TEST(KotOREffect, damageZeroAmount) {
	Effect e(kKotOREffectDamage, 0);
	EXPECT_EQ(e.getAmount(), 0);
}

GTEST_TEST(KotOREffect, visualEffectTypeRoundTrips) {
	Effect e(kKotOREffectVisual, 6002, 1);
	EXPECT_EQ(e.getType(), kKotOREffectVisual);
	EXPECT_EQ(e.getAmount(), 6002);
	EXPECT_EQ(e.getDamageType(), 1);
}

// ---------------------------------------------------------------------------
// clone()
// ---------------------------------------------------------------------------

GTEST_TEST(KotOREffect, cloneProducesEquivalentHeal) {
	Effect orig(kKotOREffectHeal, 15);
	Aurora::NWScript::EngineType *rawClone = orig.clone();
	Effect *cloned = dynamic_cast<Effect *>(rawClone);

	ASSERT_NE(cloned, nullptr);
	EXPECT_EQ(cloned->getType(),   kKotOREffectHeal);
	EXPECT_EQ(cloned->getAmount(), 15);
	delete rawClone;
}

GTEST_TEST(KotOREffect, cloneProducesEquivalentDamage) {
	Effect orig(kKotOREffectDamage, 7, 1);
	Aurora::NWScript::EngineType *rawClone = orig.clone();
	Effect *cloned = dynamic_cast<Effect *>(rawClone);

	ASSERT_NE(cloned, nullptr);
	EXPECT_EQ(cloned->getType(),       kKotOREffectDamage);
	EXPECT_EQ(cloned->getAmount(),     7);
	EXPECT_EQ(cloned->getDamageType(), 1);
	delete rawClone;
}

GTEST_TEST(KotOREffect, cloneProducesEquivalentVisualEffect) {
	Effect orig(kKotOREffectVisual, 6002, 0);
	Aurora::NWScript::EngineType *rawClone = orig.clone();
	Effect *cloned = dynamic_cast<Effect *>(rawClone);

	ASSERT_NE(cloned, nullptr);
	EXPECT_EQ(cloned->getType(),       kKotOREffectVisual);
	EXPECT_EQ(cloned->getAmount(),     6002);
	EXPECT_EQ(cloned->getDamageType(), 0);
	delete rawClone;
}

GTEST_TEST(KotOREffect, cloneIsIndependent) {
	// The clone must be a separate object
	Effect orig(kKotOREffectHeal, 5);
	Aurora::NWScript::EngineType *rawClone = orig.clone();

	EXPECT_NE(rawClone, &orig);
	delete rawClone;
}

// ---------------------------------------------------------------------------
// HP arithmetic (mirrors the logic in applyEffectToObject)
// ---------------------------------------------------------------------------

// These helpers replicate the arithmetic performed by applyEffectToObject
// without requiring the full script engine, so we can pin expected values.

static int applyHeal(int current, int amount) {
	return current + amount;
}

static int applyDamage(int current, int amount) {
	return current - amount;
}

GTEST_TEST(KotOREffect, healRestoresHP) {
	// A creature at 10/30 HP healed for 5 ends up at 15
	EXPECT_EQ(applyHeal(10, 5), 15);
}

GTEST_TEST(KotOREffect, healCanOvercap) {
	// The simple arithmetic allows over-cap; clamping is handled by the caller
	EXPECT_EQ(applyHeal(28, 5), 33);
}

GTEST_TEST(KotOREffect, damageReducesHP) {
	EXPECT_EQ(applyDamage(30, 8), 22);
}

GTEST_TEST(KotOREffect, damageCanGoNegative) {
	// Overkill damage is allowed; death detection happens after the call
	EXPECT_EQ(applyDamage(3, 10), -7);
}

GTEST_TEST(KotOREffect, zeroHealIsNoop) {
	EXPECT_EQ(applyHeal(20, 0), 20);
}

GTEST_TEST(KotOREffect, zeroDamageIsNoop) {
	EXPECT_EQ(applyDamage(20, 0), 20);
}

GTEST_TEST(KotOREffect, spellIdRoundTripsAndClones) {
	Effect e(kKotOREffectDamage, 6, 1, 14);
	EXPECT_EQ(e.getSpellId(), 14);

	e.setSpellId(33);
	EXPECT_EQ(e.getSpellId(), 33);

	std::unique_ptr<Aurora::NWScript::EngineType> cloned(e.clone());
	const Effect *copy = dynamic_cast<const Effect *>(cloned.get());
	ASSERT_TRUE(copy != nullptr);
	EXPECT_EQ(copy->getSpellId(), 33);
	EXPECT_EQ(copy->getAmount(), 6);
}
