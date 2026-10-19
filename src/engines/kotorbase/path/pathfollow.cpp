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
 *  Following a walkmesh route: waypoint helpers shared by creature movement.
 */

#include "external/glm/geometric.hpp"

#include "src/engines/kotorbase/path/pathfollow.h"

namespace Engines {

namespace KotORBase {

void MovePath::clear() {
	waypoints.clear();
	age   = 0.0f;
	valid = false;
}

void pushPathOffCorners(std::vector<glm::vec2> &path, float clearance,
                        const std::function<bool(const glm::vec2 &)> &onWalkmesh) {

	if (path.size() < 3)
		return;

	const std::vector<glm::vec2> original = path;
	for (size_t i = 1; i + 1 < original.size(); i++) {
		const glm::vec2 &corner = original[i];

		const glm::vec2 toPrev = original[i - 1] - corner;
		const glm::vec2 toNext = original[i + 1] - corner;
		if (glm::length(toPrev) < 0.001f || glm::length(toNext) < 0.001f)
			continue;

		// The bisector of the turn points at the obstacle the route bends around.
		const glm::vec2 inward = glm::normalize(toPrev) + glm::normalize(toNext);
		if (glm::length(inward) < 0.001f)
			continue; // Straight on: nothing to bend around.

		const glm::vec2 moved = corner - glm::normalize(inward) * clearance;
		if (onWalkmesh(moved))
			path[i] = moved;
	}
}

glm::vec2 nextWaypoint(std::vector<glm::vec2> &waypoints, const glm::vec2 &position, float reach) {
	while (waypoints.size() > 1 && glm::length(waypoints.front() - position) <= reach)
		waypoints.erase(waypoints.begin());

	return waypoints.empty() ? position : waypoints.front();
}

} // End of namespace KotORBase

} // End of namespace Engines
