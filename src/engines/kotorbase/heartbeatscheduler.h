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
 *
 *  Every creature, placeable, door and trigger gets a heartbeat once per
 *  round (6 seconds, the Aurora round the module and area heartbeats use
 *  too). Each object has its own phase within the round, derived from its
 *  ID, so the heartbeats of an area are spread out instead of all running
 *  on the same frame.
 */

#ifndef ENGINES_KOTORBASE_HEARTBEATSCHEDULER_H
#define ENGINES_KOTORBASE_HEARTBEATSCHEDULER_H

#include <map>
#include <vector>

#include "src/common/types.h"

namespace Engines {

namespace KotORBase {

class HeartbeatScheduler {
public:
	/** Time between two heartbeats of the same object (ms). */
	static const uint32_t kInterval = 6000;

	explicit HeartbeatScheduler(uint32_t interval = kInterval);

	uint32_t getInterval() const;

	/** Start the heartbeats of an object; the first is due within one interval. */
	void add(uint32_t id, uint32_t now);
	/** Stop the heartbeats of an object. */
	void remove(uint32_t id);
	bool contains(uint32_t id) const;
	size_t size() const;
	void clear();

	/** Append the objects whose heartbeat is due, and schedule their next one. */
	void collectDue(uint32_t now, std::vector<uint32_t> &due);
	/** Push every heartbeat back by @p ms. */
	void delay(uint32_t ms);

	/** When, within an interval, an object's heartbeats fall (ms, below interval). */
	static uint32_t getPhase(uint32_t id, uint32_t interval);

private:
	uint32_t _interval;
	std::map<uint32_t, uint32_t> _due; ///< Object ID -> timestamp of its next heartbeat.
};

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_HEARTBEATSCHEDULER_H
