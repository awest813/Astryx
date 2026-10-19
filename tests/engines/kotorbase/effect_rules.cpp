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
 *  Unit tests for KotOR effect rules: linked effects, timed modifier effects
 *  and how they fold into the creature's ability, skill, save, AC and attack
 *  values, Force point effects and damage absorption.
 */

#include <memory>

#include "gtest/gtest.h"

#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/effect.h"
#include "src/engines/kotorbase/types.h"

using namespace Engines::KotORBase;

namespace {

class TestCreature : public Creature {
public:
	TestCreature() : Creature() {
		// Average abilities: no modifiers.
		for (int ability = kAbilityStrength; ability <= kAbilityCharisma; ++ability)
			getCreatureInfo().setAbilityScore(static_cast<Ability>(ability), 10);
	}

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

// nwscript.nss SAVING_THROW_*.
const int kNWSavingThrowAll  = 0;
const int kNWSavingThrowWill = 3;

/** EffectLinkEffects(EffectACIncrease(2), EffectLinkEffects(EffectAttackIncrease(1), EffectVisualEffect(1000))). */
Effect makeBuffLink(int spellId) {
	Effect link = linkEffects(Effect(kKotOREffectACIncrease, 2),
	                          linkEffects(Effect(kKotOREffectAttackIncrease, 1),
	                                      Effect(kKotOREffectVisual, 1000)));
	link.setSpellId(spellId);
	return link;
}

} // End of anonymous namespace

GTEST_TEST(KotOREffectRules, linkFlattensAndCarriesTheSpell) {
	const Effect link = makeBuffLink(9);

	// The link is the parent; the children are flattened behind it.
	EXPECT_EQ(link.getType(), kKotOREffectVisual);
	ASSERT_EQ(link.getLinkedEffects().size(), 2u);
	EXPECT_EQ(link.getLinkedEffects()[0].getType(), kKotOREffectAttackIncrease);
	EXPECT_EQ(link.getLinkedEffects()[1].getType(), kKotOREffectACIncrease);
	EXPECT_TRUE(link.hasPart(kKotOREffectACIncrease));
	EXPECT_FALSE(link.hasPart(kKotOREffectDeath));

	for (const Effect &part : link.getLinkedEffects())
		EXPECT_EQ(part.getSpellId(), 9);

	std::unique_ptr<Aurora::NWScript::EngineType> copy(link.clone());
	const Effect *cloned = dynamic_cast<const Effect *>(copy.get());
	ASSERT_NE(cloned, nullptr);
	EXPECT_EQ(cloned->getLinkedEffects().size(), 2u);
}

GTEST_TEST(KotOREffectRules, linkedModifiersApplyAndExpireTogether) {
	TestCreature creature;
	const int baseAC = creature.getAC();

	creature.applyEffect(makeBuffLink(9), 10.0f);

	EXPECT_EQ(creature.getAC(), baseAC + 2);
	EXPECT_EQ(creature.getAttackModifier(), 1);
	ASSERT_EQ(creature.getActiveEffects().size(), 2u); // The visual leaves nothing behind.
	EXPECT_NE(creature.getActiveEffects()[0].linkId, 0);
	EXPECT_EQ(creature.getActiveEffects()[0].linkId, creature.getActiveEffects()[1].linkId);
	EXPECT_TRUE(creature.hasSpellEffect(9));

	creature.updateEffects(5.0f);
	EXPECT_EQ(creature.getAC(), baseAC + 2);

	creature.updateEffects(6.0f);
	EXPECT_EQ(creature.getAC(), baseAC);
	EXPECT_EQ(creature.getAttackModifier(), 0);
	EXPECT_TRUE(creature.getActiveEffects().empty());
	EXPECT_FALSE(creature.hasSpellEffect(9));
}

GTEST_TEST(KotOREffectRules, removingOnePartRemovesTheLink) {
	TestCreature creature;
	creature.applyEffect(makeBuffLink(9));      // No duration: permanent.
	creature.applyEffect(kEffectStun, 10.0f);   // Unrelated, -2 AC.
	ASSERT_EQ(creature.getActiveEffects().size(), 3u);
	EXPECT_TRUE(creature.getActiveEffects()[0].permanent);

	EXPECT_TRUE(creature.removeActiveEffectAt(0));
	ASSERT_EQ(creature.getActiveEffects().size(), 1u);
	EXPECT_EQ(creature.getActiveEffects()[0].type, kEffectStun);
	EXPECT_EQ(creature.getAttackModifier(), 0);
	EXPECT_EQ(creature.getArmorClassModifier(), -2);
}

GTEST_TEST(KotOREffectRules, abilityEffectsFoldIntoGetters) {
	TestCreature creature;
	creature.getCreatureInfo().setAbilityScore(kAbilityDexterity, 12);
	const int baseWill = creature.getSavingThrowBonus(kSavingThrowWill);
	const int baseAC   = creature.getAC();

	creature.applyEffect(Effect(kKotOREffectAbilityIncrease, 4, kAbilityStrength), 30.0f);
	EXPECT_EQ(creature.getAbilityScore(kAbilityStrength), 14);
	EXPECT_EQ(creature.getAbilityModifier(kAbilityStrength), 2);
	EXPECT_EQ(creature.getCreatureInfo().getAbilityScore(kAbilityStrength), 10); // Base stays.

	// Penalties stop at 3; Dexterity drives AC.
	creature.applyEffect(Effect(kKotOREffectAbilityDecrease, 20, kAbilityDexterity), 30.0f);
	EXPECT_EQ(creature.getAbilityScore(kAbilityDexterity), 3);
	EXPECT_EQ(creature.getAbilityModifier(kAbilityDexterity), -4);
	EXPECT_EQ(creature.getAC(), baseAC - 5);

	// Wisdom drives the Will save.
	creature.applyEffect(Effect(kKotOREffectAbilityIncrease, 4, kAbilityWisdom), 30.0f);
	EXPECT_EQ(creature.getSavingThrowBonus(kSavingThrowWill), baseWill + 2);

	creature.clearActiveEffects();
	EXPECT_EQ(creature.getAbilityScore(kAbilityStrength), 10);
	EXPECT_EQ(creature.getAbilityScore(kAbilityDexterity), 12);
	EXPECT_EQ(creature.getSavingThrowBonus(kSavingThrowWill), baseWill);
	EXPECT_EQ(creature.getAC(), baseAC);
}

GTEST_TEST(KotOREffectRules, skillSaveACAndAttackEffects) {
	TestCreature creature;
	creature.getCreatureInfo().setSkillRank(kSkillAwareness, 4);
	EXPECT_EQ(creature.getSkillRank(kSkillAwareness), 4);

	creature.applyEffect(Effect(kKotOREffectSkillIncrease, 5, kSkillAwareness), 30.0f);
	EXPECT_EQ(creature.getSkillRank(kSkillAwareness), 9);
	creature.applyEffect(Effect(kKotOREffectSkillDecrease, 20, kSkillAwareness), 30.0f);
	EXPECT_EQ(creature.getSkillRank(kSkillAwareness), 0);

	const int will = creature.getSavingThrowBonus(kSavingThrowWill);
	const int fort = creature.getSavingThrowBonus(kSavingThrowFortitude);
	creature.applyEffect(Effect(kKotOREffectSavingThrowIncrease, 3, kNWSavingThrowWill), 30.0f);
	creature.applyEffect(Effect(kKotOREffectSavingThrowIncrease, 1, kNWSavingThrowAll), 30.0f);
	EXPECT_EQ(creature.getSavingThrowBonus(kSavingThrowWill), will + 4);
	EXPECT_EQ(creature.getSavingThrowBonus(kSavingThrowFortitude), fort + 1);
	creature.applyEffect(Effect(kKotOREffectSavingThrowDecrease, 2, kNWSavingThrowAll), 30.0f);
	EXPECT_EQ(creature.getSavingThrowBonus(kSavingThrowFortitude), fort - 1);

	const int ac = creature.getAC();
	creature.applyEffect(Effect(kKotOREffectACDecrease, 2), 30.0f);
	creature.applyEffect(Effect(kKotOREffectAttackDecrease, 3), 30.0f);
	creature.applyEffect(Effect(kKotOREffectDamageIncrease, 4), 30.0f);
	EXPECT_EQ(creature.getAC(), ac - 2);
	EXPECT_EQ(creature.getAttackModifier(), -3);
	EXPECT_EQ(creature.getDamageModifier(), 4);

	creature.updateEffects(31.0f);
	EXPECT_EQ(creature.getSkillRank(kSkillAwareness), 4);
	EXPECT_EQ(creature.getSavingThrowBonus(kSavingThrowWill), will);
	EXPECT_EQ(creature.getAC(), ac);
	EXPECT_EQ(creature.getAttackModifier(), 0);
	EXPECT_EQ(creature.getDamageModifier(), 0);
}

GTEST_TEST(KotOREffectRules, instantModifiersDoNothing) {
	TestCreature creature;
	creature.applyEffect(Effect(kKotOREffectACIncrease, 5), 0.0f);
	EXPECT_EQ(creature.getArmorClassModifier(), 0);
	EXPECT_TRUE(creature.getActiveEffects().empty());
}

GTEST_TEST(KotOREffectRules, forcePointEffects) {
	TestCreature creature;
	creature.setMaxForcePoints(30);
	creature.setForcePoints(10);

	creature.applyEffect(Effect(kKotOREffectForcePointsHeal, 15), 0.0f);
	EXPECT_EQ(creature.getForcePoints(), 25);
	creature.applyEffect(Effect(kKotOREffectForcePointsHeal, 50), 0.0f);
	EXPECT_EQ(creature.getForcePoints(), 30);
	creature.applyEffect(Effect(kKotOREffectForcePointsDamage, 12), 0.0f);
	EXPECT_EQ(creature.getForcePoints(), 18);
	creature.applyEffect(Effect(kKotOREffectForcePointsDamage, 100), 0.0f);
	EXPECT_EQ(creature.getForcePoints(), 0);
}

GTEST_TEST(KotOREffectRules, forceShieldAbsorbsDamageUntilUsedUp) {
	TestCreature creature;
	setHP(creature, 50);

	// Without forceshields.2da, shield row 0 soaks 10 damage of any type.
	creature.applyEffect(Effect(kKotOREffectForceShield, 0), 30.0f);
	EXPECT_EQ(creature.getCurrentHitPoints(), 50); // Not temporary hit points.

	creature.applyEffect(Effect(kKotOREffectDamage, 6, kDamageTypeEnergy), 0.0f);
	EXPECT_EQ(creature.getCurrentHitPoints(), 50);
	creature.applyEffect(Effect(kKotOREffectDamage, 6, kDamageTypeEnergy), 0.0f);
	EXPECT_EQ(creature.getCurrentHitPoints(), 48);

	creature.updateEffects(0.1f);
	EXPECT_TRUE(creature.getActiveEffects().empty());
	creature.applyEffect(Effect(kKotOREffectDamage, 6, kDamageTypeEnergy), 0.0f);
	EXPECT_EQ(creature.getCurrentHitPoints(), 42);
}

GTEST_TEST(KotOREffectRules, damageResistanceAndReduction) {
	TestCreature creature;

	// 5 per hit, 8 in total, energy only.
	creature.applyEffect(Effect(kKotOREffectDamageResistance, 5, kDamageTypeEnergy, -1, 8), 30.0f);
	EXPECT_EQ(creature.getDamageResistance(kDamageTypeEnergy), 5);
	EXPECT_EQ(creature.absorbDamage(7, kDamageTypeFire), 7);
	EXPECT_EQ(creature.absorbDamage(7, kDamageTypeEnergy), 2);
	EXPECT_EQ(creature.absorbDamage(7, kDamageTypeEnergy), 4);
	EXPECT_EQ(creature.absorbDamage(7, kDamageTypeEnergy), 7);
	EXPECT_EQ(creature.getDamageResistance(kDamageTypeEnergy), 0);

	// Damage reduction stops physical damage only.
	creature.applyEffect(Effect(kKotOREffectDamageReduction, 3), 30.0f);
	EXPECT_EQ(creature.absorbDamage(5, kDamageTypeSlashing), 2);
	EXPECT_EQ(creature.absorbDamage(5, kDamageTypeEnergy), 5);
	EXPECT_EQ(creature.absorbDamage(2, kDamageTypePiercing), 0);
}

GTEST_TEST(KotOREffectRules, invisibilityLastsItsDuration) {
	TestCreature creature;
	creature.applyEffect(Effect(kKotOREffectInvisibility, 1), 6.0f);
	EXPECT_TRUE(creature.isInvisible());
	creature.updateEffects(7.0f);
	EXPECT_FALSE(creature.isInvisible());
}

GTEST_TEST(KotOREffectRules, combatFeatModifiersForEveryTier) {
	// feat.2da rows (nwscript FEAT_*).
	EXPECT_EQ(static_cast<int>(kFeatPowerBlast), 29);
	EXPECT_EQ(static_cast<int>(kFeatSniperShot), 31);
	EXPECT_EQ(static_cast<int>(kFeatMasterPowerBlast), 82);
	EXPECT_EQ(static_cast<int>(kFeatMasterSniperShot), 77);

	// Power Blast: ranged, -3 attack, +5/+8/+10 damage.
	const int powerBlast[] = { kFeatPowerBlast, kFeatImprovedPowerBlast, kFeatMasterPowerBlast };
	const int blastDamage[] = { 5, 8, 10 };
	for (int i = 0; i < 3; ++i) {
		const CombatFeatModifiers mods = getCombatFeatModifiers(powerBlast[i]);
		EXPECT_TRUE(mods.ranged);
		EXPECT_FALSE(mods.melee);
		EXPECT_EQ(mods.attack, -3);
		EXPECT_EQ(mods.damage, blastDamage[i]);
		EXPECT_EQ(mods.extraAttacks, 0);
	}

	// Sniper Shot: ranged critical threat range +1/+2/+3.
	const int sniperShot[] = { kFeatSniperShot, kFeatImprovedSniperShot, kFeatMasterSniperShot };
	for (int i = 0; i < 3; ++i) {
		const CombatFeatModifiers mods = getCombatFeatModifiers(sniperShot[i]);
		EXPECT_TRUE(mods.ranged);
		EXPECT_EQ(mods.threatRange, i + 1);
		EXPECT_EQ(mods.attack, 0);
	}

	// Power Attack mirrors Power Blast in melee; Critical Strike costs 5 Defense.
	EXPECT_TRUE(getCombatFeatModifiers(kFeatMasterPowerAttack).melee);
	EXPECT_EQ(getCombatFeatModifiers(kFeatMasterPowerAttack).damage, 10);
	EXPECT_EQ(getCombatFeatModifiers(kFeatMasterCriticalStrike).threatRange, 3);
	EXPECT_EQ(getCombatFeatModifiers(kFeatCriticalStrike).armorClass, -5);

	// Flurry / Rapid Shot: one extra attack, -4/-2/-1 attack and Defense.
	EXPECT_EQ(getCombatFeatModifiers(kFeatRapidShot).extraAttacks, 1);
	EXPECT_EQ(getCombatFeatModifiers(kFeatRapidShot).attack, -4);
	EXPECT_EQ(getCombatFeatModifiers(kFeatImprovedRapidShot).armorClass, -2);
	EXPECT_EQ(getCombatFeatModifiers(kFeatMasterFlurry).attack, -1);
	EXPECT_TRUE(getCombatFeatModifiers(kFeatMasterFlurry).melee);

	// Other feats do nothing in combat.
	const CombatFeatModifiers none = getCombatFeatModifiers(kFeatToughness);
	EXPECT_FALSE(none.melee || none.ranged);
	EXPECT_EQ(getCombatFeatModifiers(-1).damage, 0);
}

GTEST_TEST(KotOREffectRules, combatFeatDefensePenaltyLastsARound) {
	TestCreature creature;
	const int ac = creature.getAC();

	// What the combat round applies for an activated Critical Strike.
	const CombatFeatModifiers mods = getCombatFeatModifiers(kFeatCriticalStrike);
	creature.applyEffect(Effect(kKotOREffectACDecrease, -mods.armorClass), kCombatFeatPenaltyDuration);
	EXPECT_EQ(creature.getAC(), ac - 5);

	creature.updateEffects(kCombatFeatPenaltyDuration + 0.1f);
	EXPECT_EQ(creature.getAC(), ac);
}
