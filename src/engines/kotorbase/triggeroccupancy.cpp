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
 */

#include <algorithm>

#include "src/engines/kotorbase/triggeroccupancy.h"

namespace Engines {

namespace KotORBase {

TriggerOccupancy::Change TriggerOccupancy::update(uint32_t id, bool inside) {
	if (inside)
		return addSilently(id) ? kChangeEntered : kChangeNone;

	return removeSilently(id) ? kChangeExited : kChangeNone;
}

bool TriggerOccupancy::contains(uint32_t id) const {
	return std::find(_occupants.begin(), _occupants.end(), id) != _occupants.end();
}

bool TriggerOccupancy::addSilently(uint32_t id) {
	if (contains(id))
		return false;

	_occupants.push_back(id);
	return true;
}

bool TriggerOccupancy::removeSilently(uint32_t id) {
	auto it = std::find(_occupants.begin(), _occupants.end(), id);
	if (it == _occupants.end())
		return false;

	_occupants.erase(it);
	return true;
}

void TriggerOccupancy::clear() {
	_occupants.clear();
}

size_t TriggerOccupancy::size() const {
	return _occupants.size();
}

bool TriggerOccupancy::empty() const {
	return _occupants.empty();
}

const std::vector<uint32_t> &TriggerOccupancy::getOccupants() const {
	return _occupants;
}

} // End of namespace KotORBase

} // End of namespace Engines
