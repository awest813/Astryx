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
 *  The engine behaviour the Endar Spire's party and game-flow scripts rely on:
 *  Trask joining and leaving the party (k_pend_traskdl29, cp_end_trasksp_d,
 *  TalkTrask, k_pend_traskdie1) and the global fades of k_pend_area01.
 */

#include "gtest/gtest.h"

#include "src/graphics/aurora/fadequad.h"

#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/partycontroller.h"
#include "src/engines/kotorbase/types.h"

using namespace Engines::KotORBase;
using Graphics::Aurora::FadeQuad;

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

static const int kNPCTrask = 0; // AddAvailableNPCByTemplate(0, "end_trask")
static const int kNPCCarth = 2;

} // End of anonymous namespace

// --- Party: Trask joins ---

GTEST_TEST(EndarSpireFlow, partyMemberOneIsTraskAfterHeJoins) {
	// cp_end_trasksp_d: GetPartyMemberByIndex(1) is the new party Trask.
	PartyController party(nullptr);
	TestCreature pc, trask;

	party.addPartyMember(PartyController::kPCSlot, &pc);
	party.addPartyMember(kNPCTrask, &trask);

	ASSERT_EQ(party.getPartyMemberCount(), 2u);
	EXPECT_EQ(party.getPartyLeader(), &pc);
	EXPECT_EQ(party.getPartyMemberByIndex(1).second, &trask);
	EXPECT_EQ(party.getPartyMemberByIndex(1).first, kNPCTrask);
	EXPECT_TRUE(party.isObjectPartyMember(&trask));
}

GTEST_TEST(EndarSpireFlow, availableRosterHoldsTrask) {
	// k_pend_traskdl29: IsAvailableCreature(0) gates AddAvailableNPCByTemplate.
	PartyController party(nullptr);
	EXPECT_FALSE(party.isAvailableCreature(kNPCTrask));

	party.addAvailableNPCByTemplate(kNPCTrask, "end_trask");
	EXPECT_TRUE(party.isAvailableCreature(kNPCTrask));
	EXPECT_EQ(party.getAvailableNPCTemplate(kNPCTrask), "end_trask");

	// k_pend_traskdie1: RemoveAvailableNPC(0).
	party.removeAvailableNPC(kNPCTrask);
	EXPECT_FALSE(party.isAvailableCreature(kNPCTrask));
}

GTEST_TEST(EndarSpireFlow, talkTraskSwitchesBackToThePlayer) {
	// TalkTrask: if the player controls Trask, SetPartyLeader(NPC_PLAYER).
	PartyController party(nullptr);
	TestCreature pc, trask;

	party.addPartyMember(PartyController::kPCSlot, &pc);
	party.addPartyMember(kNPCTrask, &trask);

	party.setPartyLeaderByIndex(1);
	EXPECT_EQ(party.getPartyLeader(), &trask);
	EXPECT_EQ(party.getPartyMemberByIndex(1).second, &pc);

	party.setPartyLeader(PartyController::kPCSlot);
	EXPECT_EQ(party.getPartyLeader(), &pc);
	EXPECT_EQ(party.getPartyMemberByIndex(1).second, &trask);
	EXPECT_FALSE(pc.isUsable());
	EXPECT_TRUE(trask.isUsable());
}

// --- Party: Trask leaves (k_pend_traskdie1) ---

GTEST_TEST(EndarSpireFlow, removedPartyMemberStaysInTheWorld) {
	// RemovePartyMember(0) takes Trask out of the party; the script then gives
	// him a sword and has him jump into the Bandon cutscene, so he must stay a
	// valid, clickable creature where he stands.
	PartyController party(nullptr);
	TestCreature pc, trask;

	party.addPartyMember(PartyController::kPCSlot, &pc);
	party.addPartyMember(kNPCTrask, &trask);
	trask.setPosition(4.0f, 5.0f, 0.0f);
	trask.setUsable(false);

	EXPECT_TRUE(party.removePartyMember(&trask));

	EXPECT_EQ(party.getPartyMemberCount(), 1u);
	EXPECT_EQ(party.getPartyLeader(), &pc);
	EXPECT_FALSE(party.isObjectPartyMember(&trask));
	EXPECT_TRUE(trask.isUsable());

	float x, y, z;
	trask.getPosition(x, y, z);
	EXPECT_FLOAT_EQ(x, 4.0f);
	EXPECT_FLOAT_EQ(y, 5.0f);

	// Not a member any more: removing again changes nothing.
	EXPECT_FALSE(party.removePartyMember(&trask));
	EXPECT_EQ(party.getPartyMemberCount(), 1u);
}

GTEST_TEST(EndarSpireFlow, removingAMemberLeavesTheOthersWhereTheyStand) {
	// Removing one companion must not snap the others onto the leader.
	PartyController party(nullptr);
	TestCreature pc, trask, carth;

	pc.setPosition(0.0f, 0.0f, 0.0f);
	party.addPartyMember(PartyController::kPCSlot, &pc);
	party.addPartyMember(kNPCTrask, &trask);
	party.addPartyMember(kNPCCarth, &carth);
	carth.setPosition(10.0f, 2.0f, 0.0f);

	EXPECT_TRUE(party.removePartyMember(&trask));

	ASSERT_EQ(party.getPartyMemberCount(), 2u);
	EXPECT_EQ(party.getPartyMemberByIndex(1).second, &carth);

	float x, y, z;
	carth.getPosition(x, y, z);
	EXPECT_FLOAT_EQ(x, 10.0f);
	EXPECT_FLOAT_EQ(y, 2.0f);
}

GTEST_TEST(EndarSpireFlow, removingTheLeaderHandsControlOn) {
	PartyController party(nullptr);
	TestCreature pc, trask;

	party.addPartyMember(PartyController::kPCSlot, &pc);
	party.addPartyMember(kNPCTrask, &trask);
	party.setPartyLeaderByIndex(1);
	ASSERT_EQ(party.getPartyLeader(), &trask);

	EXPECT_TRUE(party.removePartyMember(&trask));
	EXPECT_EQ(party.getPartyLeader(), &pc);
	EXPECT_FALSE(pc.isUsable());
	EXPECT_TRUE(trask.isUsable());
}

// --- Fades (k_pend_area01, k_pend_traskdl29 / cp_end_trasksp_d) ---

GTEST_TEST(EndarSpireFlow, fadeInStartsFromTheColor) {
	// SetGlobalFadeIn(0.9, 1.5) after SetGlobalFadeOut(): the screen stays
	// covered for the wait whatever was drawn before the call.
	EXPECT_FLOAT_EQ(FadeQuad::getFadeStartOpacity(true, 0.0f, 1500), 1.0f);
	EXPECT_FLOAT_EQ(FadeQuad::getFadeStartOpacity(true, 1.0f, 1500), 1.0f);

	// An immediate cut in (the default arguments) keeps it until it happens.
	EXPECT_FLOAT_EQ(FadeQuad::getFadeStartOpacity(true, 0.3f, 0), 0.3f);

	// A fade out starts from what is shown.
	EXPECT_FLOAT_EQ(FadeQuad::getFadeStartOpacity(false, 0.25f, 1000), 0.25f);
}

GTEST_TEST(EndarSpireFlow, fadeInHoldsDuringItsWaitThenClears) {
	const uint32_t wait = 900, run = 1500;

	EXPECT_FLOAT_EQ(FadeQuad::getFadeOpacity(true, 1.0f,    0, wait, run), 1.0f);
	EXPECT_FLOAT_EQ(FadeQuad::getFadeOpacity(true, 1.0f,  899, wait, run), 1.0f);
	EXPECT_FLOAT_EQ(FadeQuad::getFadeOpacity(true, 1.0f,  900, wait, run), 1.0f);
	EXPECT_NEAR    (FadeQuad::getFadeOpacity(true, 1.0f, 1650, wait, run), 0.5f, 0.001f);
	EXPECT_FLOAT_EQ(FadeQuad::getFadeOpacity(true, 1.0f, 2400, wait, run), 0.0f);
	EXPECT_FLOAT_EQ(FadeQuad::getFadeOpacity(true, 1.0f, 9000, wait, run), 0.0f);
}

GTEST_TEST(EndarSpireFlow, fadeOutCutsAndFades) {
	// SetGlobalFadeOut(): an immediate cut to black.
	EXPECT_FLOAT_EQ(FadeQuad::getFadeOpacity(false, 0.0f, 1, 0, 0), 1.0f);

	// A gradual fade out from a clear screen.
	EXPECT_FLOAT_EQ(FadeQuad::getFadeOpacity(false, 0.0f,  500, 0, 1000), 0.5f);
	EXPECT_FLOAT_EQ(FadeQuad::getFadeOpacity(false, 0.0f, 2000, 0, 1000), 1.0f);

	// An already black screen doesn't flash back in.
	EXPECT_FLOAT_EQ(FadeQuad::getFadeOpacity(false, 1.0f,  100, 0, 1000), 1.0f);
}

GTEST_TEST(EndarSpireFlow, cutInAfterAWait) {
	// SetGlobalFadeIn(0.5, 0.0): stays as it is, then cuts in.
	EXPECT_FLOAT_EQ(FadeQuad::getFadeOpacity(true, 1.0f, 400, 500, 0), 1.0f);
	EXPECT_FLOAT_EQ(FadeQuad::getFadeOpacity(true, 1.0f, 600, 500, 0), 0.0f);
}
