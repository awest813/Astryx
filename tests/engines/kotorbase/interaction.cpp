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
 *  Unit tests for KotOR world interactions: bashing locked doors and
 *  containers open, computer terminals, setting mines, seeing invisible
 *  and stealthy creatures.
 */

#include <cmath>
#include <vector>

#include "gtest/gtest.h"

#include "src/common/memreadstream.h"

#include "src/aurora/2dafile.h"

#include "src/engines/kotorbase/inventory.h"
#include "src/engines/kotorbase/perception.h"
#include "src/engines/kotorbase/situated.h"
#include "src/engines/kotorbase/trap.h"
#include "src/engines/kotorbase/gui/dialog.h"

using namespace Engines::KotORBase;

GTEST_TEST(KotORInteraction, bashEligibility) {
	// Locked, with hit points, neither plot nor NotBlastable, not broken yet.
	EXPECT_TRUE(BashRules::canBash(true, false, false, false, 15));

	EXPECT_FALSE(BashRules::canBash(false, false, false, false, 15)); // Unlocked: just open it.
	EXPECT_FALSE(BashRules::canBash(true, true,  false, false, 15)); // Plot.
	EXPECT_FALSE(BashRules::canBash(true, false, true,  false, 15)); // NotBlastable.
	EXPECT_FALSE(BashRules::canBash(true, false, false, true,  15)); // Already broken.
	EXPECT_FALSE(BashRules::canBash(true, false, false, false, 0));  // No hit points.
}

GTEST_TEST(KotORInteraction, bashDamageAgainstHardness) {
	EXPECT_EQ(BashRules::damageAfterHardness(12, 5), 7);
	EXPECT_EQ(BashRules::damageAfterHardness(5, 5), 0);  // Soaked completely.
	EXPECT_EQ(BashRules::damageAfterHardness(3, 10), 0); // Never negative.
	EXPECT_EQ(BashRules::damageAfterHardness(8, 0), 8);
	EXPECT_EQ(BashRules::damageAfterHardness(8, -2), 8); // Bad data soaks nothing.

	EXPECT_EQ(BashRules::hitPointsAfterDamage(20, 7, false), 13);
	EXPECT_EQ(BashRules::hitPointsAfterDamage(5, 7, false), 0);  // Breaks.
	EXPECT_EQ(BashRules::hitPointsAfterDamage(5, 7, true), 1);   // Min1HP holds.
	EXPECT_EQ(BashRules::hitPointsAfterDamage(5, -3, false), 5); // No healing by blows.
}

GTEST_TEST(KotORInteraction, computerConversationDetection) {
	EXPECT_FALSE(DialogGUI::isComputerConversation(0)); // Spoken conversation.
	EXPECT_TRUE(DialogGUI::isComputerConversation(kConversationTypeComputer));
	EXPECT_FALSE(DialogGUI::isComputerConversation(2));

	// A DLG that can't be read is an ordinary conversation.
	EXPECT_EQ(DialogGUI::getConversationType("no_such_dlg"), 0u);
}

GTEST_TEST(KotORInteraction, computerCountersCountItemsByTag) {
	Inventory inventory;
	inventory.addItem("k_computer_spike", 3);
	inventory.addItem("K_REPAIR_PART", 2);
	inventory.addItem("g_i_medeqpmnt01");

	// Item tags are matched whatever their case (scripts use "K_COMPUTER_SPIKE").
	EXPECT_EQ(inventory.countItems(kComputerSpikeTag), 3);
	EXPECT_EQ(inventory.countItems(kRepairPartTag), 2);
	EXPECT_EQ(inventory.countItems("g_i_adrnaline001"), 0);

	inventory.removeItem("k_computer_spike", 1);
	EXPECT_EQ(inventory.countItems(kComputerSpikeTag), 2);
}

GTEST_TEST(KotORInteraction, minePlacementAheadOfTheSetter) {
	// Orientation 0 faces +Y (Object::makeLookAt()), 90 faces -X, -90 faces +X.
	glm::vec2 at = TrapRules::getMinePlacement(glm::vec2(10.0f, 20.0f), 0.0f, 1.0f);
	EXPECT_NEAR(at.x, 10.0f, 1e-4f);
	EXPECT_NEAR(at.y, 21.0f, 1e-4f);

	at = TrapRules::getMinePlacement(glm::vec2(10.0f, 20.0f), 90.0f, 2.0f);
	EXPECT_NEAR(at.x, 8.0f, 1e-4f);
	EXPECT_NEAR(at.y, 20.0f, 1e-4f);

	at = TrapRules::getMinePlacement(glm::vec2(0.0f, 0.0f), -90.0f, TrapRules::kMinePlaceDistance);
	EXPECT_NEAR(at.x, TrapRules::kMinePlaceDistance, 1e-4f);
	EXPECT_NEAR(at.y, 0.0f, 1e-4f);

	// A square around the mine.
	const std::vector<glm::vec3> geometry = TrapRules::getMineGeometry(0.5f);
	ASSERT_EQ(geometry.size(), 4u);
	for (const glm::vec3 &point : geometry) {
		EXPECT_FLOAT_EQ(std::fabs(point.x), 0.5f);
		EXPECT_FLOAT_EQ(std::fabs(point.y), 0.5f);
		EXPECT_FLOAT_EQ(point.z, 0.0f);
	}
}

GTEST_TEST(KotORInteraction, mineItemsAndTrapTypes) {
	EXPECT_TRUE(TrapRules::isMineItem(TrapRules::kBaseItemMine, false));
	EXPECT_TRUE(TrapRules::isMineItem(1, true));   // A Trap property makes a mine.
	EXPECT_FALSE(TrapRules::isMineItem(59, false)); // Security tunneler.

	Common::MemoryReadStream stream(
		"2DA V2.0\n"
		"\n"
		"   label            trapscript       resref         detectdcmod disarmdcmod\n"
		"0  Minor_Frag       k_trp_minorfrag  g_i_trapkit001 0           0\n"
		"1  Average_Frag     k_trp_avgfrag    g_i_trapkit002 5           5\n"
		"2  Minor_Gas        k_trp_minorgas   g_i_trapkit010 0           0\n");
	const Aurora::TwoDAFile traps(stream);

	// The row whose recovered item is the mine item, whatever the case.
	EXPECT_EQ(TrapRules::findTrapType(traps, "G_I_TRAPKIT002", -1), 1);
	EXPECT_EQ(TrapRules::findTrapType(traps, "g_i_trapkit010", 0), 2);
	// Else the Trap property's subtype, when it is a row.
	EXPECT_EQ(TrapRules::findTrapType(traps, "g_i_unknown", 0), 0);
	EXPECT_EQ(TrapRules::findTrapType(traps, "g_i_unknown", 7), -1);
	EXPECT_EQ(TrapRules::findTrapType(traps, "", -1), -1);

	// DCs grow with the setter's Demolitions.
	EXPECT_EQ(TrapRules::getSetMineDC(0, 0), 10);
	EXPECT_EQ(TrapRules::getSetMineDC(6, 5), 21);
	EXPECT_EQ(TrapRules::getSetMineDC(-3, 0), 10);
}

GTEST_TEST(KotORInteraction, perceptionOfInvisibleCreatures) {
	PerceptionRules::SightCheck check;
	check.distance = 5.0f;
	EXPECT_TRUE(PerceptionRules::canSee(check));

	// Out of range: unseen, whatever.
	check.distance = PerceptionRules::kPerceptionRange + 1.0f;
	EXPECT_FALSE(PerceptionRules::canSee(check));
	check.distance = 5.0f;

	// Invisible: unseen by enemies without True Seeing.
	check.targetInvisible = true;
	EXPECT_FALSE(PerceptionRules::canSee(check));
	check.seesInvisible = true;
	EXPECT_TRUE(PerceptionRules::canSee(check));
	check.seesInvisible = false;

	// Friends always see each other.
	check.hostile = false;
	EXPECT_TRUE(PerceptionRules::canSee(check));
}

GTEST_TEST(KotORInteraction, perceptionOfStealthyCreatures) {
	// Awareness against Stealth, both taking 10; distance helps the sneak.
	EXPECT_TRUE(PerceptionRules::spotsStealthy(5, 5, 0.0f));
	EXPECT_FALSE(PerceptionRules::spotsStealthy(4, 5, 0.0f));
	EXPECT_TRUE(PerceptionRules::spotsStealthy(8, 5, 12.0f));  // +3 for 12 m.
	EXPECT_FALSE(PerceptionRules::spotsStealthy(8, 5, 16.0f)); // +4 for 16 m.

	PerceptionRules::SightCheck check;
	check.targetStealthy = true;
	check.targetStealth  = 8;
	check.awareness      = 6;
	check.distance       = 3.0f;
	EXPECT_FALSE(PerceptionRules::canSee(check));

	check.awareness = 9;
	EXPECT_TRUE(PerceptionRules::canSee(check));

	// True Seeing doesn't find the stealthy.
	check.awareness     = 2;
	check.seesInvisible = true;
	EXPECT_FALSE(PerceptionRules::canSee(check));

	// Not sneaking: seen.
	check.targetStealthy = false;
	EXPECT_TRUE(PerceptionRules::canSee(check));
}
