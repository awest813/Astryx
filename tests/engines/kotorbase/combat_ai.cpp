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
 *  Unit tests for combat / AI state fixes on Creature: PC faction, EffectDeath,
 *  combat round bookkeeping, surrender, poison ticks, stuns, queued events.
 */

#include "gtest/gtest.h"

#include <vector>

#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/effect.h"
#include "src/engines/kotorbase/types.h"

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

void setHP(Creature &c, int hp) {
	c.setMaxHitPoints(hp);
	c.setCurrentHitPoints(hp);
}

} // End of anonymous namespace

GTEST_TEST(KotORCombatAI, fakePCIsFriendly) {
	TestCreature pc;
	pc.initAsFakePC();
	// repute.2da row 0, like the original's player templates.
	EXPECT_EQ(pc.getFaction(), kFactionPlayer);
	EXPECT_FALSE(pc.isEnemy());
}

GTEST_TEST(KotORCombatAI, effectDeathKills) {
	TestCreature victim, attacker;
	setHP(victim, 30);
	victim.recordDamageTaken(1, kDamageTypeEnergy, &attacker);

	victim.applyEffect(Effect(kKotOREffectDeath));
	EXPECT_TRUE(victim.isDead());
	EXPECT_EQ(victim.getCurrentHitPoints(), 0);
	EXPECT_EQ(victim.getLastKiller(), static_cast<Object *>(&attacker));
	EXPECT_TRUE(victim.consumeDeathEvent());
}

GTEST_TEST(KotORCombatAI, effectDeathRespectsPlotImmunityAndMin1HP) {
	TestCreature plot;
	setHP(plot, 30);
	plot.setPlotFlag(true);
	plot.applyEffect(Effect(kKotOREffectDeath));
	EXPECT_FALSE(plot.isDead());
	EXPECT_EQ(plot.getCurrentHitPoints(), 30);

	TestCreature immune;
	setHP(immune, 30);
	immune.addImmunity(kImmunityTypeDeath);
	immune.applyEffect(Effect(kKotOREffectDeath));
	EXPECT_FALSE(immune.isDead());

	TestCreature min1;
	setHP(min1, 30);
	min1.setMinOneHitPoints(true);
	min1.applyEffect(Effect(kKotOREffectDeath));
	EXPECT_FALSE(min1.isDead());
	EXPECT_EQ(min1.getCurrentHitPoints(), 1);
}

GTEST_TEST(KotORCombatAI, restartingCombatKeepsScheduledAttack) {
	TestCreature attacker, target, other;

	attacker.startCombat(&target, 0);
	attacker.setAttemptedAttackTarget(&target); // notifyCombatRoundBegan
	attacker.startCombat(&target, 0);           // re-engaging the same target
	EXPECT_EQ(attacker.getAttemptedAttackTarget(), static_cast<Object *>(&target));

	attacker.startCombat(&other, 0);            // switching targets
	EXPECT_EQ(attacker.getAttemptedAttackTarget(), nullptr);
	EXPECT_EQ(attacker.getAttackTarget(), static_cast<Object *>(&other));
}

GTEST_TEST(KotORCombatAI, stopAttacksOnCancelsOnlyAttackersOfTarget) {
	TestCreature victim, attacker, bystander, otherTarget;
	attacker.startCombat(&victim, 0);
	attacker.setAttemptedAttackTarget(&victim);
	attacker.addAction(Action(kActionWait));
	bystander.startCombat(&otherTarget, 0);
	bystander.addAction(Action(kActionWait));

	std::vector<Creature *> creatures = { &victim, &attacker, &bystander, &otherTarget };
	Creature::stopAttacksOn(creatures, &victim, true);

	EXPECT_FALSE(attacker.isInCombat());
	EXPECT_EQ(attacker.getAttemptedAttackTarget(), nullptr);
	EXPECT_EQ(attacker.getActionCount(), 0u);

	EXPECT_TRUE(bystander.isInCombat());
	EXPECT_EQ(bystander.getActionCount(), 1u);
}

GTEST_TEST(KotORCombatAI, surrenderReversesEffectModifiers) {
	TestCreature creature;
	const int baseAC = creature.getArmorClassModifier();
	creature.applyEffect(kEffectStun, 10.0f);
	EXPECT_EQ(creature.getArmorClassModifier(), baseAC - 2);

	creature.surrenderToEnemies(false);
	EXPECT_TRUE(creature.getActiveEffects().empty());
	EXPECT_EQ(creature.getArmorClassModifier(), baseAC);
}

GTEST_TEST(KotORCombatAI, poisonTicksEverySecond) {
	TestCreature creature, poisoner;
	setHP(creature, 50);
	creature.recordDamageTaken(0, 0, &poisoner);

	creature.applyEffect(kEffectPoison, 10.0f, 3);
	// Small frame times used to truncate every tick to 0 damage.
	for (int i = 0; i < 100; ++i)
		creature.updateEffects(0.016f);
	EXPECT_EQ(creature.getCurrentHitPoints(), 47);
	EXPECT_EQ(creature.getTotalDamageDealt(), 3);
	EXPECT_TRUE(creature.consumeDamagedEvent());

	// Poison doesn't kill.
	for (int i = 0; i < 40; ++i)
		creature.updateEffects(0.25f);
	EXPECT_GE(creature.getCurrentHitPoints(), 1);
	EXPECT_FALSE(creature.isDead());
}

GTEST_TEST(KotORCombatAI, stunIncapacitatesUntilExpired) {
	TestCreature creature;
	EXPECT_FALSE(creature.isIncapacitated());
	creature.applyEffect(Effect(kKotOREffectStunned), 2.0f);
	EXPECT_TRUE(creature.isIncapacitated());
	creature.updateEffects(2.5f);
	EXPECT_FALSE(creature.isIncapacitated());

	creature.applyEffect(Effect(kKotOREffectKnockdown), 1.0f);
	EXPECT_TRUE(creature.isIncapacitated());
}

GTEST_TEST(KotORCombatAI, perceptionEventsOnlyOnChange) {
	TestCreature a, b;
	a.setPosition(0.0f, 0.0f, 0.0f);
	b.setPosition(3.0f, 0.0f, 0.0f);

	a.updatePerception(b);
	a.updatePerception(b); // no change, no new event

	Creature::PerceptionEvent event;
	ASSERT_TRUE(a.consumePerceptionEvent(event));
	EXPECT_EQ(event.object, static_cast<Object *>(&b));
	EXPECT_TRUE(event.seen);
	EXPECT_EQ(a.getLastPerceived(), static_cast<Object *>(&b));
	EXPECT_TRUE(a.getLastPerceptionSeen());
	EXPECT_FALSE(a.consumePerceptionEvent(event));

	ASSERT_TRUE(b.consumePerceptionEvent(event));
	EXPECT_EQ(event.object, static_cast<Object *>(&a));

	b.setPosition(100.0f, 0.0f, 0.0f);
	a.updatePerception(b);
	ASSERT_TRUE(a.consumePerceptionEvent(event));
	EXPECT_FALSE(event.seen);
	EXPECT_TRUE(a.getLastPerceptionVanished());

	// A destroyed object must not linger in the queue.
	b.setPosition(1.0f, 0.0f, 0.0f);
	a.updatePerception(b);
	a.forgetObject(&b);
	EXPECT_FALSE(a.consumePerceptionEvent(event));
}

GTEST_TEST(KotORCombatAI, attackQueuesOnAttackedAndMin1HPSurvives) {
	TestCreature attacker, target;
	setHP(attacker, 20);
	setHP(target, 1);
	target.setMinOneHitPoints(true);

	attacker.startCombat(&target, 0);
	attacker.setAssuredHit(true);
	attacker.executeAttack(&target);

	Object *who = nullptr;
	EXPECT_TRUE(target.consumeAttackedEvent(who));
	EXPECT_EQ(who, static_cast<Object *>(&attacker));
	EXPECT_FALSE(target.consumeAttackedEvent(who));

	// Still alive at 1 HP: no kill bookkeeping.
	EXPECT_FALSE(target.isDead());
	EXPECT_EQ(target.getCurrentHitPoints(), 1);
	EXPECT_EQ(target.getLastKiller(), nullptr);
	EXPECT_TRUE(attacker.isInCombat());
}

GTEST_TEST(KotORCombatAI, endRoundEventIsQueuedOnce) {
	TestCreature creature;
	EXPECT_FALSE(creature.consumeEndRoundEvent());
	creature.queueEndRoundEvent();
	EXPECT_TRUE(creature.consumeEndRoundEvent());
	EXPECT_FALSE(creature.consumeEndRoundEvent());
}

GTEST_TEST(KotORCombatAI, forceRegenerationAndRest) {
	TestCreature jedi;
	setHP(jedi, 40);
	jedi.setMaxForcePoints(40);
	jedi.setForcePoints(10);

	jedi.regenerateForcePoints();
	EXPECT_EQ(jedi.getForcePoints(), 12);

	// No regeneration in combat.
	TestCreature enemy;
	jedi.startCombat(&enemy, 0);
	jedi.regenerateForcePoints();
	EXPECT_EQ(jedi.getForcePoints(), 12);
	jedi.cancelCombat();

	jedi.setCurrentHitPoints(5);
	jedi.restoreFully();
	EXPECT_EQ(jedi.getCurrentHitPoints(), 40);
	EXPECT_EQ(jedi.getForcePoints(), 40);
}
