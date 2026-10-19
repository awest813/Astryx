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
 *  Unit tests for KotOR talents: TALENT_TYPE_* constants, 2DA categories,
 *  GetCreatureTalentBest/Random selection, Force power costs and item spells.
 */

#include <memory>
#include <vector>

#include "gtest/gtest.h"

#include "src/common/ustring.h"
#include "src/common/memreadstream.h"

#include "src/aurora/2dafile.h"

#include "src/engines/kotorbase/action.h"
#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/item.h"
#include "src/engines/kotorbase/talent.h"

using namespace Engines::KotORBase;

namespace {

class TestCreature : public Creature {
public:
	TestCreature() : Creature() {}

protected:
	void getPartModelsPC(PartModels &parts, uint32_t state, uint8_t textureVariation) override {
		(void)parts;
		(void)state;
		(void)textureVariation;
	}
};

Aurora::TwoDAFile *makeTwoDA(const char *text) {
	Common::MemoryReadStream stream(text);
	return new Aurora::TwoDAFile(stream);
}

// Categories as the game's 2DAs write them: hex bitmasks.
const char *kSpells =
	"2DA V2.0\n"
	"\n"
	"   label          forcepoints goodevil category maxcr\n"
	"0  HEAL           10          G        0x2201   3\n"
	"1  LIGHTNING      30          E        0x1101   9\n"
	"2  STUN           10          G        0x1101   4\n"
	"3  NO_AI          5           -        ****     ****\n"
	"4  PUSH           20          -        0x1101   6\n";

const char *kFeats =
	"2DA V2.0\n"
	"\n"
	"   label          category maxcr\n"
	"0  PASSIVE        ****     ****\n"
	"1  POWER_ATTACK   0x1104   2\n"
	"2  RAPID_SHOT     0x1111   2\n";

const char *kSkills =
	"2DA V2.0\n"
	"\n"
	"   label        category maxcr\n"
	"0  COMPUTER     ****     ****\n"
	"1  DEMOLITIONS  ****     ****\n"
	"2  STEALTH      ****     ****\n"
	"3  AWARENESS    ****     ****\n"
	"4  PERSUADE     ****     ****\n"
	"5  REPAIR       ****     ****\n"
	"6  SECURITY     0x0004   1\n"
	"7  TREATINJURY  0x2208   1\n";

struct Tables {
	std::unique_ptr<Aurora::TwoDAFile> spells { makeTwoDA(kSpells) };
	std::unique_ptr<Aurora::TwoDAFile> feats  { makeTwoDA(kFeats)  };
	std::unique_ptr<Aurora::TwoDAFile> skills { makeTwoDA(kSkills) };

	TalentTables get() const {
		TalentTables tables;
		tables.spells = spells.get();
		tables.feats  = feats.get();
		tables.skills = skills.get();
		return tables;
	}
};

/** A Jedi with all four powers, two combat feats, a passive feat and Security. */
void makeJedi(TestCreature &jedi, int forcePoints) {
	CreatureInfo &info = jedi.getCreatureInfo();
	for (uint32_t power = 0; power <= 4; ++power)
		info.addForcePower(power);
	info.addFeat(0);
	info.addFeat(1);
	info.addFeat(2);
	info.setSkillRank(kSkillSecurity, 4);

	jedi.setMaxForcePoints(100);
	jedi.setForcePoints(forcePoints);
}

} // End of anonymous namespace

GTEST_TEST(KotORTalent, typeConstantsMatchNWScript) {
	// nwscript.nss: TALENT_TYPE_FORCE = TALENT_TYPE_SPELL = 0, FEAT = 1, SKILL = 2.
	EXPECT_EQ(static_cast<int>(kTalentTypeSpell), 0);
	EXPECT_EQ(static_cast<int>(kTalentTypeFeat), 1);
	EXPECT_EQ(static_cast<int>(kTalentTypeSkill), 2);
	EXPECT_EQ(kTalentExcludeAllOfType, -1);

	EXPECT_TRUE(Talent(kTalentTypeSpell, 0).isValid());
	EXPECT_TRUE(Talent(kTalentTypeSkill, 6).isValid());
	EXPECT_FALSE(Talent().isValid());
	EXPECT_FALSE(Talent(kTalentTypeFeat, -1).isValid());

	Talent fromItem(kTalentTypeSpell, 4, "g_i_frarmbnds01");
	std::unique_ptr<Aurora::NWScript::EngineType> copy(fromItem.clone());
	const Talent *cloned = dynamic_cast<const Talent *>(copy.get());
	ASSERT_NE(cloned, nullptr);
	EXPECT_EQ(cloned->getType(), kTalentTypeSpell);
	EXPECT_EQ(cloned->getID(), 4);
	EXPECT_EQ(cloned->getItem(), "g_i_frarmbnds01");
}

GTEST_TEST(KotORTalent, categoriesComeFromTheTables) {
	Tables tables;

	EXPECT_EQ(getTalentCategory(*tables.spells, 1), 0x1101);
	EXPECT_EQ(getTalentCategory(*tables.spells, 3), -1);  // "****": not for the AI.
	EXPECT_EQ(getTalentCategory(*tables.spells, 99), -1); // No such row.
	EXPECT_EQ(getTalentCategory(*tables.feats, 1), 0x1104);
	EXPECT_EQ(getTalentMaxCR(*tables.spells, 1), 9);
	EXPECT_EQ(getTalentMaxCR(*tables.spells, 3), 0);

	// TALENT_CATEGORY_* arguments are bitmasks.
	EXPECT_TRUE(isTalentInCategory(0x1101, 0x1101));
	EXPECT_TRUE(isTalentInCategory(0x1111, 0x1101));
	EXPECT_FALSE(isTalentInCategory(0x1104, 0x1101));
	EXPECT_FALSE(isTalentInCategory(-1, 0));
}

GTEST_TEST(KotORTalent, creatureTalentsNeedACategoryAndForcePoints) {
	Tables tables;
	TestCreature jedi;
	makeJedi(jedi, 20);

	const std::vector<TalentEntry> talents = getCreatureTalents(jedi, tables.get());

	std::vector<int> spells, feats, skills;
	for (const TalentEntry &talent : talents) {
		if (talent.type == kTalentTypeSpell)
			spells.push_back(talent.id);
		else if (talent.type == kTalentTypeFeat)
			feats.push_back(talent.id);
		else if (talent.type == kTalentTypeSkill)
			skills.push_back(talent.id);
	}

	// 20 FP: Heal (10), Stun (10) and Push (20), not Lightning (30).
	EXPECT_EQ(spells, (std::vector<int>{ 0, 2, 4 }));
	EXPECT_EQ(feats, (std::vector<int>{ 1, 2 }));       // Not the passive feat.
	EXPECT_EQ(skills, (std::vector<int>{ kSkillSecurity })); // Only trained skills.
}

GTEST_TEST(KotORTalent, bestTalentIsClosestToMaxCR) {
	Tables tables;
	TestCreature jedi;
	makeJedi(jedi, 100);

	const std::vector<TalentEntry> talents = getCreatureTalents(jedi, tables.get());

	// Hostile Force powers: Lightning (9), Push (6), Stun (4).
	const TalentEntry *best = findBestTalent(talents, 0x1101, 20);
	ASSERT_NE(best, nullptr);
	EXPECT_EQ(best->type, kTalentTypeSpell);
	EXPECT_EQ(best->id, 1);

	best = findBestTalent(talents, 0x1101, 8);
	ASSERT_NE(best, nullptr);
	EXPECT_EQ(best->id, 4);

	// Excluding one power, then every Force power.
	best = findBestTalent(talents, 0x1101, 20, kTalentTypeSpell, 1);
	ASSERT_NE(best, nullptr);
	EXPECT_EQ(best->id, 4);

	best = findBestTalent(talents, 0x1101, 20, kTalentTypeSpell, kTalentExcludeAllOfType);
	ASSERT_NE(best, nullptr);
	EXPECT_EQ(best->type, kTalentTypeFeat); // Rapid Shot's 0x1111 covers 0x1101.
	EXPECT_EQ(best->id, 2);

	// Melee feats.
	best = findBestTalent(talents, 0x1104, 20);
	ASSERT_NE(best, nullptr);
	EXPECT_EQ(best->type, kTalentTypeFeat);
	EXPECT_EQ(best->id, 1);

	// Under every power's challenge rating only Rapid Shot (2) is left; under that, nothing.
	best = findBestTalent(talents, 0x1101, 3);
	ASSERT_NE(best, nullptr);
	EXPECT_EQ(best->type, kTalentTypeFeat);
	EXPECT_EQ(findBestTalent(talents, 0x1101, 1), nullptr);
	EXPECT_EQ(findTalentsInCategory(talents, 0x2201).size(), 1u);
	EXPECT_EQ(findTalentsInCategory(talents, 0x1101).size(), 4u);
}

GTEST_TEST(KotORTalent, forcePowerCostDependsOnAlignment) {
	// Powers of the other side of the Force cost half again as much.
	EXPECT_EQ(getForcePowerCost(20, "E", 90), 30);
	EXPECT_EQ(getForcePowerCost(20, "G", 10), 30);
	EXPECT_EQ(getForcePowerCost(20, "E", 10), 20);
	EXPECT_EQ(getForcePowerCost(20, "G", 90), 20);
	EXPECT_EQ(getForcePowerCost(20, "-", 90), 20);
	EXPECT_EQ(getForcePowerCost(20, "E", 50), 20); // Neutral.
	EXPECT_EQ(getForcePowerCost(0, "E", 90), 0);

	// A light side Jedi can't afford dark Lightning with 40 FP (45), a dark one can.
	Tables tables;
	TestCreature light;
	makeJedi(light, 40);
	light.getCreatureInfo().setAlignment(90);
	EXPECT_EQ(findBestTalent(getCreatureTalents(light, tables.get()), 0x1101, 20)->id, 4);

	TestCreature dark;
	makeJedi(dark, 40);
	dark.getCreatureInfo().setAlignment(5);
	EXPECT_EQ(findBestTalent(getCreatureTalents(dark, tables.get()), 0x1101, 20)->id, 1);
}

GTEST_TEST(KotORTalent, hasTalentAndSpellActions) {
	TestCreature jedi;
	makeJedi(jedi, 50);

	EXPECT_TRUE(creatureHasTalent(jedi, Talent(kTalentTypeSpell, 1)));
	EXPECT_FALSE(creatureHasTalent(jedi, Talent(kTalentTypeSpell, 7)));
	EXPECT_TRUE(creatureHasTalent(jedi, Talent(kTalentTypeFeat, 2)));
	EXPECT_FALSE(creatureHasTalent(jedi, Talent(kTalentTypeFeat, 5)));
	EXPECT_TRUE(creatureHasTalent(jedi, Talent(kTalentTypeSkill, kSkillSecurity)));
	EXPECT_FALSE(creatureHasTalent(jedi, Talent(kTalentTypeSkill, kSkillRepair)));
	EXPECT_FALSE(creatureHasTalent(jedi, Talent()));

	// A known power is cast normally, paying Force points.
	TestCreature target;
	const Action cast = makeTalentSpellAction(jedi, Talent(kTalentTypeSpell, 1), &target);
	EXPECT_EQ(cast.type, kActionCastSpell);
	EXPECT_EQ(cast.actionID, 1);
	EXPECT_EQ(cast.object, static_cast<Object *>(&target));
	EXPECT_FALSE(cast.cheat);
	EXPECT_TRUE(cast.item.empty());
	EXPECT_FALSE(cast.consumeItem);
}

GTEST_TEST(KotORTalent, itemSpellResolution) {
	// iprp_spells.2da style subtype table: the spells.2da row is in "spellindex".
	std::unique_ptr<Aurora::TwoDAFile> iprpSpells(makeTwoDA(
		"2DA V2.0\n"
		"\n"
		"   label        spellindex\n"
		"0  MEDPAC       100\n"
		"1  ADRENAL_STR  ****\n"
		"2  GRENADE_FRAG 113\n"));

	EXPECT_EQ(resolveCastSpellSubtype(0, iprpSpells.get()), 100);
	EXPECT_EQ(resolveCastSpellSubtype(2, iprpSpells.get()), 113);
	EXPECT_EQ(resolveCastSpellSubtype(1, iprpSpells.get()), 1);  // No index: the row itself.
	EXPECT_EQ(resolveCastSpellSubtype(7, iprpSpells.get()), 7);  // Past the table.

	// KotOR's CastSpell subtypes are spells.2da rows already.
	EXPECT_EQ(resolveCastSpellSubtype(113, nullptr), 113);

	// Without game data an item template has no spells.
	EXPECT_TRUE(Item::getConsumableSpells("no_such_item").empty());
}
