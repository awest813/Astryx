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
 *  Unit tests for Area line-of-sight geometry helpers.
 */

#include "gtest/gtest.h"

#include "src/engines/kotorbase/area.h"

using Engines::KotORBase::Area;

GTEST_TEST(LineOfSight, segmentIntersectsCircle) {
	// Horizontal segment through origin circle.
	EXPECT_TRUE(Area::segmentIntersectsCircle(-2.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 0.5f));

	// Segment misses the circle.
	EXPECT_FALSE(Area::segmentIntersectsCircle(-2.0f, 2.0f, 2.0f, 2.0f, 0.0f, 0.0f, 0.5f));

	// Endpoint inside circle.
	EXPECT_TRUE(Area::segmentIntersectsCircle(0.0f, 0.0f, 5.0f, 0.0f, 0.2f, 0.0f, 0.5f));

	// Degenerate segment at center.
	EXPECT_TRUE(Area::segmentIntersectsCircle(1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.25f));
}
