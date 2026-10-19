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
 *  Unit tests for following walkmesh routes.
 */

#include "gtest/gtest.h"

#include "external/glm/geometric.hpp"

#include "src/engines/kotorbase/path/pathfollow.h"

using namespace Engines::KotORBase;

static bool everywhere(const glm::vec2 &) {
	return true;
}

GTEST_TEST(KotORPathFollow, cornerIsPushedToTheOutsideOfTheTurn) {
	// Around a wall filling x < 0, y < 0: the route bends at its corner (0, 0).
	std::vector<glm::vec2> path = { glm::vec2(1.0f, -2.0f), glm::vec2(0.0f, 0.0f), glm::vec2(-2.0f, 1.0f) };
	pushPathOffCorners(path, 0.5f, everywhere);

	EXPECT_NEAR(glm::length(path[1]), 0.5f, 1e-4f);
	EXPECT_GT(path[1].x, 0.0f);
	EXPECT_GT(path[1].y, 0.0f);

	// The ends stay where they are.
	EXPECT_EQ(path.front(), glm::vec2(1.0f, -2.0f));
	EXPECT_EQ(path.back(), glm::vec2(-2.0f, 1.0f));
}

GTEST_TEST(KotORPathFollow, cornerStaysWhenTheOutsideIsOffTheWalkmesh) {
	std::vector<glm::vec2> path = { glm::vec2(1.0f, -2.0f), glm::vec2(0.0f, 0.0f), glm::vec2(-2.0f, 1.0f) };
	pushPathOffCorners(path, 0.5f, [](const glm::vec2 &) { return false; });

	EXPECT_EQ(path[1], glm::vec2(0.0f, 0.0f));
}

GTEST_TEST(KotORPathFollow, straightRouteIsLeftAlone) {
	std::vector<glm::vec2> path = { glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec2(2.0f, 0.0f) };
	pushPathOffCorners(path, 0.5f, everywhere);

	EXPECT_EQ(path[1], glm::vec2(1.0f, 0.0f));
}

GTEST_TEST(KotORPathFollow, reachedWaypointsAreDroppedButNotTheGoal) {
	std::vector<glm::vec2> waypoints = { glm::vec2(1.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec2(5.0f, 1.0f) };

	// Far from the first one: head for it.
	EXPECT_EQ(nextWaypoint(waypoints, glm::vec2(0.0f, 0.0f), 0.3f), glm::vec2(1.0f, 0.0f));
	EXPECT_EQ(waypoints.size(), 3U);

	// At it: on to the next.
	EXPECT_EQ(nextWaypoint(waypoints, glm::vec2(0.9f, 0.0f), 0.3f), glm::vec2(1.0f, 1.0f));
	EXPECT_EQ(waypoints.size(), 2U);

	EXPECT_EQ(nextWaypoint(waypoints, glm::vec2(1.0f, 1.1f), 0.3f), glm::vec2(5.0f, 1.0f));
	EXPECT_EQ(waypoints.size(), 1U);

	// The goal itself is never dropped.
	EXPECT_EQ(nextWaypoint(waypoints, glm::vec2(5.0f, 1.0f), 0.3f), glm::vec2(5.0f, 1.0f));
	EXPECT_EQ(waypoints.size(), 1U);
}

GTEST_TEST(KotORPathFollow, clearedRouteIsInvalid) {
	MovePath path;
	path.waypoints.push_back(glm::vec2(1.0f, 2.0f));
	path.valid = true;
	path.age   = 2.0f;

	path.clear();
	EXPECT_FALSE(path.valid);
	EXPECT_TRUE(path.waypoints.empty());
	EXPECT_EQ(path.age, 0.0f);
}
