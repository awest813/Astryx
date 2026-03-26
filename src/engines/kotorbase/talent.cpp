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
 *  Talent engine type for KotOR games.
 */

#include "src/engines/kotorbase/talent.h"

namespace Engines {

namespace KotORBase {

Talent::Talent(TalentType type, int id) :
		_type(type),
		_id(id) {
}

bool Talent::isValid() const {
	return ((_type == kTalentTypeSpell) ||
	        (_type == kTalentTypeFeat)  ||
	        (_type == kTalentTypeSkill)) && (_id >= 0);
}

TalentType Talent::getType() const {
	return _type;
}

int Talent::getID() const {
	return _id;
}

int Talent::getCategory() const {
	return _type;
}

Aurora::NWScript::EngineType *Talent::clone() const {
	return new Talent(_type, _id);
}

} // End of namespace KotORBase

} // End of namespace Engines
