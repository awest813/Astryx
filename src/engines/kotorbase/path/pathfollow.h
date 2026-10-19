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

#ifndef ENGINES_KOTORBASE_PATH_PATHFOLLOW_H
#define ENGINES_KOTORBASE_PATH_PATHFOLLOW_H

#include <functional>
#include <vector>

#include "external/glm/vec2.hpp"

namespace Engines {

namespace KotORBase {

/** A route a creature is walking: waypoints in order, the last one its goal. */
struct MovePath {
	std::vector<glm::vec2> waypoints;
	glm::vec2 goal { 0.0f };
	float age { 0.0f }; ///< Seconds since the route was computed.
	bool valid { false };

	void clear();
};

/** A smoothed walkmesh route runs exactly through wall corners. Push every
 *  intermediate waypoint @p clearance away from the corner it turns around
 *  (to the outside of the turn), if that spot is still on the walkmesh. */
void pushPathOffCorners(std::vector<glm::vec2> &path, float clearance,
                        const std::function<bool(const glm::vec2 &)> &onWalkmesh);

/** Drop the waypoints @p position has reached (within @p reach), but never
 *  the last one. Returns the waypoint to head for next. */
glm::vec2 nextWaypoint(std::vector<glm::vec2> &waypoints, const glm::vec2 &position, float reach);

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_PATH_PATHFOLLOW_H
