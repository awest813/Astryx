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
 *  Regression tests for the Endar Spire NWScript support slice.
 */

#include "gtest/gtest.h"

#include "src/engines/kotorbase/creatureinfo.h"
#include "src/engines/kotorbase/location.h"
#include "src/engines/kotorbase/object.h"
#include "src/engines/kotorbase/script/endar_spire_support.h"
#include "src/engines/kotorbase/types.h"

using namespace Engines::KotORBase;

namespace {

int computeStartingHP(Class klass, int constitutionScore) {
	int classHitDie = 0;
	switch (klass) {
		case kClassSoldier:   classHitDie = 10; break;
		case kClassScout:     classHitDie = 8;  break;
		case kClassScoundrel: classHitDie = 6;  break;
		default: break;
	}

	CreatureInfo info;
	info.setAbilityScore(kAbilityConstitution, constitutionScore);
	return classHitDie + info.getAbilityModifier(kAbilityConstitution);
}

} // End of anonymous namespace

GTEST_TEST(EndarSpireGoldenPath, characterCreationUsesKotorAbilityModifiers) {
	EXPECT_EQ(computeStartingHP(kClassSoldier, 14), 12);
	EXPECT_EQ(computeStartingHP(kClassScout, 12), 9);
	EXPECT_EQ(computeStartingHP(kClassScoundrel, 12), 7);
}

GTEST_TEST(EndarSpireGoldenPath, moveToLocationBuildsSinglePreciseMoveAction) {
	Location destination;
	destination.setPosition(12.5f, -3.0f, 1.25f);
	destination.setFacing(180.0f);

	Action action = EndarSpireSupport::makeMoveToLocationAction(destination);

	EXPECT_EQ(action.type, kActionMoveToPoint);
	EXPECT_FLOAT_EQ(action.range, 0.1f);
	EXPECT_FLOAT_EQ(action.location.x, 12.5f);
	EXPECT_FLOAT_EQ(action.location.y, -3.0f);
	EXPECT_FLOAT_EQ(action.location.z, 1.25f);
}

GTEST_TEST(EndarSpireGoldenPath, jumpToLocationAppliesFacingAndPosition) {
	Object object(kObjectTypeCreature);
	object.setPosition(0.0f, 0.0f, 0.0f);
	object.setOrientation(0.0f, 0.0f, 1.0f, 0.0f);

	Location destination;
	destination.setPosition(8.0f, 4.0f, 2.0f);
	destination.setFacing(135.0f);

	EndarSpireSupport::applyLocationToObject(object, destination);

	float x, y, z;
	object.getPosition(x, y, z);
	EXPECT_FLOAT_EQ(x, 8.0f);
	EXPECT_FLOAT_EQ(y, 4.0f);
	EXPECT_FLOAT_EQ(z, 2.0f);

	float ox, oy, oz, facing;
	object.getOrientation(ox, oy, oz, facing);
	EXPECT_FLOAT_EQ(ox, 0.0f);
	EXPECT_FLOAT_EQ(oy, 0.0f);
	EXPECT_FLOAT_EQ(oz, 1.0f);
	EXPECT_FLOAT_EQ(facing, 135.0f);
}

GTEST_TEST(EndarSpireGoldenPath, objectLocationRoundTripPreservesFacing) {
	Object source(kObjectTypeCreature);
	source.setPosition(3.0f, 6.0f, 9.0f);
	source.setOrientation(0.0f, 0.0f, 1.0f, 270.0f);

	Location location = source.getLocation();

	float x, y, z;
	location.getPosition(x, y, z);
	EXPECT_FLOAT_EQ(x, 3.0f);
	EXPECT_FLOAT_EQ(y, 6.0f);
	EXPECT_FLOAT_EQ(z, 9.0f);
	EXPECT_FLOAT_EQ(location.getFacing(), 270.0f);
}

GTEST_TEST(EndarSpireGoldenPath, actionAnimationIdsStayMappedToTutorialAnimations) {
	EXPECT_STREQ(EndarSpireSupport::getActionAnimationName(10), "talk");
	EXPECT_STREQ(EndarSpireSupport::getActionAnimationName(38), "attack1");
	EXPECT_STREQ(EndarSpireSupport::getActionAnimationName(44), "die");
	EXPECT_EQ(EndarSpireSupport::getActionAnimationName(999), nullptr);
}
