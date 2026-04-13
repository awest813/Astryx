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

enum EffectType {
	kEffectHeal                  = 0,
	kEffectDamage                = 1,
	kEffectVisual                = 2,
	kEffectACIncrease            = 3,
	kEffectAttackIncrease        = 4,
	kEffectSkillIncrease         = 5,
	kEffectTemporaryHitpoints    = 6,
	kEffectDeath                 = 7,  ///< Immediately kill the target.
	kEffectKnockdown             = 8,  ///< Knock the target prone (stun for one round).
	kEffectParalyze              = 9,  ///< Paralyse the target (Force Stasis etc.).
	kEffectStunned               = 10, ///< Stun the target (interrupts actions).
	kEffectHaste                 = 11, ///< Grant haste (extra attack per round).
	kEffectAbilityIncrease       = 12, ///< Temporarily raise an ability score.
	kEffectMovementSpeedIncrease = 13, ///< Increase movement speed by a percentage.
	kEffectResurrection          = 14, ///< Revive a dead creature at 1 HP.
	kEffectForcePush             = 15, ///< Push the target back (and potentially knockdown).
	kEffectForceDrain            = 16, ///< Drain life/Force from target.
	kEffectForceShield           = 17, ///< Shield against damage.
	kEffectForcePointsHeal          = 18, ///< Restore Force points.
	kEffectForcePointsDamage        = 19, ///< Drain/damage Force points.

	// Debuff / decrease effects (Mirror images of the increase variants above)
	kEffectSavingThrowIncrease      = 20, ///< Bonus to a saving throw.
	kEffectDamageReduction          = 21, ///< Reduce incoming damage by a flat amount.
	kEffectInvisibility             = 22, ///< Make the target invisible.
	kEffectAbilityDecrease          = 23, ///< Temporarily lower an ability score.
	kEffectAttackDecrease           = 24, ///< Penalty to attack rolls.
	kEffectDamageDecrease           = 25, ///< Penalty to damage rolls.
	kEffectDamageImmunityDecrease   = 26, ///< Reduce damage immunity by a percentage.
	kEffectACDecrease               = 27, ///< Penalty to armour class.
	kEffectMovementSpeedDecrease    = 28, ///< Reduce movement speed by a percentage.
	kEffectSavingThrowDecrease      = 29, ///< Penalty to a saving throw.
	kEffectSkillDecrease            = 30, ///< Penalty to a skill rank.
	kEffectForceResistanceDecrease  = 31  ///< Reduce Force resistance.
};

class Effect : public Aurora::NWScript::EngineType {
public:
	Effect(EffectType type, int amount = 0, int damageType = 0);

	EffectType getType() const;
	int getAmount() const;
	int getDamageType() const;

	Aurora::NWScript::EngineType *clone() const;

private:
	EffectType _type;
	int _amount;
	int _damageType;
};

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_EFFECT_H
