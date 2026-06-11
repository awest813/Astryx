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
 *  Unit tests for the swoop minigame helper.
 */

#include "gtest/gtest.h"

#include "src/engines/kotorbase/swoopminigame.h"

using Engines::KotORBase::SwoopMinigame;

TEST(SwoopMinigame, ObstacleHitSlowsSpeed) {
	SwoopMinigame &swoop = SwoopMinigame::get();
	swoop.reset();
	swoop.setActive(true);
	swoop.setSpeed(100.0f);
	swoop.onObstacleHit();

	EXPECT_LT(swoop.getSpeed(), 100.0f);
	EXPECT_EQ(swoop.getLastEvent(), 1);
}

TEST(SwoopMinigame, OffsetClampedToTunnelBounds) {
	SwoopMinigame &swoop = SwoopMinigame::get();
	swoop.reset();
	swoop.setTunnelPos(1.0f);
	swoop.setTunnelNeg(-1.0f);
	swoop.setPlayerOffset(5.0f);

	EXPECT_EQ(swoop.getPlayerOffset(), 1.0f);
}

TEST(SwoopMinigame, ActiveUpdateIncreasesSpeed) {
	SwoopMinigame &swoop = SwoopMinigame::get();
	swoop.reset();
	swoop.setActive(true);
	swoop.setSpeed(0.0f);
	swoop.setMaxSpeed(50.0f);
	swoop.update(1.0f);

	EXPECT_GT(swoop.getSpeed(), 0.0f);
}
