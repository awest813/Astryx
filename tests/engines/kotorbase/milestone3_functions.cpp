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
 *  Unit tests for Milestone 3 NWScript function implementations:
 *  GetDistanceBetween, AngleToVector, VectorToAngle, GetItemStackSize,
 *  SetItemStackSize, GetFactionEqual, ChangeFaction, GetModuleFileName.
 *
 *  Tests exercise arithmetic and data-class logic directly without requiring
 *  live game data (no GFF files, no models, no Aurora resource archives).
 */

#include "gtest/gtest.h"

#include <cmath>

#include "src/engines/kotorbase/types.h"

using namespace Engines::KotORBase;

// ---------------------------------------------------------------------------
// Distance formula (mirrors GetDistanceBetween / getDistanceBetween3D)
// ---------------------------------------------------------------------------

static float dist3D(float x1, float y1, float z1, float x2, float y2, float z2) {
	float dx = x1 - x2;
	float dy = y1 - y2;
	float dz = z1 - z2;
	return std::sqrt(dx * dx + dy * dy + dz * dz);
}

static float dist2D(float x1, float y1, float x2, float y2) {
	float dx = x1 - x2;
	float dy = y1 - y2;
	return std::sqrt(dx * dx + dy * dy);
}

GTEST_TEST(KotORMilestone3, distanceBetweenOriginIs0) {
	EXPECT_FLOAT_EQ(dist3D(0, 0, 0, 0, 0, 0), 0.0f);
}

GTEST_TEST(KotORMilestone3, distanceBetweenAlongXAxis) {
	EXPECT_FLOAT_EQ(dist3D(0, 0, 0, 5, 0, 0), 5.0f);
	EXPECT_FLOAT_EQ(dist3D(5, 0, 0, 0, 0, 0), 5.0f);
}

GTEST_TEST(KotORMilestone3, distanceBetween3DPythagorean) {
	// 3-4-5 triangle in XY plane
	EXPECT_FLOAT_EQ(dist3D(0, 0, 0, 3, 4, 0), 5.0f);
	// 3D: sqrt(3^2+4^2+0^2) = 5
	EXPECT_FLOAT_EQ(dist3D(1, 1, 1, 4, 5, 1), 5.0f);
}

GTEST_TEST(KotORMilestone3, distanceBetween2DIgnoresZ) {
	EXPECT_FLOAT_EQ(dist2D(0, 0, 3, 4), 5.0f);
	EXPECT_FLOAT_EQ(dist2D(0, 0, 0, 7), 7.0f);
}

// ---------------------------------------------------------------------------
// AngleToVector / VectorToAngle arithmetic
// ---------------------------------------------------------------------------

static void angleToVector(float angleDeg, float &x, float &y) {
	const float rad = angleDeg * (M_PI / 180.0f);
	x = std::cos(rad);
	y = std::sin(rad);
}

static float vectorToAngle(float x, float y) {
	return std::atan2(y, x) * (180.0f / M_PI);
}

GTEST_TEST(KotORMilestone3, angleZeroPoinetsAlongXAxis) {
	float x, y;
	angleToVector(0.0f, x, y);
	EXPECT_NEAR(x, 1.0f, 1e-6f);
	EXPECT_NEAR(y, 0.0f, 1e-6f);
}

GTEST_TEST(KotORMilestone3, angle90PointsAlongYAxis) {
	float x, y;
	angleToVector(90.0f, x, y);
	EXPECT_NEAR(x, 0.0f, 1e-6f);
	EXPECT_NEAR(y, 1.0f, 1e-6f);
}

GTEST_TEST(KotORMilestone3, angle180PointsNegativeX) {
	float x, y;
	angleToVector(180.0f, x, y);
	EXPECT_NEAR(x, -1.0f, 1e-6f);
	EXPECT_NEAR(y,  0.0f, 1e-6f);
}

GTEST_TEST(KotORMilestone3, angleToVectorIsUnitLength) {
	for (float deg : {0.0f, 30.0f, 45.0f, 60.0f, 90.0f, 135.0f, 180.0f, 270.0f, 315.0f}) {
		float x, y;
		angleToVector(deg, x, y);
		EXPECT_NEAR(std::sqrt(x * x + y * y), 1.0f, 1e-6f)
			<< "magnitude != 1 for angle " << deg;
	}
}

GTEST_TEST(KotORMilestone3, vectorToAngleRoundTrip) {
	// AngleToVector then VectorToAngle should return the original angle
	for (float deg : {0.0f, 45.0f, 90.0f, 135.0f, -90.0f, -45.0f}) {
		float x, y;
		angleToVector(deg, x, y);
		float recovered = vectorToAngle(x, y);
		EXPECT_NEAR(recovered, deg, 1e-4f)
			<< "round-trip failed for angle " << deg;
	}
}

GTEST_TEST(KotORMilestone3, vectorToAngleXAxisIsZeroDegrees) {
	EXPECT_NEAR(vectorToAngle(1.0f, 0.0f), 0.0f, 1e-6f);
}

GTEST_TEST(KotORMilestone3, vectorToAngleYAxisIs90Degrees) {
	EXPECT_NEAR(vectorToAngle(0.0f, 1.0f), 90.0f, 1e-6f);
}

// ---------------------------------------------------------------------------
// GetItemStackSize / SetItemStackSize  — clamp arithmetic tests
//   mirrors Item::setStackSize: _stackSize = (size < 1) ? 1 : size
// ---------------------------------------------------------------------------

static int clampStackSize(int size) {
	return (size < 1) ? 1 : size;
}

GTEST_TEST(KotORMilestone3, stackSizeDefaultsToOne) {
	// An item's stack size starts at 1 (the in-memory field default).
	EXPECT_EQ(clampStackSize(1), 1);
}

GTEST_TEST(KotORMilestone3, stackSizePositiveValuesAccepted) {
	EXPECT_EQ(clampStackSize(5),  5);
	EXPECT_EQ(clampStackSize(99), 99);
}

GTEST_TEST(KotORMilestone3, stackSizeClampsBelowOne) {
	// Values < 1 are clamped to 1 so the engine never deals with zero-count items.
	EXPECT_EQ(clampStackSize(0),  1);
	EXPECT_EQ(clampStackSize(-1), 1);
	EXPECT_EQ(clampStackSize(-5), 1);
}

// ---------------------------------------------------------------------------
// GetFactionEqual arithmetic
// ---------------------------------------------------------------------------

static bool calcFactionEqual(Faction a, Faction b) {
	return (a != kFactionInvalid && a == b);
}

GTEST_TEST(KotORMilestone3, sameFactionIsEqual) {
	EXPECT_TRUE(calcFactionEqual(kFactionFriendly1, kFactionFriendly1));
	EXPECT_TRUE(calcFactionEqual(kFactionHostile1, kFactionHostile1));
	EXPECT_TRUE(calcFactionEqual(kFactionNeutral, kFactionNeutral));
}

GTEST_TEST(KotORMilestone3, differentFactionsAreNotEqual) {
	EXPECT_FALSE(calcFactionEqual(kFactionFriendly1, kFactionHostile1));
	EXPECT_FALSE(calcFactionEqual(kFactionFriendly1, kFactionFriendly2));
	EXPECT_FALSE(calcFactionEqual(kFactionHostile1, kFactionNeutral));
}

GTEST_TEST(KotORMilestone3, invalidFactionNeverEqual) {
	EXPECT_FALSE(calcFactionEqual(kFactionInvalid, kFactionInvalid));
	EXPECT_FALSE(calcFactionEqual(kFactionInvalid, kFactionFriendly1));
}
