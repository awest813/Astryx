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

#include <memory>
#include <vector>

#include "gtest/gtest.h"

#include "src/common/ustring.h"
#include "src/common/memreadstream.h"

#include "src/aurora/2dafile.h"

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
using Engines::KotORBase::progressionClass;
using Engines::KotORBase::skillPointsPerLevel;
using Engines::KotORBase::kClassJediConsular;
using Engines::KotORBase::completeLevelUpPicks;
using Engines::KotORBase::featsGainedAtClassLevel;
using Engines::KotORBase::forcePowersGainedAtClassLevel;
using Engines::KotORBase::gainFromTable;
using Engines::KotORBase::levelUpThresholdFromTable;
using Engines::KotORBase::killXPFromTable;
using Engines::KotORBase::killExperience;
using Engines::KotORBase::grantedFeatsFromTable;
using Engines::KotORBase::classGrantedFeats;
using Engines::KotORBase::classFeatTableCode;
using Engines::KotORBase::selectableFeatsFromTable;
using Engines::KotORBase::selectableForcePowersFromTable;
using Engines::KotORBase::forcePowerClassColumn;
using Engines::KotORBase::getSelectableForcePowers;
using Engines::KotORBase::getFeatDisplayName;
using Engines::KotORBase::kAbilityStrength;
using Engines::KotORBase::kSkillRepair;
using Engines::KotORBase::kFeatToughness;

namespace {

class TestCreature : public Creature {
public:
	TestCreature() : Creature() {}

protected:
	void getPartModelsPC(Creature::PartModels &parts, uint32_t state, uint8_t textureVariation) override {
		(void)parts;
		(void)state;
		(void)textureVariation;
	}
};

class TestCharacterGenerationInfo : public CharacterGenerationInfo {
public:
	TestCharacterGenerationInfo() : CharacterGenerationInfo() {}

	Common::UString getPortrait() const override {
		return Common::UString();
	}
};

} // End of anonymous namespace

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
	// d10/2+1 + (-4 CON) = 2; floor only kicks in below 1.
	EXPECT_EQ(hpGainOnLevelUp(kClassJediGuardian, -4), 2);
	EXPECT_EQ(hpGainOnLevelUp(kClassJediGuardian, -10), 1);
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
	CreatureInfo::Abilities ab;
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

TEST(LevelUpHelpers, ProgressionClassMapsPrestigeToBase) {
	EXPECT_EQ(progressionClass(kClassJediWeaponMaster), kClassJediGuardian);
}

TEST(LevelUpHelpers, SkillPointsPerLevelUsesPrestigeBase) {
	CreatureInfo info;
	info.incrementClassLevel(kClassJediWeaponMaster);
	EXPECT_EQ(skillPointsPerLevel(info), 1);
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
	TestCharacterGenerationInfo info;
	info.addFeat(kFeatPowerAttack);

	const CreatureInfo creatureInfo(info);
	EXPECT_TRUE(creatureInfo.hasFeat(kFeatPowerAttack));
}

TEST(LevelUpHelpers, ApplyDefaultChargenBuildFillsQuickChar) {
	TestCharacterGenerationInfo info;
	info.setAbilityScore(kAbilityIntelligence, 12);

	applyDefaultChargenBuild(info);

	EXPECT_FALSE(info.getFeats().empty());
	const auto &skills = info.getSkills();
	const uint32_t total =
		skills.computerUse + skills.demolitions + skills.stealth + skills.awareness +
		skills.persuade + skills.repair + skills.security + skills.treatInjury;
	EXPECT_GT(total, 0u);
}

TEST(LevelUpHelpers, ApplyJediClassGrantsDefenseAndForcePool) {
	TestCreature creature;
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

static Aurora::TwoDAFile *makeTwoDA(const char *text) {
	Common::MemoryReadStream stream(text);
	return new Aurora::TwoDAFile(stream);
}

TEST(LevelUpHelpers, FeatGainTableReadsPerClassLevel) {
	// featgain.2da: rows are class levels, named by the "label" column.
	std::unique_ptr<Aurora::TwoDAFile> twoda(makeTwoDA(
		"2DA V2.0\n"
		"\n"
		"   label sol_reg sol_bon jgd_reg jgd_bon\n"
		"0  1     1       0       1       0\n"
		"1  2     1       0       1       0\n"
		"2  3     0       0       ****    0\n"
		"3  4     2       0       1       0\n"));

	EXPECT_EQ(gainFromTable(*twoda, "sol_reg", 1), 1);
	EXPECT_EQ(gainFromTable(*twoda, "SOL_REG", 3), 0);
	EXPECT_EQ(gainFromTable(*twoda, "sol_reg", 4), 2);
	EXPECT_EQ(gainFromTable(*twoda, "jgd_reg", 3), 0);   // "****" grants nothing.
	EXPECT_EQ(gainFromTable(*twoda, "jgd_reg", 9), 0);   // Past the table.
	EXPECT_EQ(gainFromTable(*twoda, "sct_reg", 1), -1);  // No such class column.
	EXPECT_EQ(gainFromTable(*twoda, "sol_reg", 0), -1);
}

TEST(LevelUpHelpers, PowerGainTableWithoutLabelUsesRowIndex) {
	std::unique_ptr<Aurora::TwoDAFile> twoda(makeTwoDA(
		"2DA V2.0\n"
		"\n"
		"   jcn jgd\n"
		"0  2   1\n"
		"1  1   0\n"
		"2  1   1\n"));

	EXPECT_EQ(gainFromTable(*twoda, "jcn", 1), 2);
	EXPECT_EQ(gainFromTable(*twoda, "jgd", 2), 0);
	EXPECT_EQ(gainFromTable(*twoda, "jgd", 3), 1);
}

TEST(LevelUpHelpers, GainFallbackWithoutGameData) {
	// No classes.2da: one feat per level, Force powers only for Jedi.
	EXPECT_EQ(featsGainedAtClassLevel(kClassSoldier, 2), 1);
	EXPECT_EQ(forcePowersGainedAtClassLevel(kClassSoldier, 2), 0);
	EXPECT_EQ(forcePowersGainedAtClassLevel(kClassJediConsular, 2), 1);
	EXPECT_EQ(featsGainedAtClassLevel(kClassSoldier, 0), 0);
}

TEST(LevelUpHelpers, CompleteLevelUpPicksHonoursCount) {
	const std::vector<uint32_t> available = { 10, 11, 12, 13 };

	// Chosen picks first, unknown and duplicate ones dropped, then topped up.
	EXPECT_EQ(completeLevelUpPicks({ 12, 99, 12 }, available, 2), (std::vector<uint32_t>{ 12, 10 }));
	// Never more than the level grants.
	EXPECT_EQ(completeLevelUpPicks({ 13, 12, 11 }, available, 2), (std::vector<uint32_t>{ 13, 12 }));
	// Nothing granted, nothing picked.
	EXPECT_TRUE(completeLevelUpPicks({ 10 }, available, 0).empty());
	// Fewer choices than picks: take what there is.
	EXPECT_EQ(completeLevelUpPicks({}, { 10 }, 3), (std::vector<uint32_t>{ 10 }));
}

TEST(LevelUpHelpers, ExpTableThresholdByLevelColumn) {
	// exptable.2da: "level" is the level reached, "xp" the total XP it needs.
	std::unique_ptr<Aurora::TwoDAFile> twoda(makeTwoDA(
		"2DA V2.0\n"
		"\n"
		"   level xp\n"
		"0  1     0\n"
		"1  2     1000\n"
		"2  3     3000\n"
		"3  4     6500\n"));

	EXPECT_EQ(levelUpThresholdFromTable(*twoda, 1), 1000);
	EXPECT_EQ(levelUpThresholdFromTable(*twoda, 2), 3000);
	EXPECT_EQ(levelUpThresholdFromTable(*twoda, 3), 6500);  // The table, not the d20 formula.
	EXPECT_EQ(levelUpThresholdFromTable(*twoda, 4), -1);    // Level cap.
	EXPECT_EQ(levelUpThresholdFromTable(*twoda, 0), -1);
}

TEST(LevelUpHelpers, ExpTableThresholdWithoutLevelColumnUsesRowIndex) {
	std::unique_ptr<Aurora::TwoDAFile> twoda(makeTwoDA(
		"2DA V2.0\n"
		"\n"
		"   xp\n"
		"0  0\n"
		"1  1000\n"
		"2  3000\n"));

	EXPECT_EQ(levelUpThresholdFromTable(*twoda, 1), 1000);
	EXPECT_EQ(levelUpThresholdFromTable(*twoda, 2), 3000);
	EXPECT_EQ(levelUpThresholdFromTable(*twoda, 3), -1);
}

TEST(LevelUpHelpers, ThresholdFallbackWithoutGameData) {
	EXPECT_EQ(levelUpThreshold(3), 6000);
	EXPECT_EQ(levelUpThreshold(0), 1000);
}

TEST(LevelUpHelpers, KillXPTableByPartyLevelAndChallengeRating) {
	// xptable.2da: one row per party level (the row index, as KotOR.js reads
	// it), one "cr<N>" column per challenge rating.
	std::unique_ptr<Aurora::TwoDAFile> twoda(makeTwoDA(
		"2DA V2.0\n"
		"\n"
		"   cr0  cr1 cr2\n"
		"0  10   20  30\n"
		"1  5    15  25\n"
		"2  **** 10  20\n"));

	EXPECT_EQ(killXPFromTable(*twoda, 1, 0), 5);
	EXPECT_EQ(killXPFromTable(*twoda, 1, 2), 25);
	EXPECT_EQ(killXPFromTable(*twoda, 2, 1), 10);
	EXPECT_EQ(killXPFromTable(*twoda, 1, 9), 25);   // Past the last CR column.
	EXPECT_EQ(killXPFromTable(*twoda, 7, 1), 10);   // Past the last level row.
	EXPECT_EQ(killXPFromTable(*twoda, 2, 0), 0);    // "****" gives nothing.
	EXPECT_EQ(killXPFromTable(*twoda, 1, -3), 5);
	EXPECT_EQ(killXPFromTable(*twoda, 0, 1), -1);
}

TEST(LevelUpHelpers, KillXPTablePrefersLevelColumn) {
	std::unique_ptr<Aurora::TwoDAFile> twoda(makeTwoDA(
		"2DA V2.0\n"
		"\n"
		"   level cr0 cr1\n"
		"0  1     100 200\n"
		"1  2     50  150\n"));

	EXPECT_EQ(killXPFromTable(*twoda, 1, 1), 200);
	EXPECT_EQ(killXPFromTable(*twoda, 2, 0), 50);

	std::unique_ptr<Aurora::TwoDAFile> noCR(makeTwoDA(
		"2DA V2.0\n"
		"\n"
		"   level xp\n"
		"0  1     0\n"));
	EXPECT_EQ(killXPFromTable(*noCR, 1, 1), -1);
}

TEST(LevelUpHelpers, KillExperienceFallbackWithoutGameData) {
	EXPECT_EQ(killExperience(5, 4), 100);
	EXPECT_EQ(killExperience(1, 0), 0);
}

static const char *kFeatTable =
	"2DA V2.0\n"
	"\n"
	"   label        constant           prereqfeat1 prereqfeat2 mincharlevel minstr sol_list sol_granted sol_recom\n"
	"0  PowerAttack  FEAT_POWER_ATTACK  ****        ****        ****         ****   1        ****        2\n"
	"1  ImpPowerAtk  FEAT_IMP_PA        0           ****        3            ****   1        ****        ****\n"
	"2  MasPowerAtk  FEAT_MAS_PA        0           1           6            ****   1        ****        ****\n"
	"3  Toughness    FEAT_TOUGHNESS     ****        ****        ****         ****   0        ****        1\n"
	"4  ProfBlaster  FEAT_PROF_BLASTER  ****        ****        ****         ****   3        1           ****\n"
	"5  HeavyArmor   FEAT_HEAVY_ARMOR   ****        ****        ****         13     1        ****        ****\n"
	"6  BonusOnly    FEAT_BONUS_ONLY    ****        ****        ****         ****   2        ****        ****\n"
	"7  Unavailable  FEAT_UNAVAILABLE   ****        ****        ****         ****   4        ****        ****\n"
	"8  Unused       ****               ****        ****        ****         ****   1        ****        ****\n"
	"9  ListedLevel1 FEAT_LISTED        ****        ****        ****         ****   1        1           ****\n"
	"10 ProfMelee    FEAT_PROF_MELEE    ****        ****        ****         ****   3        2           ****\n";

static CreatureInfo soldierAtLevel(int level, uint32_t strength) {
	CreatureInfo info;
	for (int i = 0; i < level; ++i)
		info.incrementClassLevel(kClassSoldier);
	info.setAbilityScore(kAbilityStrength, strength);
	return info;
}

TEST(LevelUpHelpers, GrantedFeatsFromTableByClassLevel) {
	std::unique_ptr<Aurora::TwoDAFile> twoda(makeTwoDA(kFeatTable));

	// Only "granted" list entries, at exactly that class level.
	EXPECT_EQ(grantedFeatsFromTable(*twoda, "sol", 1), (std::vector<uint32_t>{ 4 }));
	EXPECT_EQ(grantedFeatsFromTable(*twoda, "sol", 2), (std::vector<uint32_t>{ 10 }));
	EXPECT_TRUE(grantedFeatsFromTable(*twoda, "sol", 3).empty());
	EXPECT_TRUE(grantedFeatsFromTable(*twoda, "jgd", 1).empty());
	EXPECT_TRUE(grantedFeatsFromTable(*twoda, "", 1).empty());
}

TEST(LevelUpHelpers, SelectableFeatsFromTableRecommendedFirst) {
	std::unique_ptr<Aurora::TwoDAFile> twoda(makeTwoDA(kFeatTable));

	// Level 2 soldier, STR 12, no feats: ranked feats first (Toughness, Power
	// Attack), then the rest in table order. Improved Power Attack misses its
	// prerequisite, Heavy Armor its strength; bonus-only, granted, unavailable
	// and unused rows never show.
	EXPECT_EQ(selectableFeatsFromTable(*twoda, "sol", {}, soldierAtLevel(2, 12)),
	          (std::vector<uint32_t>{ 3, 0, 9 }));

	// Knowing Power Attack at level 3 opens Improved Power Attack, not Master.
	EXPECT_EQ(selectableFeatsFromTable(*twoda, "sol", { 0 }, soldierAtLevel(3, 12)),
	          (std::vector<uint32_t>{ 3, 1, 9 }));

	// Level 2 is too low for Improved Power Attack; STR 14 allows Heavy Armor.
	EXPECT_EQ(selectableFeatsFromTable(*twoda, "sol", { 0, 3 }, soldierAtLevel(2, 14)),
	          (std::vector<uint32_t>{ 5, 9 }));

	// Master Power Attack needs both earlier feats and level 6.
	EXPECT_EQ(selectableFeatsFromTable(*twoda, "sol", { 0, 1, 3, 9 }, soldierAtLevel(6, 12)),
	          (std::vector<uint32_t>{ 2 }));

	// No such class columns: nothing.
	EXPECT_TRUE(selectableFeatsFromTable(*twoda, "jcn", {}, soldierAtLevel(2, 12)).empty());
}

TEST(LevelUpHelpers, SelectableFeatsFromTableAlternativeAndSkillRequirements) {
	std::unique_ptr<Aurora::TwoDAFile> twoda(makeTwoDA(
		"2DA V2.0\n"
		"\n"
		"   label   sol_list orreqfeat0 orreqfeat1 reqskill reqskillminranks\n"
		"0  Base1   1        ****       ****       ****     ****\n"
		"1  Base2   1        ****       ****       ****     ****\n"
		"2  Either  1        0          1          ****     ****\n"
		"3  Repairs 1        ****       ****       5        4\n"));

	CreatureInfo info = soldierAtLevel(2, 10);
	EXPECT_EQ(selectableFeatsFromTable(*twoda, "sol", {}, info), (std::vector<uint32_t>{ 0, 1 }));
	EXPECT_EQ(selectableFeatsFromTable(*twoda, "sol", { 1 }, info), (std::vector<uint32_t>{ 0, 2 }));

	info.setSkillRank(kSkillRepair, 4);
	EXPECT_EQ(selectableFeatsFromTable(*twoda, "sol", { 0, 1 }, info), (std::vector<uint32_t>{ 2, 3 }));
}

static const char *kSpellTable =
	"2DA V2.0\n"
	"\n"
	"   label       usertype guardian consular prerequisites light_recom dark_recom\n"
	"0  ****        ****     ****     ****     ****          ****        ****\n"
	"1  FORCE_HEAL  1        1        1        ****          2           ****\n"
	"2  FORCE_PUSH  1        1        1        ****          1           ****\n"
	"3  WHIRLWIND   1        5        3        2             ****        ****\n"
	"4  FORCE_WAVE  1        10       9        2_3           ****        ****\n"
	"5  DROID_ONLY  2        1        1        ****          ****        ****\n"
	"6  SHOCK       1        1        1        ****          ****        1\n"
	"7  CONSULAR    1        ****     1        ****          ****        ****\n";

/** A level 4 soldier taking the first Jedi Guardian level. */
static CreatureInfo convertedGuardian() {
	CreatureInfo info = soldierAtLevel(4, 10);
	info.incrementClassLevel(kClassJediGuardian);
	return info;
}

TEST(LevelUpHelpers, SelectableForcePowersFromTable) {
	std::unique_ptr<Aurora::TwoDAFile> twoda(makeTwoDA(kSpellTable));

	CreatureInfo info = convertedGuardian();

	// Light side: ranked by light_recom first. Whirlwind needs Force Push;
	// non-player and other-class powers never show.
	EXPECT_EQ(selectableForcePowersFromTable(*twoda, kClassJediGuardian, info),
	          (std::vector<uint32_t>{ 2, 1, 6 }));

	// Knowing Push at character level 5 opens Whirlwind, not Wave (level 10).
	info.addForcePower(2);
	EXPECT_EQ(selectableForcePowersFromTable(*twoda, kClassJediGuardian, info),
	          (std::vector<uint32_t>{ 1, 3, 6 }));

	// Dark side ranking.
	CreatureInfo dark = convertedGuardian();
	dark.setAlignment(10);
	EXPECT_EQ(selectableForcePowersFromTable(*twoda, kClassJediGuardian, dark),
	          (std::vector<uint32_t>{ 6, 1, 2 }));

	// Consulars have their own column; soldiers have none.
	EXPECT_EQ(selectableForcePowersFromTable(*twoda, kClassJediConsular, convertedGuardian()),
	          (std::vector<uint32_t>{ 2, 1, 6, 7 }));
	EXPECT_TRUE(selectableForcePowersFromTable(*twoda, kClassSoldier, info).empty());
	EXPECT_EQ(forcePowerClassColumn(kClassJediGuardian), "guardian");
	EXPECT_TRUE(forcePowerClassColumn(kClassScout).empty());
}

TEST(LevelUpHelpers, ProgressionTablesFallBackWithoutGameData) {
	EXPECT_TRUE(classFeatTableCode(kClassSoldier).empty());
	EXPECT_TRUE(classGrantedFeats(kClassSoldier, 1).empty());

	// The built-in feat list and the six core Force powers.
	CreatureInfo info;
	info.incrementClassLevel(kClassSoldier);
	EXPECT_EQ(getSelectableFeats(info), getSelectableFeats(kClassSoldier, info.getFeats()));

	CreatureInfo jedi;
	jedi.incrementClassLevel(kClassJediConsular);
	jedi.addForcePower(1);
	EXPECT_EQ(getSelectableForcePowers(jedi), (std::vector<uint32_t>{ 2, 3, 4, 5, 6 }));

	EXPECT_EQ(getFeatDisplayName(kFeatToughness), "Toughness");
}

