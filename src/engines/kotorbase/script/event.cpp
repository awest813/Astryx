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
 *  Event engine type for KotOR games.
 */

#include "src/engines/kotorbase/script/event.h"

namespace Engines {

namespace KotORBase {

Event::Event(EventType type) :
		_type(type),
		_userDefinedNumber(0),
		_spellId(-1),
		_spellHarmful(false) {
}

void Event::setUserDefinedNumber(int number) {
	_userDefinedNumber = number;
}

void Event::setSpellId(int spellId) {
	_spellId = spellId;
}

void Event::setSpellHarmful(bool harmful) {
	_spellHarmful = harmful;
}

void Event::setActivateItem(Aurora::NWScript::Object *item, Aurora::NWScript::Object *activator,
                            Aurora::NWScript::Object *target, const Location &targetLocation) {
	_item = item;
	_activator = activator;
	_target = target;
	_targetLocation = targetLocation;
}

Aurora::NWScript::Object *Event::getItem() const {
	return *_item;
}

Aurora::NWScript::Object *Event::getActivator() const {
	return *_activator;
}

Aurora::NWScript::Object *Event::getTarget() const {
	return *_target;
}

const Location &Event::getTargetLocation() const {
	return _targetLocation;
}

EventType Event::getType() const {
	return _type;
}

int Event::getUserDefinedNumber() const {
	return _userDefinedNumber;
}

int Event::getSpellId() const {
	return _spellId;
}

bool Event::getSpellHarmful() const {
	return _spellHarmful;
}

Aurora::NWScript::EngineType *Event::clone() const {
	Event *e = new Event(_type);
	e->_userDefinedNumber = _userDefinedNumber;
	e->_spellId = _spellId;
	e->_spellHarmful = _spellHarmful;
	e->_item = _item;
	e->_activator = _activator;
	e->_target = _target;
	e->_targetLocation = _targetLocation;
	return e;
}

} // End of namespace KotORBase

} // End of namespace Engines
