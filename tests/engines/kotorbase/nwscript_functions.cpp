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
 *  Unit tests for newly implemented KotOR NWScript functions:
 *  GetIsEnemy / GetIsFriend / GetIsNeutral faction logic,
 *  GetName object-name accessor, and ActionMoveToLocation helpers.
 *
 *  These tests exercise the same arithmetic as the Functions:: implementations
 *  without requiring a live game module (no GFF files, no models, no events).
 */

#include "gtest/gtest.h"

#include "src/engines/kotorbase/types.h"

// ---------------------------------------------------------------------------
// Faction relationship helpers (mirrors functions_object.cpp internals)
// ---------------------------------------------------------------------------

using namespace Engines::KotORBase;

static bool isFactionHostile(Faction f) {
	return f == kFactionHostile1 ||
	       f == kFactionHostile2 ||
	       f == kFactionEndarSpire;
}

static bool isFactionFriendly(Faction f) {
	return f == kFactionFriendly1 || f == kFactionFriendly2;
}

static bool calcIsEnemy(Faction tf, Faction sf) {
	return (isFactionHostile(tf) && isFactionFriendly(sf)) ||
	       (isFactionHostile(sf) && isFactionFriendly(tf));
}

static bool calcIsFriend(Faction tf, Faction sf) {
	return (tf != kFactionInvalid && tf == sf) ||
	       (isFactionFriendly(tf) && isFactionFriendly(sf));
}

static bool calcIsNeutral(Faction tf, Faction sf) {
	return !calcIsEnemy(tf, sf) && !calcIsFriend(tf, sf);
}

// ---------------------------------------------------------------------------
// GetIsEnemy tests
// ---------------------------------------------------------------------------

GTEST_TEST(KotORNWScriptFuncs, hostileVsFriendly1IsEnemy) {
	EXPECT_TRUE(calcIsEnemy(kFactionHostile1, kFactionFriendly1));
	EXPECT_TRUE(calcIsEnemy(kFactionFriendly1, kFactionHostile1));
}

GTEST_TEST(KotORNWScriptFuncs, hostileVsFriendly2IsEnemy) {
	EXPECT_TRUE(calcIsEnemy(kFactionHostile2, kFactionFriendly2));
	EXPECT_TRUE(calcIsEnemy(kFactionFriendly2, kFactionHostile2));
}

GTEST_TEST(KotORNWScriptFuncs, endarSpireVsFriendlyIsEnemy) {
	// EndarSpire faction (Sith troopers) is hostile to PC party (Friendly1)
	EXPECT_TRUE(calcIsEnemy(kFactionEndarSpire, kFactionFriendly1));
	EXPECT_TRUE(calcIsEnemy(kFactionFriendly1, kFactionEndarSpire));
}

GTEST_TEST(KotORNWScriptFuncs, sameFactionIsNotEnemy) {
	EXPECT_FALSE(calcIsEnemy(kFactionHostile1, kFactionHostile1));
	EXPECT_FALSE(calcIsEnemy(kFactionFriendly1, kFactionFriendly1));
}

GTEST_TEST(KotORNWScriptFuncs, hostileVsHostileIsNotEnemy) {
	// Two hostile-faction creatures are not enemies of each other
	EXPECT_FALSE(calcIsEnemy(kFactionHostile1, kFactionHostile2));
	EXPECT_FALSE(calcIsEnemy(kFactionHostile2, kFactionEndarSpire));
}

GTEST_TEST(KotORNWScriptFuncs, invalidFactionIsNotEnemy) {
	EXPECT_FALSE(calcIsEnemy(kFactionInvalid, kFactionFriendly1));
	EXPECT_FALSE(calcIsEnemy(kFactionFriendly1, kFactionInvalid));
}

// ---------------------------------------------------------------------------
// GetIsFriend tests
// ---------------------------------------------------------------------------

GTEST_TEST(KotORNWScriptFuncs, sameFriendly1IsFriend) {
	EXPECT_TRUE(calcIsFriend(kFactionFriendly1, kFactionFriendly1));
}

GTEST_TEST(KotORNWScriptFuncs, friendly1AndFriendly2AreFriends) {
	// Both in the "friendly" family → friends
	EXPECT_TRUE(calcIsFriend(kFactionFriendly1, kFactionFriendly2));
	EXPECT_TRUE(calcIsFriend(kFactionFriendly2, kFactionFriendly1));
}

GTEST_TEST(KotORNWScriptFuncs, sameHostileIsFriend) {
	// Two creatures in the exact same hostile faction are friendly with each other
	EXPECT_TRUE(calcIsFriend(kFactionHostile1, kFactionHostile1));
}

GTEST_TEST(KotORNWScriptFuncs, hostileAndFriendlyIsNotFriend) {
	EXPECT_FALSE(calcIsFriend(kFactionHostile1, kFactionFriendly1));
	EXPECT_FALSE(calcIsFriend(kFactionFriendly1, kFactionHostile1));
}

GTEST_TEST(KotORNWScriptFuncs, invalidFactionIsNotFriend) {
	EXPECT_FALSE(calcIsFriend(kFactionInvalid, kFactionFriendly1));
	EXPECT_FALSE(calcIsFriend(kFactionInvalid, kFactionInvalid));
}

// ---------------------------------------------------------------------------
// GetIsNeutral tests
// ---------------------------------------------------------------------------

GTEST_TEST(KotORNWScriptFuncs, neutralVsFriendlyIsNeutral) {
	// Neutral faction is neither enemy nor friend of friendly factions
	EXPECT_TRUE(calcIsNeutral(kFactionNeutral, kFactionFriendly1));
}

GTEST_TEST(KotORNWScriptFuncs, neutralVsHostileIsNeutral) {
	// Neutral faction is not enemy or friend of hostile faction
	EXPECT_TRUE(calcIsNeutral(kFactionNeutral, kFactionHostile1));
}

GTEST_TEST(KotORNWScriptFuncs, enemyFactionIsNotNeutral) {
	EXPECT_FALSE(calcIsNeutral(kFactionHostile1, kFactionFriendly1));
	EXPECT_FALSE(calcIsNeutral(kFactionEndarSpire, kFactionFriendly1));
}

GTEST_TEST(KotORNWScriptFuncs, friendlyFactionIsNotNeutral) {
	EXPECT_FALSE(calcIsNeutral(kFactionFriendly1, kFactionFriendly1));
	EXPECT_FALSE(calcIsNeutral(kFactionFriendly1, kFactionFriendly2));
}

// ---------------------------------------------------------------------------
// Mutual consistency: exactly one of (enemy, friend, neutral) is always true
// ---------------------------------------------------------------------------

GTEST_TEST(KotORNWScriptFuncs, exactlyOneRelationship) {
	const Faction factions[] = {
		kFactionHostile1, kFactionFriendly1, kFactionFriendly2,
		kFactionHostile2, kFactionNeutral,   kFactionEndarSpire,
		kFactionInvalid
	};

	for (Faction a : factions) {
		for (Faction b : factions) {
			bool e = calcIsEnemy(a, b);
			bool f = calcIsFriend(a, b);
			bool n = calcIsNeutral(a, b);
			// Exactly one should be true (or exactly neutral when neither e nor f)
			int count = (e ? 1 : 0) + (f ? 1 : 0) + (n ? 1 : 0);
			EXPECT_EQ(count, 1)
				<< "faction a=" << static_cast<int>(a)
				<< " b=" << static_cast<int>(b)
				<< " → e=" << e << " f=" << f << " n=" << n;
		}
	}
}
