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

#include "src/common/configman.h"

#include "src/engines/kotorbase/pause.h"

namespace Engines {

namespace KotORBase {

// AutoPauseOptions

AutoPauseOptions AutoPauseOptions::fromConfig() {
	AutoPauseOptions options;

	options.endOfCombatRound  = ConfigMan.getBool("endofcombatround",  options.endOfCombatRound);
	options.enemySighted      = ConfigMan.getBool("enemysighted",      options.enemySighted);
	options.mineSighted       = ConfigMan.getBool("minesighted",       options.mineSighted);
	options.partyMemberDown   = ConfigMan.getBool("partymemberdown",   options.partyMemberDown);
	options.actionMenuUsed    = ConfigMan.getBool("actionmenuused",    options.actionMenuUsed);
	options.newTargetSelected = ConfigMan.getBool("newtargetselected", options.newTargetSelected);

	return options;
}

bool AutoPauseOptions::isEnabled(PauseReason reason) const {
	switch (reason) {
		case kPauseReasonGeneric:
			return true;
		case kPauseReasonCombatRoundEnd:
			return endOfCombatRound;
		case kPauseReasonEnemySighted:
			return enemySighted;
		case kPauseReasonMineSighted:
			return mineSighted;
		case kPauseReasonPartyMemberKilled:
			return partyMemberDown;
		case kPauseReasonActionMenuUsed:
			return actionMenuUsed;
		case kPauseReasonNewTargetSelected:
			return newTargetSelected;
		default:
			break;
	}

	return false;
}

// PauseBlockers

bool PauseBlockers::any() const {
	return conversation || cutscene || minigame || movie;
}

Common::UString getPauseReasonFallbackText(PauseReason reason) {
	switch (reason) {
		case kPauseReasonCombatRoundEnd:
			return "Paused: End of combat round";
		case kPauseReasonEnemySighted:
			return "Paused: Enemy sighted";
		case kPauseReasonMineSighted:
			return "Paused: Mine sighted";
		case kPauseReasonPartyMemberKilled:
			return "Paused: Party member down";
		case kPauseReasonActionMenuUsed:
			return "Paused: Action menu used";
		case kPauseReasonNewTargetSelected:
			return "Paused: New target selected";
		case kPauseReasonGeneric:
		default:
			break;
	}

	return "Paused";
}

uint32_t getPauseReasonStrRef(PauseReason reason) {
	switch (reason) {
		case kPauseReasonGeneric:
			return 1508;
		case kPauseReasonCombatRoundEnd:
			return 42432;
		case kPauseReasonEnemySighted:
			return 42396;
		case kPauseReasonMineSighted:
			return 49116;
		case kPauseReasonPartyMemberKilled:
			return 42397;
		case kPauseReasonActionMenuUsed:
			return 42482;
		case kPauseReasonNewTargetSelected:
			return 42481;
		default:
			break;
	}

	return 0xFFFFFFFF;
}

// PauseState

bool PauseState::isPaused() const {
	return _paused;
}

PauseReason PauseState::getReason() const {
	return _reason;
}

bool PauseState::requestPause(PauseReason reason, const AutoPauseOptions &options,
                              const PauseBlockers &blockers, uint32_t now) {

	if (!options.isEnabled(reason) || blockers.any())
		return _paused;

	if (!_paused)
		_pausedSince = now;

	_paused = true;
	_reason = reason;

	return true;
}

uint32_t PauseState::unpause(uint32_t now) {
	if (!_paused)
		return 0;

	_paused = false;
	_reason = kPauseReasonGeneric;

	return (now > _pausedSince) ? (now - _pausedSince) : 0;
}

uint32_t PauseState::toggle(const PauseBlockers &blockers, uint32_t now) {
	if (_paused)
		return unpause(now);

	requestPause(kPauseReasonGeneric, AutoPauseOptions(), blockers, now);
	return 0;
}

bool PauseState::isWorldFrozen(const PauseBlockers &blockers) const {
	return _paused && !blockers.any();
}

// AutoPauseMonitor

void AutoPauseMonitor::reset() {
	_sightedEnemies.clear();
	_sightedMines.clear();
	_downMembers.clear();
	_leaderTarget = 0;
	_leaderTargetKilled = false;
}

/** Replace @p known with @p current; true if @p current has an ID @p known didn't. */
static bool updateSet(std::set<uint32_t> &known, const std::vector<uint32_t> &current) {
	bool added = false;

	std::set<uint32_t> now(current.begin(), current.end());
	for (uint32_t id : now)
		if (known.find(id) == known.end())
			added = true;

	known.swap(now);
	return added;
}

bool AutoPauseMonitor::updateSightedEnemies(const std::vector<uint32_t> &visibleHostiles, bool leaderInCombat) {
	// An enemy pauses the game the first time it is seen, not each time it
	// steps back into view (around a pillar, at the edge of perception).
	bool added = false;
	for (uint32_t id : visibleHostiles)
		if (_sightedEnemies.insert(id).second)
			added = true;

	return added && !leaderInCombat;
}

bool AutoPauseMonitor::updateSightedMines(const std::vector<uint32_t> &visibleMines) {
	return updateSet(_sightedMines, visibleMines);
}

bool AutoPauseMonitor::updatePartyDown(const std::vector<uint32_t> &downMembers) {
	return updateSet(_downMembers, downMembers);
}

bool AutoPauseMonitor::updateLeaderTarget(uint32_t target, bool leaderInCombat,
                                          const std::function<bool(uint32_t)> &isDead) {

	const uint32_t previous = _leaderTarget;
	_leaderTarget = target;

	if (target == previous)
		return false;

	// The old target may be forgotten (target 0) before a new one is chosen.
	if ((previous != 0) && isDead && isDead(previous))
		_leaderTargetKilled = true;

	if (target == 0) {
		if (!leaderInCombat)
			_leaderTargetKilled = false;
		return false;
	}

	const bool switched = _leaderTargetKilled;
	_leaderTargetKilled = false;

	return switched;
}

} // End of namespace KotORBase

} // End of namespace Engines
