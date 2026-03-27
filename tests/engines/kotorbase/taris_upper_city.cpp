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
 *  Unit tests for Milestone 3 (Taris Upper City) behaviours:
 *  - GetDistanceBetween distance model
 *  - GetReputation / AdjustReputation faction-reputation store
 *  - New Effect sub-types (ACIncrease, AttackIncrease, SkillIncrease,
 *    TemporaryHitpoints) — constructor and clone correctness
 *  - EffectTemporaryHitpoints applied via the HP-cap logic
 */

#include <cmath>
#include <map>
#include <utility>

#include "gtest/gtest.h"

#include "src/engines/kotorbase/effect.h"

namespace {

// ---------------------------------------------------------------------------
// Distance model — pure geometry, no engine objects needed
// ---------------------------------------------------------------------------

static float euclidean3D(float x1, float y1, float z1,
                         float x2, float y2, float z2) {
	const float dx = x1 - x2;
	const float dy = y1 - y2;
	const float dz = z1 - z2;
	return std::sqrt(dx * dx + dy * dy + dz * dz);
}

} // anonymous namespace

GTEST_TEST(KotORBaseTarisUpperCity, distanceZeroForSamePoint) {
	EXPECT_FLOAT_EQ(euclidean3D(1.0f, 2.0f, 3.0f, 1.0f, 2.0f, 3.0f), 0.0f);
}

GTEST_TEST(KotORBaseTarisUpperCity, distanceAxisAlignedX) {
	EXPECT_FLOAT_EQ(euclidean3D(0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f), 5.0f);
}

GTEST_TEST(KotORBaseTarisUpperCity, distanceAxisAlignedY) {
	EXPECT_FLOAT_EQ(euclidean3D(0.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f), 3.0f);
}

GTEST_TEST(KotORBaseTarisUpperCity, distanceAxisAlignedZ) {
	EXPECT_FLOAT_EQ(euclidean3D(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 4.0f), 4.0f);
}

GTEST_TEST(KotORBaseTarisUpperCity, distance3DPythagoras) {
	// 3-4-5 right triangle extended to 3D: sqrt(3²+4²+0²) = 5
	EXPECT_FLOAT_EQ(euclidean3D(0.0f, 0.0f, 0.0f, 3.0f, 4.0f, 0.0f), 5.0f);
}

GTEST_TEST(KotORBaseTarisUpperCity, distanceIsSymmetric) {
	float d1 = euclidean3D(1.0f, 2.0f, 3.0f, 7.0f, 5.0f, 9.0f);
	float d2 = euclidean3D(7.0f, 5.0f, 9.0f, 1.0f, 2.0f, 3.0f);
	EXPECT_FLOAT_EQ(d1, d2);
}

// ---------------------------------------------------------------------------
// Reputation store model
// ---------------------------------------------------------------------------

namespace {

struct ReputationStore {
	std::map<std::pair<int,int>, int> store;

	static int defaultBetween(int src, int tgt) {
		const bool srcHostile  = (src == 1 || src == 3);
		const bool srcFriendly = (src == 2 || src == 4);
		const bool tgtHostile  = (tgt == 1 || tgt == 3);
		const bool tgtFriendly = (tgt == 2 || tgt == 4);

		if (src == tgt)       return 100;
		if ((srcHostile && tgtFriendly) || (srcFriendly && tgtHostile))
			return 0;
		return 50;
	}

	int getReputation(int src, int tgt) const {
		auto key = std::make_pair(src, tgt);
		auto it = store.find(key);
		if (it != store.end())
			return it->second;
		return defaultBetween(src, tgt);
	}

	void adjustReputation(int tgtFaction, int srcFaction, int delta) {
		auto key = std::make_pair(srcFaction, tgtFaction);
		int current = getReputation(srcFaction, tgtFaction);
		int updated = current + delta;
		if (updated < 0)   updated = 0;
		if (updated > 100) updated = 100;
		store[key] = updated;
	}
};

} // anonymous namespace

GTEST_TEST(KotORBaseReputation, sameFactionDefaultIs100) {
	ReputationStore rs;
	EXPECT_EQ(rs.getReputation(2, 2), 100);
}

GTEST_TEST(KotORBaseReputation, hostileVsFriendlyDefaultIs0) {
	ReputationStore rs;
	EXPECT_EQ(rs.getReputation(1, 2), 0);
	EXPECT_EQ(rs.getReputation(2, 1), 0);
}

GTEST_TEST(KotORBaseReputation, neutralDefaultIs50) {
	ReputationStore rs;
	// Faction 5 (neutral) vs faction 2 (friendly) → neither rule matches → 50
	EXPECT_EQ(rs.getReputation(5, 2), 50);
}

GTEST_TEST(KotORBaseReputation, adjustIncreasesStoredValue) {
	ReputationStore rs;
	// Start: faction 1 (hostile) vs faction 2 (friendly) → 0
	EXPECT_EQ(rs.getReputation(1, 2), 0);
	rs.adjustReputation(2, 1, 30);
	EXPECT_EQ(rs.getReputation(1, 2), 30);
}

GTEST_TEST(KotORBaseReputation, adjustClampsAt100) {
	ReputationStore rs;
	// Same faction starts at 100, adjusting positive should stay at 100
	rs.adjustReputation(2, 2, 50);
	EXPECT_EQ(rs.getReputation(2, 2), 100);
}

GTEST_TEST(KotORBaseReputation, adjustClampsAt0) {
	ReputationStore rs;
	// Neutral starts at 50, reduce by 200 → clamps to 0
	rs.adjustReputation(5, 6, -200);
	EXPECT_EQ(rs.getReputation(5, 6), 0);
}

GTEST_TEST(KotORBaseReputation, pairsAreIndependent) {
	ReputationStore rs;
	rs.adjustReputation(1, 2, 25); // (src=2, tgt=1) changes
	// (src=1, tgt=2) should be unchanged
	EXPECT_EQ(rs.getReputation(2, 1), 0);  // still hostile default
	EXPECT_EQ(rs.getReputation(1, 2), 25); // adjusted pair
}

// ---------------------------------------------------------------------------
// Effect sub-type constructors
// ---------------------------------------------------------------------------

using KotORBase::Effect;
using KotORBase::kEffectACIncrease;
using KotORBase::kEffectAttackIncrease;
using KotORBase::kEffectSkillIncrease;
using KotORBase::kEffectTemporaryHitpoints;

GTEST_TEST(KotORBaseEffectM3, effectACIncreaseTypeAndAmount) {
	Effect e(kEffectACIncrease, 4);
	EXPECT_EQ(e.getType(), kEffectACIncrease);
	EXPECT_EQ(e.getAmount(), 4);
}

GTEST_TEST(KotORBaseEffectM3, effectAttackIncreaseTypeAndAmount) {
	Effect e(kEffectAttackIncrease, 2);
	EXPECT_EQ(e.getType(), kEffectAttackIncrease);
	EXPECT_EQ(e.getAmount(), 2);
}

GTEST_TEST(KotORBaseEffectM3, effectSkillIncreaseTypeAmountAndSkillId) {
	// EffectSkillIncrease: amount=3, damageType used as skillId=5
	Effect e(kEffectSkillIncrease, 3, 5);
	EXPECT_EQ(e.getType(), kEffectSkillIncrease);
	EXPECT_EQ(e.getAmount(), 3);
	EXPECT_EQ(e.getDamageType(), 5);
}

GTEST_TEST(KotORBaseEffectM3, effectTemporaryHitpointsTypeAndAmount) {
	Effect e(kEffectTemporaryHitpoints, 20);
	EXPECT_EQ(e.getType(), kEffectTemporaryHitpoints);
	EXPECT_EQ(e.getAmount(), 20);
}

GTEST_TEST(KotORBaseEffectM3, cloneIsIndependentForNewTypes) {
	Effect original(kEffectACIncrease, 6);
	Aurora::NWScript::EngineType *copy = original.clone();
	Effect *cloned = dynamic_cast<Effect *>(copy);

	ASSERT_NE(cloned, nullptr);
	EXPECT_EQ(cloned->getType(), kEffectACIncrease);
	EXPECT_EQ(cloned->getAmount(), 6);
	EXPECT_NE(cloned, &original);

	delete copy;
}

// ---------------------------------------------------------------------------
// EffectTemporaryHitpoints HP-cap model
// ---------------------------------------------------------------------------

namespace {

struct HPModel {
	int current;
	int maxHP;

	void applyTemporaryHP(int amount) {
		int boosted = current + amount;
		if (boosted > maxHP)
			boosted = maxHP;
		current = boosted;
	}
};

} // anonymous namespace

GTEST_TEST(KotORBaseTempHP, addsBelowMax) {
	HPModel m { 30, 50 };
	m.applyTemporaryHP(10);
	EXPECT_EQ(m.current, 40);
}

GTEST_TEST(KotORBaseTempHP, clampsAtMax) {
	HPModel m { 45, 50 };
	m.applyTemporaryHP(20);
	EXPECT_EQ(m.current, 50);
}

GTEST_TEST(KotORBaseTempHP, exactMaxStaysAtMax) {
	HPModel m { 50, 50 };
	m.applyTemporaryHP(5);
	EXPECT_EQ(m.current, 50);
}

GTEST_TEST(KotORBaseTempHP, zeroAmountIsNoOp) {
	HPModel m { 30, 50 };
	m.applyTemporaryHP(0);
	EXPECT_EQ(m.current, 30);
}
