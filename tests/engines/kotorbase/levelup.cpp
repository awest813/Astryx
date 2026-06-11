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
 *  Unit tests for KotOR level-up helpers.
 */

#include "gtest/gtest.h"

#include "src/engines/kotorbase/levelup.h"
#include "src/engines/kotorbase/types.h"

using Engines::KotORBase::hpGainOnLevelUp;
using Engines::KotORBase::kClassJediGuardian;
using Engines::KotORBase::kClassScoundrel;
using Engines::KotORBase::kClassScout;
using Engines::KotORBase::kClassSoldier;
using Engines::KotORBase::levelUpThreshold;

TEST(LevelUpHelpers, ThresholdLevel2Is1000) {
	EXPECT_EQ(levelUpThreshold(1), 1000);
}

TEST(LevelUpHelpers, ThresholdLevel3Is3000) {
	EXPECT_EQ(levelUpThreshold(2), 3000);
}

TEST(LevelUpHelpers, ThresholdIncreasesMonotonically) {
	for (int lvl = 1; lvl < 10; ++lvl)
		EXPECT_LT(levelUpThreshold(lvl), levelUpThreshold(lvl + 1));
}

TEST(LevelUpHelpers, ScoundrelHPGainCON12) {
	EXPECT_EQ(hpGainOnLevelUp(kClassScoundrel, 1), 5);
}

TEST(LevelUpHelpers, SoldierHPGainCON14) {
	EXPECT_EQ(hpGainOnLevelUp(kClassSoldier, 2), 8);
}

TEST(LevelUpHelpers, ScoutHPGainCON10) {
	EXPECT_EQ(hpGainOnLevelUp(kClassScout, 0), 5);
}

TEST(LevelUpHelpers, JediGuardianHPGainMinimumOne) {
	EXPECT_EQ(hpGainOnLevelUp(kClassJediGuardian, -4), 1);
}
