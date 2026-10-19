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
 *  Which objects stand inside a trigger, independent of the game world.
 *
 *  Every trigger keeps its own set of occupants (by object ID), so
 *  overlapping triggers each see their own OnEnter/OnExit, and any creature
 *  (not only the party leader) can enter and leave them.
 */

#ifndef ENGINES_KOTORBASE_TRIGGEROCCUPANCY_H
#define ENGINES_KOTORBASE_TRIGGEROCCUPANCY_H

#include <cstddef>
#include <vector>

#include "src/common/types.h"

namespace Engines {

namespace KotORBase {

class TriggerOccupancy {
public:
	enum Change {
		kChangeNone,    ///< Still inside, or still outside.
		kChangeEntered, ///< Was outside, is inside now: run OnEnter.
		kChangeExited   ///< Was inside, is outside now: run OnExit.
	};

	/** Record where the object is now, and return what that changed. */
	Change update(uint32_t id, bool inside);

	/** Is the object inside? */
	bool contains(uint32_t id) const;

	/** Put the object inside without an event (it was there when the area loaded). */
	bool addSilently(uint32_t id);
	/** Forget the object without an event (it was destroyed or left the area). */
	bool removeSilently(uint32_t id);

	void clear();

	size_t size() const;
	bool empty() const;
	/** The objects inside, in the order they entered. */
	const std::vector<uint32_t> &getOccupants() const;

private:
	std::vector<uint32_t> _occupants;
};

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_TRIGGEROCCUPANCY_H
