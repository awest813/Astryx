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
 *  Action queue for creatures in KotOR games.
 */

#include "src/engines/kotorbase/actionqueue.h"

namespace Engines {

namespace KotORBase {

Action *ActionQueue::getCurrent() {
	return _actions.empty() ? nullptr : &_actions.front();
}

const Action *ActionQueue::getCurrent() const {
	return _actions.empty() ? nullptr : &_actions.front();
}

void ActionQueue::clear() {
	_actions = std::queue<Action>();
}

void ActionQueue::add(const Action &action) {
	_actions.push(action);
}

void ActionQueue::removeActionsTargeting(const Object *object) {
	std::queue<Action> kept;
	for (; !_actions.empty(); _actions.pop())
		if (_actions.front().object != object)
			kept.push(_actions.front());

	_actions.swap(kept);
}

void ActionQueue::pop() {
	// Scripts run by the current action may already have cleared the queue.
	if (!_actions.empty())
		_actions.pop();
}

} // End of namespace KotORBase

} // End of namespace Engines
