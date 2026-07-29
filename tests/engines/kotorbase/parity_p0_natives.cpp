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
