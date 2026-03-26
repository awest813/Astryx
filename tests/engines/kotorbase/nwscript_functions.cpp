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
 *  Unit tests for Endar Spire NWScript relationship helpers.
 */

#include "gtest/gtest.h"

#include "src/engines/kotorbase/script/endar_spire_support.h"
#include "src/engines/kotorbase/types.h"

using namespace Engines::KotORBase;

GTEST_TEST(KotORNWScriptFuncs, hostileVsFriendly1IsEnemy) {
	EXPECT_TRUE(EndarSpireSupport::areFactionsEnemy(kFactionHostile1, kFactionFriendly1));
	EXPECT_TRUE(EndarSpireSupport::areFactionsEnemy(kFactionFriendly1, kFactionHostile1));
}

GTEST_TEST(KotORNWScriptFuncs, hostileVsFriendly2IsEnemy) {
	EXPECT_TRUE(EndarSpireSupport::areFactionsEnemy(kFactionHostile2, kFactionFriendly2));
	EXPECT_TRUE(EndarSpireSupport::areFactionsEnemy(kFactionFriendly2, kFactionHostile2));
}

GTEST_TEST(KotORNWScriptFuncs, endarSpireVsFriendlyIsEnemy) {
	EXPECT_TRUE(EndarSpireSupport::areFactionsEnemy(kFactionEndarSpire, kFactionFriendly1));
	EXPECT_TRUE(EndarSpireSupport::areFactionsEnemy(kFactionFriendly1, kFactionEndarSpire));
}

GTEST_TEST(KotORNWScriptFuncs, sameFactionIsNotEnemy) {
	EXPECT_FALSE(EndarSpireSupport::areFactionsEnemy(kFactionHostile1, kFactionHostile1));
	EXPECT_FALSE(EndarSpireSupport::areFactionsEnemy(kFactionFriendly1, kFactionFriendly1));
}

GTEST_TEST(KotORNWScriptFuncs, hostileVsHostileIsNotEnemy) {
	EXPECT_FALSE(EndarSpireSupport::areFactionsEnemy(kFactionHostile1, kFactionHostile2));
	EXPECT_FALSE(EndarSpireSupport::areFactionsEnemy(kFactionHostile2, kFactionEndarSpire));
}

GTEST_TEST(KotORNWScriptFuncs, invalidFactionIsNotEnemy) {
	EXPECT_FALSE(EndarSpireSupport::areFactionsEnemy(kFactionInvalid, kFactionFriendly1));
	EXPECT_FALSE(EndarSpireSupport::areFactionsEnemy(kFactionFriendly1, kFactionInvalid));
}

GTEST_TEST(KotORNWScriptFuncs, sameFriendly1IsFriend) {
	EXPECT_TRUE(EndarSpireSupport::areFactionsFriendly(kFactionFriendly1, kFactionFriendly1));
}

GTEST_TEST(KotORNWScriptFuncs, friendly1AndFriendly2AreFriends) {
	EXPECT_TRUE(EndarSpireSupport::areFactionsFriendly(kFactionFriendly1, kFactionFriendly2));
	EXPECT_TRUE(EndarSpireSupport::areFactionsFriendly(kFactionFriendly2, kFactionFriendly1));
}

GTEST_TEST(KotORNWScriptFuncs, sameHostileIsFriend) {
	EXPECT_TRUE(EndarSpireSupport::areFactionsFriendly(kFactionHostile1, kFactionHostile1));
}

GTEST_TEST(KotORNWScriptFuncs, hostileAndFriendlyIsNotFriend) {
	EXPECT_FALSE(EndarSpireSupport::areFactionsFriendly(kFactionHostile1, kFactionFriendly1));
	EXPECT_FALSE(EndarSpireSupport::areFactionsFriendly(kFactionFriendly1, kFactionHostile1));
}

GTEST_TEST(KotORNWScriptFuncs, invalidFactionIsNotFriend) {
	EXPECT_FALSE(EndarSpireSupport::areFactionsFriendly(kFactionInvalid, kFactionFriendly1));
	EXPECT_FALSE(EndarSpireSupport::areFactionsFriendly(kFactionInvalid, kFactionInvalid));
}

GTEST_TEST(KotORNWScriptFuncs, neutralVsFriendlyIsNeutral) {
	EXPECT_TRUE(EndarSpireSupport::areFactionsNeutral(kFactionNeutral, kFactionFriendly1));
}

GTEST_TEST(KotORNWScriptFuncs, neutralVsHostileIsNeutral) {
	EXPECT_TRUE(EndarSpireSupport::areFactionsNeutral(kFactionNeutral, kFactionHostile1));
}

GTEST_TEST(KotORNWScriptFuncs, enemyFactionIsNotNeutral) {
	EXPECT_FALSE(EndarSpireSupport::areFactionsNeutral(kFactionHostile1, kFactionFriendly1));
	EXPECT_FALSE(EndarSpireSupport::areFactionsNeutral(kFactionEndarSpire, kFactionFriendly1));
}

GTEST_TEST(KotORNWScriptFuncs, friendlyFactionIsNotNeutral) {
	EXPECT_FALSE(EndarSpireSupport::areFactionsNeutral(kFactionFriendly1, kFactionFriendly1));
	EXPECT_FALSE(EndarSpireSupport::areFactionsNeutral(kFactionFriendly1, kFactionFriendly2));
}

GTEST_TEST(KotORNWScriptFuncs, exactlyOneRelationship) {
	const Faction factions[] = {
		kFactionHostile1, kFactionFriendly1, kFactionFriendly2,
		kFactionHostile2, kFactionNeutral,   kFactionEndarSpire,
		kFactionInvalid
	};

	for (Faction a : factions) {
		for (Faction b : factions) {
			bool enemy = EndarSpireSupport::areFactionsEnemy(a, b);
			bool friendly = EndarSpireSupport::areFactionsFriendly(a, b);
			bool neutral = EndarSpireSupport::areFactionsNeutral(a, b);

			int count = (enemy ? 1 : 0) + (friendly ? 1 : 0) + (neutral ? 1 : 0);
			EXPECT_EQ(count, 1)
				<< "faction a=" << static_cast<int>(a)
				<< " b=" << static_cast<int>(b)
				<< " -> e=" << enemy
				<< " f=" << friendly
				<< " n=" << neutral;
		}
	}
}
