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

TEST(SwoopMinigame, HitPointsAndInvulnerability) {
	SwoopMinigame &swoop = SwoopMinigame::get();
	swoop.reset();
	swoop.setHitPoints(50.0f);
	swoop.setMaxHitPoints(100.0f);
	swoop.startInvulnerability(2.0f);
	EXPECT_TRUE(swoop.isInvulnerable());

	swoop.onBulletHit(20.0f);
	EXPECT_FLOAT_EQ(swoop.getHitPoints(), 50.0f);

	swoop.update(3.0f);
	EXPECT_FALSE(swoop.isInvulnerable());
	swoop.onDamage(15.0f);
	EXPECT_FLOAT_EQ(swoop.getHitPoints(), 35.0f);
	EXPECT_EQ(swoop.getLastEvent(), 4);
}

TEST(SwoopMinigame, GunBankAndNamedKinds) {
	SwoopMinigame &swoop = SwoopMinigame::get();
	swoop.reset();
	swoop.getGunBank(0).damage = 25.0f;
	swoop.getGunBank(0).bulletModel = "w_blaster";
	EXPECT_EQ(swoop.getGunBankCount(), 4);
	EXPECT_FLOAT_EQ(swoop.getGunBank(0).damage, 25.0f);

	swoop.registerNamedObject("enemy1", 2);
	swoop.registerNamedObject("follow1", 1);
	EXPECT_EQ(swoop.getObjectKind("enemy1"), 2);
	EXPECT_EQ(swoop.getObjectKind("follow1"), 1);
	EXPECT_TRUE(swoop.hasNamedObject("enemy1"));
}

TEST(SwoopMinigame, FollowerHitPoints) {
	SwoopMinigame &swoop = SwoopMinigame::get();
	swoop.reset();
	swoop.setFollowerHitPoints(80.0f);
	swoop.adjustFollowerHitPoints(-30.0f);
	EXPECT_FLOAT_EQ(swoop.getFollowerHitPoints(), 50.0f);
	EXPECT_FLOAT_EQ(swoop.getLastHPChange(), -30.0f);
}
