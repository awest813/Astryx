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
 *  Unit tests for pausing the game and auto-pause.
 */

#include <set>

#include "gtest/gtest.h"

#include "src/common/configman.h"

#include "src/engines/kotorbase/pause.h"

using namespace Engines::KotORBase;

GTEST_TEST(KotORBasePause, toggleReportsPausedTime) {
	PauseState state;
	const PauseBlockers none;

	EXPECT_FALSE(state.isPaused());
	EXPECT_EQ(state.toggle(none, 1000), 0u);
	EXPECT_TRUE(state.isPaused());
	EXPECT_EQ(state.getReason(), kPauseReasonGeneric);
	EXPECT_TRUE(state.isWorldFrozen(none));

	EXPECT_EQ(state.toggle(none, 3500), 2500u);
	EXPECT_FALSE(state.isPaused());
	EXPECT_FALSE(state.isWorldFrozen(none));

	// Unpausing when not paused is harmless.
	EXPECT_EQ(state.unpause(5000), 0u);
}

GTEST_TEST(KotORBasePause, neverPausesDuringConversationsCutscenesMinigames) {
	AutoPauseOptions options;

	PauseBlockers conversation, cutscene, minigame, movie;
	conversation.conversation = true;
	cutscene.cutscene = true;
	minigame.minigame = true;
	movie.movie = true;

	for (const PauseBlockers &blockers : { conversation, cutscene, minigame, movie }) {
		PauseState state;
		EXPECT_TRUE(blockers.any());
		EXPECT_FALSE(state.requestPause(kPauseReasonGeneric, options, blockers, 10));
		EXPECT_EQ(state.toggle(blockers, 20), 0u);
		EXPECT_FALSE(state.isPaused());
	}

	// A pause that is already in place doesn't freeze a conversation.
	PauseState state;
	EXPECT_TRUE(state.requestPause(kPauseReasonGeneric, options, PauseBlockers(), 10));
	EXPECT_FALSE(state.isWorldFrozen(conversation));
	EXPECT_TRUE(state.isWorldFrozen(PauseBlockers()));
}

GTEST_TEST(KotORBasePause, autoPauseFollowsOptions) {
	AutoPauseOptions options;
	options.endOfCombatRound = false;
	options.enemySighted = true;
	options.actionMenuUsed = false;

	PauseState state;
	EXPECT_FALSE(state.requestPause(kPauseReasonCombatRoundEnd, options, PauseBlockers(), 0));
	EXPECT_FALSE(state.requestPause(kPauseReasonActionMenuUsed, options, PauseBlockers(), 0));
	EXPECT_FALSE(state.isPaused());

	EXPECT_TRUE(state.requestPause(kPauseReasonEnemySighted, options, PauseBlockers(), 100));
	EXPECT_EQ(state.getReason(), kPauseReasonEnemySighted);

	// A later reason replaces the shown one, but the pause keeps its start time.
	EXPECT_TRUE(state.requestPause(kPauseReasonGeneric, options, PauseBlockers(), 400));
	EXPECT_EQ(state.getReason(), kPauseReasonGeneric);
	EXPECT_EQ(state.unpause(600), 500u);

	// Generic pauses can't be turned off.
	AutoPauseOptions off;
	off.endOfCombatRound = off.enemySighted = off.mineSighted = false;
	off.partyMemberDown = off.actionMenuUsed = off.newTargetSelected = false;
	EXPECT_TRUE(off.isEnabled(kPauseReasonGeneric));
	EXPECT_FALSE(off.isEnabled(kPauseReasonPartyMemberKilled));
	EXPECT_FALSE(off.isEnabled(kPauseReasonNewTargetSelected));
}

GTEST_TEST(KotORBasePause, optionsFromConfig) {
	ConfigMan.setBool(Common::kConfigRealmDefault, "endofcombatround", true);
	ConfigMan.setBool(Common::kConfigRealmDefault, "enemysighted", false);
	ConfigMan.setBool(Common::kConfigRealmDefault, "minesighted", false);
	ConfigMan.setBool(Common::kConfigRealmDefault, "partymemberdown", false);
	ConfigMan.setBool(Common::kConfigRealmDefault, "actionmenuused", true);
	ConfigMan.setBool(Common::kConfigRealmDefault, "newtargetselected", false);

	const AutoPauseOptions options = AutoPauseOptions::fromConfig();
	EXPECT_TRUE(options.endOfCombatRound);
	EXPECT_FALSE(options.enemySighted);
	EXPECT_FALSE(options.mineSighted);
	EXPECT_FALSE(options.partyMemberDown);
	EXPECT_TRUE(options.actionMenuUsed);
	EXPECT_FALSE(options.newTargetSelected);
}

GTEST_TEST(KotORBasePause, reasonTexts) {
	EXPECT_EQ(getPauseReasonFallbackText(kPauseReasonGeneric), "Paused");
	EXPECT_EQ(getPauseReasonFallbackText(kPauseReasonEnemySighted), "Paused: Enemy sighted");
	EXPECT_EQ(getPauseReasonStrRef(kPauseReasonGeneric), 1508u);
	EXPECT_EQ(getPauseReasonStrRef(kPauseReasonEnemySighted), 42396u);
}

GTEST_TEST(KotORBaseAutoPause, enemySightedOnlyWhenNewAndOutOfCombat) {
	AutoPauseMonitor monitor;

	EXPECT_FALSE(monitor.updateSightedEnemies({}, false));
	EXPECT_TRUE(monitor.updateSightedEnemies({ 7 }, false));
	// Still the same enemy: no new pause.
	EXPECT_FALSE(monitor.updateSightedEnemies({ 7 }, false));
	// A second enemy shows up while the leader already fights: no pause.
	EXPECT_FALSE(monitor.updateSightedEnemies({ 7, 8 }, true));
	// The enemy leaves sight and comes back: already seen, no new pause.
	EXPECT_FALSE(monitor.updateSightedEnemies({ 8 }, false));
	EXPECT_FALSE(monitor.updateSightedEnemies({ 7, 8 }, false));
	// A third enemy shows up out of combat.
	EXPECT_TRUE(monitor.updateSightedEnemies({ 7, 8, 9 }, false));

	monitor.reset();
	EXPECT_TRUE(monitor.updateSightedEnemies({ 7, 8 }, false));
}

GTEST_TEST(KotORBaseAutoPause, minesAndPartyDown) {
	AutoPauseMonitor monitor;

	EXPECT_TRUE(monitor.updateSightedMines({ 3 }));
	EXPECT_FALSE(monitor.updateSightedMines({ 3 }));

	EXPECT_FALSE(monitor.updatePartyDown({}));
	EXPECT_TRUE(monitor.updatePartyDown({ 1 }));
	EXPECT_FALSE(monitor.updatePartyDown({ 1 }));
	EXPECT_FALSE(monitor.updatePartyDown({}));      // Got back up.
	EXPECT_TRUE(monitor.updatePartyDown({ 1, 2 }));
}

GTEST_TEST(KotORBaseAutoPause, newTargetAfterTheOldOneDied) {
	AutoPauseMonitor monitor;
	std::set<uint32_t> dead;
	auto isDead = [&dead](uint32_t id) { return dead.count(id) > 0; };

	EXPECT_FALSE(monitor.updateLeaderTarget(10, true, isDead)); // First target.
	EXPECT_FALSE(monitor.updateLeaderTarget(11, true, isDead)); // Switched by choice.

	dead.insert(11);
	EXPECT_TRUE(monitor.updateLeaderTarget(12, true, isDead));  // Old one died.

	// The dead target is dropped first, then a new one is picked.
	dead.insert(12);
	EXPECT_FALSE(monitor.updateLeaderTarget(0, true, isDead));
	EXPECT_TRUE(monitor.updateLeaderTarget(13, true, isDead));

	// The fight ended in between: the next target is a new fight.
	dead.insert(13);
	EXPECT_FALSE(monitor.updateLeaderTarget(0, false, isDead));
	EXPECT_FALSE(monitor.updateLeaderTarget(14, true, isDead));
}
