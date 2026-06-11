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

#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/creatureinfo.h"
#include "src/engines/kotorbase/gui/chargeninfo.h"
#include "src/engines/kotorbase/levelup.h"
#include "src/engines/kotorbase/types.h"

using Engines::KotORBase::Creature;
using Engines::KotORBase::CreatureInfo;
using Engines::KotORBase::applyDefaultChargenBuild;
using Engines::KotORBase::applyJediClass;
using Engines::KotORBase::CharacterGenerationInfo;
using Engines::KotORBase::grantsAbilityIncrease;
using Engines::KotORBase::previewStatsAtLevel1;
using Engines::KotORBase::hpGainOnLevelUp;
using Engines::KotORBase::kAbilityCharisma;
using Engines::KotORBase::kAbilityIntelligence;
using Engines::KotORBase::kAbilityWisdom;
using Engines::KotORBase::kClassJediGuardian;
using Engines::KotORBase::kClassJediWeaponMaster;
using Engines::KotORBase::kClassScoundrel;
using Engines::KotORBase::kClassScout;
using Engines::KotORBase::kClassSoldier;
using Engines::KotORBase::kFeatJediDefense;
using Engines::KotORBase::kFeatPowerAttack;
using Engines::KotORBase::levelUpThreshold;
using Engines::KotORBase::getSelectableFeats;
using Engines::KotORBase::getClassDisplayName;
using Engines::KotORBase::getForcePowerDisplayName;
using Engines::KotORBase::formatAbilityModifier;
using Engines::KotORBase::isJediClass;
using Engines::KotORBase::kClassJediConsular;

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

TEST(LevelUpHelpers, AbilityIncreaseAtLevelsFourEightTwelve) {
	EXPECT_FALSE(grantsAbilityIncrease(1));
	EXPECT_FALSE(grantsAbilityIncrease(2));
	EXPECT_TRUE(grantsAbilityIncrease(3));
	EXPECT_FALSE(grantsAbilityIncrease(4));
	EXPECT_TRUE(grantsAbilityIncrease(7));
	EXPECT_TRUE(grantsAbilityIncrease(11));
}

TEST(LevelUpHelpers, PreviewStatsSoldierCon14) {
	KotORBase::CreatureInfo::Abilities ab;
	ab.strength = 14;
	ab.dexterity = 12;
	ab.constitution = 14;
	ab.intelligence = 10;
	ab.wisdom = 10;
	ab.charisma = 10;

	const auto stats = previewStatsAtLevel1(kClassSoldier, ab);
	EXPECT_EQ(stats.vitality, 12); // d10 + 2 CON
	EXPECT_EQ(stats.defense, 11);  // 10 + 1 DEX
	EXPECT_EQ(stats.fortitude, 13);
}

TEST(LevelUpHelpers, FormatAbilityModifierPositive) {
	EXPECT_EQ(formatAbilityModifier(3), "+3");
}

TEST(LevelUpHelpers, FormatAbilityModifierNegative) {
	EXPECT_EQ(formatAbilityModifier(-2), "-2");
}

TEST(LevelUpHelpers, ClassDisplayNameJediFallback) {
	EXPECT_EQ(getClassDisplayName(kClassJediConsular), "Jedi Consular");
}

TEST(LevelUpHelpers, SoldierFeatListExcludesKnownFeats) {
	CreatureInfo info;
	info.incrementClassLevel(kClassSoldier);
	info.addFeat(kFeatPowerAttack);

	const auto feats = getSelectableFeats(info);
	for (uint32_t feat : feats)
		EXPECT_NE(feat, kFeatPowerAttack);
}

TEST(LevelUpHelpers, IsJediClassIncludesPrestige) {
	EXPECT_TRUE(isJediClass(kClassJediWeaponMaster));
	EXPECT_FALSE(isJediClass(kClassSoldier));
}

TEST(LevelUpHelpers, IsJediWithPrestigeClass) {
	CreatureInfo info;
	info.incrementClassLevel(kClassJediWeaponMaster);
	EXPECT_TRUE(info.isJedi());
}

TEST(LevelUpHelpers, ForcePowerDisplayNameFallback) {
	EXPECT_EQ(getForcePowerDisplayName(1), "Force Heal");
	EXPECT_EQ(getForcePowerDisplayName(5), "Force Stun");
}

TEST(LevelUpHelpers, ChargenInfoCopiesFeats) {
	CharacterGenerationInfo info;
	info.addFeat(kFeatPowerAttack);

	const CreatureInfo creatureInfo(info);
	EXPECT_TRUE(creatureInfo.hasFeat(kFeatPowerAttack));
}

TEST(LevelUpHelpers, ApplyDefaultChargenBuildFillsQuickChar) {
	CharacterGenerationInfo info;
	info.setAbilityScore(kAbilityIntelligence, 12);

	applyDefaultChargenBuild(info);

	EXPECT_FALSE(info.getFeats().empty());
	EXPECT_GT(info.getSkills().awareness, 0u);
}

TEST(LevelUpHelpers, ApplyJediClassGrantsDefenseAndForcePool) {
	Creature creature;
	creature.initAsFakePC();

	CreatureInfo &info = creature.getCreatureInfo();
	info.setAbilityScore(kAbilityWisdom, 14);
	info.setAbilityScore(kAbilityCharisma, 12);

	applyJediClass(creature, kClassJediConsular);

	EXPECT_EQ(creature.getLevel(kClassJediConsular), 1);
	EXPECT_TRUE(info.hasFeat(kFeatJediDefense));
	EXPECT_EQ(creature.getMaxForcePoints(), 11);
	EXPECT_EQ(creature.getForcePoints(), 11);
}
