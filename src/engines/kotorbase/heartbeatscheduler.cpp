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
 *  When the objects of an area get their OnHeartbeat, independent of the game world.
 */

#include "src/engines/kotorbase/heartbeatscheduler.h"

namespace Engines {

namespace KotORBase {

const uint32_t HeartbeatScheduler::kInterval;

HeartbeatScheduler::HeartbeatScheduler(uint32_t interval) : _interval(interval > 0 ? interval : 1) {
}

uint32_t HeartbeatScheduler::getInterval() const {
	return _interval;
}

uint32_t HeartbeatScheduler::getPhase(uint32_t id, uint32_t interval) {
	if (interval == 0)
		return 0;

	// Multiplicative hashing: objects created one after the other (consecutive
	// IDs) land far apart within the interval.
	return static_cast<uint32_t>((static_cast<uint64_t>(id) * 2654435761ULL) % interval);
}

void HeartbeatScheduler::add(uint32_t id, uint32_t now) {
	if (_due.count(id))
		return;

	// The next moment that has this object's phase, strictly in the future.
	const uint32_t phase = getPhase(id, _interval);
	const uint32_t current = now % _interval;
	const uint32_t wait = (phase > current) ? (phase - current) : (_interval - current + phase);

	_due[id] = now + wait;
}

void HeartbeatScheduler::remove(uint32_t id) {
	_due.erase(id);
}

bool HeartbeatScheduler::contains(uint32_t id) const {
	return _due.count(id) != 0;
}

size_t HeartbeatScheduler::size() const {
	return _due.size();
}

void HeartbeatScheduler::clear() {
	_due.clear();
}

void HeartbeatScheduler::collectDue(uint32_t now, std::vector<uint32_t> &due) {
	for (auto &entry : _due) {
		if (now < entry.second)
			continue;

		due.push_back(entry.first);

		// Keep the phase. After a long stall (loading, a modal menu), skip the
		// missed heartbeats rather than running them all at once.
		const uint32_t missed = (now - entry.second) / _interval + 1;
		entry.second += missed * _interval;
	}
}

} // End of namespace KotORBase

} // End of namespace Engines
