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
 *  Effect engine type for KotOR games.
 */

#ifndef ENGINES_KOTORBASE_EFFECT_H
#define ENGINES_KOTORBASE_EFFECT_H

#include "src/aurora/nwscript/enginetype.h"

namespace Engines {

namespace KotORBase {

enum KotOREffectType {
	kKotOREffectHeal                  = 0,
	kKotOREffectDamage                = 1,
	kKotOREffectVisual                = 2,
	kKotOREffectACIncrease            = 3,
	kKotOREffectAttackIncrease        = 4,
	kKotOREffectSkillIncrease         = 5,
	kKotOREffectTemporaryHitpoints    = 6,
	kKotOREffectDeath                 = 7,  ///< Immediately kill the target.
	kKotOREffectKnockdown             = 8,  ///< Knock the target prone (stun for one round).
	kKotOREffectParalyze              = 9,  ///< Paralyse the target (Force Stasis etc.).
	kKotOREffectStunned               = 10, ///< Stun the target (interrupts actions).
	kKotOREffectHaste                 = 11, ///< Grant haste (extra attack per round).
	kKotOREffectAbilityIncrease       = 12, ///< Temporarily raise an ability score.
	kKotOREffectMovementSpeedIncrease = 13, ///< Increase movement speed by a percentage.
	kKotOREffectResurrection          = 14, ///< Revive a dead creature at 1 HP.
	kKotOREffectForcePush             = 15, ///< Push the target back (and potentially knockdown).
	kKotOREffectForceDrain            = 16, ///< Drain life/Force from target.
	kKotOREffectForceShield           = 17, ///< Shield against damage.
	kKotOREffectForcePointsHeal          = 18, ///< Restore Force points.
	kKotOREffectForcePointsDamage        = 19, ///< Drain/damage Force points.

	kKotOREffectSavingThrowIncrease      = 20, ///< Bonus to a saving throw.
	kKotOREffectDamageReduction          = 21, ///< Reduce incoming damage by a flat amount.
	kKotOREffectInvisibility             = 22, ///< Make the target invisible.
	kKotOREffectAbilityDecrease          = 23, ///< Temporarily lower an ability score.
	kKotOREffectAttackDecrease           = 24, ///< Penalty to attack rolls.
	kKotOREffectDamageDecrease           = 25, ///< Penalty to damage rolls.
	kKotOREffectDamageImmunityDecrease   = 26, ///< Reduce damage immunity by a percentage.
	kKotOREffectACDecrease               = 27, ///< Penalty to armour class.
	kKotOREffectMovementSpeedDecrease    = 28, ///< Reduce movement speed by a percentage.
	kKotOREffectSavingThrowDecrease      = 29, ///< Penalty to a saving throw.
	kKotOREffectSkillDecrease            = 30, ///< Penalty to a skill rank.
	kKotOREffectForceResistanceDecrease  = 31, ///< Reduce Force resistance.
	kKotOREffectSeeInvisible             = 32, ///< Allow the target to see invisible creatures.
	kKotOREffectPoison                   = 33, ///< Damage over time poison effect.
	kKotOREffectImmunity                 = 34, ///< Grant immunity to an ImmunityType (amount).
	kKotOREffectSpellImmunity            = 35, ///< Grant immunity to a specific spell id (amount).
	kKotOREffectDamageImmunityIncrease   = 36, ///< Raise damage immunity % (amount) for a damage type.
	kKotOREffectSleep                    = 37, ///< Put the target to sleep (stun-like).
	kKotOREffectRegenerate               = 38, ///< Heal amount HP periodically (simplified).
	kKotOREffectTemporaryForcePoints     = 39, ///< Temporary Force points (amount).
	kKotOREffectAreaOfEffect             = 40, ///< Area-of-effect marker (amount = AOE id).
	kKotOREffectDamageResistance         = 41, ///< Resist damage type (amount=limit, damageType=type).
	kKotOREffectConcealment              = 42, ///< Miss-chance percent (amount).
	kKotOREffectAssuredHit               = 43, ///< Next attack auto-hits.
	kKotOREffectAssuredDeflection        = 44, ///< Next deflection auto-succeeds.
	kKotOREffectEntangle                 = 45, ///< Entangle / immobilize (stun-like).
	kKotOREffectForceJump                = 46, ///< Force Jump visual/movement marker.
	kKotOREffectBeam                     = 47, ///< Beam VFX marker.
	kKotOREffectForceResistanceIncrease  = 48, ///< Raise Force resistance (amount).
	kKotOREffectBodyFuel                 = 49, ///< Body fuel (regen-like).
	kKotOREffectDamageIncrease           = 50, ///< Bonus damage (amount).
	kKotOREffectHitPointChangeWhenDying  = 51, ///< Dying HP change per tick (amount).
	kKotOREffectDroidStun                = 52, ///< Droid-only stun.
	kKotOREffectForceResisted            = 53, ///< Force resisted VFX marker.
	kKotOREffectForceFizzle              = 54, ///< Force fizzle VFX marker.
	kKotOREffectDispelMagicAll           = 55, ///< Dispel all magic effects.
	kKotOREffectBlasterDeflectionIncrease = 56, ///< Raise blaster deflection (amount).
	kKotOREffectBlasterDeflectionDecrease = 57  ///< Lower blaster deflection (amount).
};

class Effect : public Aurora::NWScript::EngineType {
public:
	Effect(KotOREffectType type, int amount = 0, int damageType = 0, int spellId = -1);

	KotOREffectType getType() const;
	int getAmount() const;
	int getDamageType() const;
	int getSpellId() const;
	void setSpellId(int spellId);

	Aurora::NWScript::EngineType *clone() const;

private:
	KotOREffectType _type;
	int _amount;
	int _damageType;
	int _spellId;
};

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_EFFECT_H
