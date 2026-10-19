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
 */

#include <cmath>

#include "src/engines/kotorbase/perception.h"

namespace Engines {

namespace KotORBase {

namespace PerceptionRules {

const float kPerceptionRange     = 16.0f;
const float kStealthDistanceStep =  4.0f;

/** Both sides of the opposed check take 10. */
static const int kTake = 10;

bool spotsStealthy(int awareness, int stealth, float distance) {
	const int distanceBonus = (distance > 0.0f) ? static_cast<int>(std::floor(distance / kStealthDistanceStep)) : 0;

	return (awareness + kTake) >= (stealth + kTake + distanceBonus);
}

bool canSee(const SightCheck &check) {
	if (check.distance > kPerceptionRange)
		return false;

	// Friends see each other, however they hide.
	if (!check.hostile)
		return true;

	if (check.targetInvisible && !check.seesInvisible)
		return false;

	if (check.targetStealthy &&
	    !spotsStealthy(check.awareness, check.targetStealth, check.distance))
		return false;

	return true;
}

} // End of namespace PerceptionRules

} // End of namespace KotORBase

} // End of namespace Engines
