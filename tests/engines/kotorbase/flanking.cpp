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
 *  Unit tests for flanking geometry and opposite-side detection.
 */

#include "gtest/gtest.h"

#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/types.h"

using namespace Engines::KotORBase;

namespace {

class TestCreature : public Creature {
public:
	TestCreature() : Creature() {}

protected:
	void getPartModelsPC(PartModels &parts, uint32_t state, uint8_t textureVariation) override {
		(void)parts;
		(void)state;
		(void)textureVariation;
	}
};

} // End of anonymous namespace

GTEST_TEST(Flanking, oppositeSidesGeometry) {
	// Defender at origin; attackers east and west → flanked.
	EXPECT_TRUE(Creature::areOnOppositeSides(0.0f, 0.0f, 2.0f, 0.0f, -2.0f, 0.0f));

	// Both attackers north of defender → not opposite.
	EXPECT_FALSE(Creature::areOnOppositeSides(0.0f, 0.0f, 1.0f, 2.0f, -1.0f, 2.0f));

	// ~90° apart → not opposite enough.
	EXPECT_FALSE(Creature::areOnOppositeSides(0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 2.0f));

	// ~135° apart → opposite.
	EXPECT_TRUE(Creature::areOnOppositeSides(0.0f, 0.0f, 2.0f, 0.0f, -1.5f, 1.5f));
}

GTEST_TEST(Flanking, isFlankedByRequiresArea) {
	TestCreature defender;
	TestCreature attacker;
	defender.setPosition(0.0f, 0.0f, 0.0f);
	attacker.setPosition(2.0f, 0.0f, 0.0f);

	// Without area context flanking stays conservative.
	EXPECT_FALSE(defender.isFlankedBy(&attacker, nullptr));
}

GTEST_TEST(Flanking, multiClassIncrementsLevel) {
	TestCreature hero;
	CreatureInfo &info = hero.getCreatureInfo();
	EXPECT_EQ(info.getClassLevel(kClassSoldier), 0);

	info.incrementClassLevel(kClassSoldier);
	info.incrementClassLevel(kClassJediGuardian);
	EXPECT_EQ(info.getClassLevel(kClassSoldier), 1);
	EXPECT_EQ(info.getClassLevel(kClassJediGuardian), 1);
	EXPECT_TRUE(info.isJedi());
}
