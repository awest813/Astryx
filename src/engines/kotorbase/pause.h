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
 *  Pausing the game world, and auto-pause (as KotOR.js' AutoPauseManager).
 */

#ifndef ENGINES_KOTORBASE_PAUSE_H
#define ENGINES_KOTORBASE_PAUSE_H

#include <functional>
#include <set>
#include <vector>
#include <cstdint>

#include "src/common/ustring.h"

namespace Engines {

namespace KotORBase {

/** Why the game paused (KotOR.js' AutoPauseState). */
enum PauseReason {
	kPauseReasonGeneric           = 0, ///< The player pressed pause, or a script paused.
	kPauseReasonCombatRoundEnd    = 1,
	kPauseReasonEnemySighted      = 2,
	kPauseReasonMineSighted       = 3,
	kPauseReasonPartyMemberKilled = 4,
	kPauseReasonActionMenuUsed    = 5,
	kPauseReasonNewTargetSelected = 6
};

/** The auto-pause options of the options menu (OptionsAutoPauseMenu). */
struct AutoPauseOptions {
	bool endOfCombatRound  { false };
	bool enemySighted      { true };
	bool mineSighted       { true };
	bool partyMemberDown   { true };
	bool actionMenuUsed    { false };
	bool newTargetSelected { true };

	/** Read the options from the config ("endofcombatround", "enemysighted", ...). */
	static AutoPauseOptions fromConfig();

	/** Does this reason pause the game? Generic pauses always do. */
	bool isEnabled(PauseReason reason) const;
};

/** What currently forbids pausing. */
struct PauseBlockers {
	bool conversation { false }; ///< A conversation (or a cutscene played as one) runs.
	bool cutscene     { false }; ///< Cutscene mode, or the player's input is disabled.
	bool minigame     { false }; ///< A swoop race or turret minigame runs.
	bool movie        { false }; ///< A full-screen movie plays.

	bool any() const;
};

/** English text for a pause reason, when the talk table has none. */
Common::UString getPauseReasonFallbackText(PauseReason reason);
/** dialog.tlk string of a pause reason's message (KotOR.js' AutoPauseManager). */
uint32_t getPauseReasonStrRef(PauseReason reason);

/** The world's pause state. */
class PauseState {
public:
	bool isPaused() const;
	PauseReason getReason() const;

	/** Pause for @p reason if enabled and nothing blocks it; true if now paused. */
	bool requestPause(PauseReason reason, const AutoPauseOptions &options,
	                  const PauseBlockers &blockers, uint32_t now);

	/** Unpause. Returns how long the game was paused (ms) so timers can be shifted. */
	uint32_t unpause(uint32_t now);

	/** Pause (generic) or unpause. Returns the time paused when unpausing, else 0. */
	uint32_t toggle(const PauseBlockers &blockers, uint32_t now);

	/** Should the world (actions, combat, scripts, timers) be frozen this frame? */
	bool isWorldFrozen(const PauseBlockers &blockers) const;

private:
	bool _paused { false };
	PauseReason _reason { kPauseReasonGeneric };
	uint32_t _pausedSince { 0 };
};

/** Detects the events auto-pause reacts to, from per-frame snapshots. */
class AutoPauseMonitor {
public:
	void reset();

	/** Hostiles the party leader currently sees. True when one wasn't seen before
	 *  and the leader isn't fighting yet (KotOR.js pauses only outside of combat). */
	bool updateSightedEnemies(const std::vector<uint32_t> &visibleHostiles, bool leaderInCombat);

	/** Detected mines the party currently sees. True when a new one appeared. */
	bool updateSightedMines(const std::vector<uint32_t> &visibleMines);

	/** Party members currently down. True when one went down since the last update. */
	bool updatePartyDown(const std::vector<uint32_t> &downMembers);

	/** The party leader's attack target (0 for none). True when the leader's
	 *  target died and the leader, still fighting, took on a new one. */
	bool updateLeaderTarget(uint32_t target, bool leaderInCombat,
	                        const std::function<bool(uint32_t)> &isDead);

private:
	std::set<uint32_t> _sightedEnemies;
	std::set<uint32_t> _sightedMines;
	std::set<uint32_t> _downMembers;
	uint32_t _leaderTarget { 0 };
	bool _leaderTargetKilled { false };
};

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_PAUSE_H
