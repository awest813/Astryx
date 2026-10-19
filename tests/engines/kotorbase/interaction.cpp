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
 *  containers open, computer terminals.
 */

#include "gtest/gtest.h"

#include "src/engines/kotorbase/inventory.h"
#include "src/engines/kotorbase/situated.h"
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
