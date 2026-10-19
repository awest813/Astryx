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
 *  Seeing invisible and stealthy creatures in KotOR games.
 *
 *  Only hostile eyes are fooled: a creature always sees its friends.
 *  - An invisible creature (EffectInvisibility) is unseen by those without
 *    True Seeing / See Invisibility.
 *  - A creature in stealth mode is unseen by those who fail an opposed
 *    check, Awareness against Stealth. Both sides take 10 (the check is
 *    made every time the creatures move, so rolling would only wear the
 *    stealth down), and every kStealthDistanceStep metres between them
 *    count as +1 to the stealth. True Seeing reveals the invisible, not
 *    the stealthy.
 */

#ifndef ENGINES_KOTORBASE_PERCEPTION_H
#define ENGINES_KOTORBASE_PERCEPTION_H

namespace Engines {

namespace KotORBase {

namespace PerceptionRules {

/** How far (m) creatures see and hear each other. */
extern const float kPerceptionRange;
/** Every this many metres away add one to a sneaking creature's Stealth. */
extern const float kStealthDistanceStep;

/** What a perceiver needs to know to tell whether it sees a target. */
struct SightCheck {
	bool hostile { true };        ///< Is the perceiver hostile to the target?
	bool targetInvisible { false };
	bool seesInvisible { false }; ///< Does the perceiver have True Seeing?
	bool targetStealthy { false };///< Is the target in stealth mode?
	int  targetStealth { 0 };     ///< The target's Stealth skill rank.
	int  awareness { 0 };         ///< The perceiver's Awareness skill rank.
	float distance { 0.0f };
};

/** Does the perceiver see the target (within perception range)? */
bool canSee(const SightCheck &check);

/** Does a perceiver with this Awareness spot a sneaking creature with this Stealth this far away? */
bool spotsStealthy(int awareness, int stealth, float distance);

} // End of namespace PerceptionRules

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_PERCEPTION_H
