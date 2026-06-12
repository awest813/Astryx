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
 *  Party roster snapshot for KotOR save games.
 */

#ifndef ENGINES_KOTORBASE_PARTYSTATE_H
#define ENGINES_KOTORBASE_PARTYSTATE_H

#include "src/common/ustring.h"

#include "src/engines/kotorbase/creatureinfo.h"

namespace Engines {

namespace KotORBase {

struct SavedPartyMemberState {
	int npcSlot { -1 };
	Common::UString templateResRef;
	CreatureInfo info;
};

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_PARTYSTATE_H
