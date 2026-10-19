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
 *  KotOR animation ID to model animation name mapping.
 */

#ifndef ENGINES_KOTORBASE_ANIMATIONNAMES_H
#define ENGINES_KOTORBASE_ANIMATIONNAMES_H

#include <functional>

#include "src/common/types.h"
#include "src/common/ustring.h"

namespace Engines {

namespace KotORBase {

Common::UString getAnimationNameById(int animID);

/** The model animation a script ANIMATION_* constant (ActionPlayAnimation,
 *  PlayAnimation) plays: the name in its animations.2da row, as KotOR.js maps
 *  them (ModuleObject.getAnimationNameById/animationConstantToAnimation), or
 *  the usual model animation name when animations.2da isn't there.
 *
 *  @param animation An ANIMATION_LOOPING_* (0-35) or ANIMATION_FIREFORGET_*
 *                   (100-123) constant.
 *  @param weapon    The creature's weapon animation number (for READY), -1 for none.
 *  @param simple    Is the creature a "simple" one (model type S or L: droids, beasts)?
 *  @return The animation name, or "" for an unknown constant.
 */
Common::UString getScriptAnimationName(int animation, int weapon, bool simple);

/** Is this script ANIMATION_* constant a fire-and-forget (play once) one? */
bool isFireAndForgetScriptAnimation(int animation);

/** How ActionPlayAnimation plays an animation (as KotOR.js' ActionPlayAnimation). */
struct PlayAnimationTiming {
	float modelLength { 0.0f };   ///< For Model::playAnimation(): 0 once, -1 for good, else loop that long.
	float wait { 0.0f };          ///< How long the action lasts; 0: it's done at once.
	bool backToDefault { false }; ///< Return to the default animation when the action ends?
};

/** ActionPlayAnimation(@p animation, @p speed, @p duration) of an animation
 *  @p animationLength seconds long: a fire-and-forget animation, or one asked
 *  for with no duration, plays once; a negative duration loops it until the
 *  next animation; else it loops for that many seconds. */
PlayAnimationTiming getPlayAnimationTiming(int animation, float duration, float speed,
                                           float animationLength);

/** Can a creature's model play this animation? */
typedef std::function<bool(const Common::UString &)> HasAnimation;

/** Is this weapon animation number (1 baton, 2 sword, 3 staff, 4 two swords,
 *  5 pistol, 6 two pistols, 7 rifle, 8 unarmed) a melee one? */
bool isMeleeWeaponAnimation(int weapon);

/** The attack animation a creature plays in a combat round.
 *
 *  Ranged weapons shoot ("b5a1"). In melee, a creature engaged with an
 *  opponent who fights back in melee plays one of the dueling attacks
 *  ("c2a1" to "c2a5"); otherwise one of the plain attacks ("g2a1", "g2a2").
 *
 *  @param weapon       The weapon animation number, -1 for none.
 *  @param engaged      Dueling an opponent who fights back in melee.
 *  @param roll         A random number, picking among the variants.
 *  @param hasAnimation Can the model play this animation?
 *  @return The animation, or an empty string for none.
 */
Common::UString chooseAttackAnimation(int weapon, bool engaged, uint32_t roll,
                                      const HasAnimation &hasAnimation);

/** The defensive animation of a creature attacked by @p attack (the attacker's
 *  attack animation): the parry matching a dueling attack ("c2a3" is parried
 *  by "c2p3") when wielding the same kind of weapon, a plain dodge or parry
 *  ("g2g1") otherwise. Empty for none. */
Common::UString chooseDefenseAnimation(int weapon, const Common::UString &attack,
                                       const HasAnimation &hasAnimation);

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_ANIMATIONNAMES_H
