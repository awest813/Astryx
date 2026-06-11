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
 *  Regression tests for cinematic camera state transitions.
 */

#include "gtest/gtest.h"

#include <cmath>

/** Mirrors CameraController::restoreGameplayCamera early-out logic. */
static bool shouldSnapToOrbit(bool cinematic, float blendTime) {
	return blendTime <= 0.0f || !cinematic;
}

/** Mirrors smoothstep used during gameplay camera restore. */
static float smoothstep(float t) {
	if (t < 0.0f) t = 0.0f;
	if (t > 1.0f) t = 1.0f;
	return t * t * (3.0f - 2.0f * t);
}

TEST(CameraCutscene, RestoreGameplayRequiresCinematicMode) {
	EXPECT_TRUE(shouldSnapToOrbit(false, 1.0f));
	EXPECT_FALSE(shouldSnapToOrbit(true, 1.0f));
	EXPECT_TRUE(shouldSnapToOrbit(true, 0.0f));
}

TEST(CameraCutscene, SmoothstepEndpoints) {
	EXPECT_FLOAT_EQ(smoothstep(0.0f), 0.0f);
	EXPECT_FLOAT_EQ(smoothstep(1.0f), 1.0f);
}

TEST(CameraCutscene, SmoothstepMidpoint) {
	EXPECT_NEAR(smoothstep(0.5f), 0.5f, 0.001f);
}

TEST(CameraCutscene, SmoothstepIsMonotonic) {
	float prev = smoothstep(0.0f);
	for (int i = 1; i <= 10; ++i) {
		const float t = i / 10.0f;
		const float cur = smoothstep(t);
		EXPECT_GE(cur, prev);
		prev = cur;
	}
}
