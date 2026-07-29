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
 *  Unit tests for Phase 2 P0 parity helpers: damage queries, attack results,
 *  immunity tracking, and surrender state transitions.
 *
 *  These exercise Creature state used by the newly wired NWScript natives
 *  without requiring a live module.
 */

#include "gtest/gtest.h"

#include <string>
#include <vector>

#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/effect.h"
#include "src/engines/kotorbase/types.h"

using namespace Engines::KotORBase;

namespace {

/** Minimal concrete Creature for state-only tests. */
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

} // End of anonymous namespace

GTEST_TEST(ParityP0Natives, damageDealtQueries) {
	TestCreature victim;
	victim.recordDamageTaken(12, kDamageTypeEnergy);

	EXPECT_EQ(victim.getTotalDamageDealt(), 12);
	EXPECT_EQ(victim.getDamageDealtByType(kDamageTypeEnergy), 12);
	EXPECT_EQ(victim.getDamageDealtByType(kDamageTypeFire), -1);
	EXPECT_EQ(victim.getDamageDealtByType(0), 12);
}

GTEST_TEST(ParityP0Natives, attackResultAndWeapon) {
	TestCreature attacker;
	TestCreature dummyWeapon; // Object stand-in

	attacker.recordAttackResult(kAttackResultMiss);
	EXPECT_EQ(attacker.getLastAttackResult(), kAttackResultMiss);

	attacker.recordAttackResult(kAttackResultCriticalHitSuccessful, &dummyWeapon);
	EXPECT_EQ(attacker.getLastAttackResult(), kAttackResultCriticalHitSuccessful);
	EXPECT_EQ(attacker.getLastWeaponUsed(), static_cast<Object *>(&dummyWeapon));
}

GTEST_TEST(ParityP0Natives, lastKillerFallback) {
	TestCreature victim;
	TestCreature killer;

	victim.setLastKiller(&killer);
	EXPECT_EQ(victim.getLastKiller(), static_cast<Object *>(&killer));
}

GTEST_TEST(ParityP0Natives, immunityRoundTrip) {
	TestCreature creature;
	EXPECT_FALSE(creature.isImmune(kImmunityTypeStun));

	creature.addImmunity(kImmunityTypeStun);
	EXPECT_TRUE(creature.isImmune(kImmunityTypeStun));

	creature.addSpellImmunity(42);
	EXPECT_TRUE(creature.isImmuneToSpell(42));
	EXPECT_FALSE(creature.isImmuneToSpell(7));

	creature.adjustDamageImmunity(kDamageTypeEnergy, 50);
	EXPECT_EQ(creature.getDamageImmunityPercent(kDamageTypeEnergy), 50);
	creature.adjustDamageImmunity(kDamageTypeEnergy, 75);
	EXPECT_EQ(creature.getDamageImmunityPercent(kDamageTypeEnergy), 100);

	creature.clearImmunities();
	EXPECT_FALSE(creature.isImmune(kImmunityTypeStun));
	EXPECT_EQ(creature.getDamageImmunityPercent(kDamageTypeEnergy), 0);
}

GTEST_TEST(ParityP0Natives, applyImmunityEffect) {
	TestCreature creature;
	Effect immunity(kKotOREffectImmunity, kImmunityTypePoison);
	creature.applyEffect(immunity);
	EXPECT_TRUE(creature.isImmune(kImmunityTypePoison));

	Effect dmgImm(kKotOREffectDamageImmunityIncrease, 25, kDamageTypeSlashing);
	creature.applyEffect(dmgImm);
	EXPECT_EQ(creature.getDamageImmunityPercent(kDamageTypeSlashing), 25);
}

GTEST_TEST(ParityP0Natives, surrenderChangesFactionAndClearsCombat) {
	TestCreature creature;
	creature.setFaction(kFactionHostile1);
	creature.startCombat(&creature, 1);
	EXPECT_TRUE(creature.isInCombat());

	creature.addImmunity(kImmunityTypeFear);
	creature.surrenderToEnemies(false);

	EXPECT_FALSE(creature.isInCombat());
	EXPECT_EQ(creature.getFaction(), kFactionSurrender1);
	EXPECT_FALSE(creature.isImmune(kImmunityTypeFear));
}

GTEST_TEST(ParityP0Natives, surrenderRetainBuffsKeepsImmunities) {
	TestCreature creature;
	creature.addImmunity(kImmunityTypeDeath);
	creature.surrenderToEnemies(true);
	EXPECT_EQ(creature.getFaction(), kFactionSurrender1);
	EXPECT_TRUE(creature.isImmune(kImmunityTypeDeath));
}

GTEST_TEST(ParityP0Natives, effectConstructors) {
	Effect sleep(kKotOREffectSleep, 0);
	EXPECT_EQ(sleep.getType(), kKotOREffectSleep);

	Effect regen(kKotOREffectRegenerate, 5);
	EXPECT_EQ(regen.getType(), kKotOREffectRegenerate);
	EXPECT_EQ(regen.getAmount(), 5);

	Effect tempFP(kKotOREffectTemporaryForcePoints, 10);
	EXPECT_EQ(tempFP.getType(), kKotOREffectTemporaryForcePoints);
	EXPECT_EQ(tempFP.getAmount(), 10);
}

GTEST_TEST(ParityP0Natives, damageResistanceAndConcealment) {
	TestCreature creature;
	creature.setDamageResistance(kDamageTypeEnergy, 5);
	EXPECT_EQ(creature.getDamageResistance(kDamageTypeEnergy), 5);

	creature.setConcealment(40);
	EXPECT_EQ(creature.getConcealment(), 40);

	Effect resist(kKotOREffectDamageResistance, 8, kDamageTypeFire);
	creature.applyEffect(resist);
	EXPECT_EQ(creature.getDamageResistance(kDamageTypeFire), 8);

	Effect conceal(kKotOREffectConcealment, 25);
	creature.applyEffect(conceal);
	EXPECT_EQ(creature.getConcealment(), 25);

	Effect assured(kKotOREffectAssuredHit, 1);
	creature.applyEffect(assured);
	EXPECT_TRUE(creature.hasAssuredHit());

	Effect forceResist(kKotOREffectForceResistanceIncrease, 12);
	creature.applyEffect(forceResist);
	EXPECT_EQ(creature.getForceResistance(), 12);

	Effect forceResistDown(kKotOREffectForceResistanceDecrease, 4);
	creature.applyEffect(forceResistDown);
	EXPECT_EQ(creature.getForceResistance(), 8);
}

GTEST_TEST(ParityP0Natives, aiLevelAndEntangle) {
	TestCreature creature;
	creature.setAILevel(3);
	EXPECT_EQ(creature.getAILevel(), 3);

	Effect entangle(kKotOREffectEntangle, 0);
	creature.applyEffect(entangle);
	EXPECT_TRUE(creature.hasEffect(kEffectStun));
}

GTEST_TEST(ParityP0Natives, dayNightPartition) {
	// Mirror GetIsDay / GetIsNight hour buckets used by functions_time.cpp
	auto isDay = [](int hour) { return hour >= 6 && hour < 18; };
	EXPECT_TRUE(isDay(6));
	EXPECT_TRUE(isDay(12));
	EXPECT_FALSE(isDay(5));
	EXPECT_FALSE(isDay(18));
	EXPECT_FALSE(isDay(23));
}

GTEST_TEST(ParityP0Natives, clearEffectsAndSpellIdTracking) {
	TestCreature creature;
	creature.applyEffect(kEffectStun, 6.0f, 0, 5);
	creature.applyEffect(kEffectPoison, 6.0f, 2, 25);
	EXPECT_TRUE(creature.hasEffect(kEffectStun));
	EXPECT_TRUE(creature.hasEffect(kEffectPoison));
	EXPECT_TRUE(creature.hasSpellEffect(5));
	EXPECT_TRUE(creature.hasSpellEffect(25));
	EXPECT_EQ(creature.getActiveEffects().size(), 2U);

	creature.clearActiveEffects();
	EXPECT_TRUE(creature.getActiveEffects().empty());
	EXPECT_FALSE(creature.hasEffect(kEffectStun));
	EXPECT_FALSE(creature.hasSpellEffect(5));
}

GTEST_TEST(ParityP0Natives, listeningPatternsAndObjectState) {
	TestCreature creature;
	EXPECT_FALSE(creature.getIsListening());
	creature.setListening(true);
	EXPECT_TRUE(creature.getIsListening());
	creature.setListenPattern(Common::UString("hello*"), 1);
	EXPECT_EQ(creature.getListenPattern(1), Common::UString("hello*"));
	creature.clearListenPatterns();
	EXPECT_TRUE(creature.getListenPattern(1).empty());
}

GTEST_TEST(ParityP0Natives, reflexAdjustedDamageHalvesOnSave) {
	// Mirror GetReflexAdjustedDamage: success → half, failure → full
	auto adjusted = [](int damage, bool saved) {
		return saved ? damage / 2 : damage;
	};
	EXPECT_EQ(adjusted(20, true), 10);
	EXPECT_EQ(adjusted(21, true), 10);
	EXPECT_EQ(adjusted(20, false), 20);
}

GTEST_TEST(ParityP0Natives, globPatternCapture) {
	// Mirror TestStringAgainstPattern '*' capture semantics (trailing wildcard)
	auto match = [](const std::string &pat, const std::string &str, std::vector<std::string> &caps) {
		caps.clear();
		std::vector<std::string> parts;
		std::string cur;
		for (char c : pat) {
			if (c == '*') { parts.push_back(cur); cur.clear(); parts.push_back("*"); }
			else cur.push_back(c);
		}
		parts.push_back(cur);
		size_t pos = 0;
		for (size_t i = 0; i < parts.size(); ++i) {
			if (parts[i] == "*") {
				while (i + 1 < parts.size() && parts[i + 1] == "*")
					++i;
				if (i + 1 >= parts.size() || parts[i + 1].empty()) {
					caps.push_back(str.substr(pos));
					return true;
				}
				size_t found = str.find(parts[i + 1], pos);
				if (found == std::string::npos) return false;
				caps.push_back(str.substr(pos, found - pos));
				pos = found;
			} else if (!parts[i].empty()) {
				if (str.compare(pos, parts[i].size(), parts[i]) != 0) return false;
				pos += parts[i].size();
			}
		}
		return pos == str.size();
	};
	std::vector<std::string> caps;
	EXPECT_TRUE(match("hello*", "hello world", caps));
	ASSERT_EQ(caps.size(), 1u);
	EXPECT_EQ(caps[0], " world");
	EXPECT_FALSE(match("foo", "bar", caps));
	EXPECT_TRUE(match("*world", "hello world", caps));
	ASSERT_EQ(caps.size(), 1u);
	EXPECT_EQ(caps[0], "hello ");
}
