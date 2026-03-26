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
 *  Shared Endar Spire NWScript helpers used by the runtime and regression tests.
 */

#ifndef ENGINES_KOTORBASE_SCRIPT_ENDAR_SPIRE_SUPPORT_H
#define ENGINES_KOTORBASE_SCRIPT_ENDAR_SPIRE_SUPPORT_H

#include "external/glm/vec3.hpp"

#include "src/engines/kotorbase/action.h"
#include "src/engines/kotorbase/location.h"
#include "src/engines/kotorbase/object.h"
#include "src/engines/kotorbase/types.h"

namespace Engines {

namespace KotORBase {

namespace EndarSpireSupport {

inline bool isFactionHostile(Faction faction) {
	return faction == kFactionHostile1 ||
	       faction == kFactionHostile2 ||
	       faction == kFactionEndarSpire;
}

inline bool isFactionFriendly(Faction faction) {
	return faction == kFactionFriendly1 ||
	       faction == kFactionFriendly2;
}

inline bool areFactionsEnemy(Faction targetFaction, Faction sourceFaction) {
	return (isFactionHostile(targetFaction) && isFactionFriendly(sourceFaction)) ||
	       (isFactionHostile(sourceFaction) && isFactionFriendly(targetFaction));
}

inline bool areFactionsFriendly(Faction targetFaction, Faction sourceFaction) {
	return (targetFaction != kFactionInvalid && targetFaction == sourceFaction) ||
	       (isFactionFriendly(targetFaction) && isFactionFriendly(sourceFaction));
}

inline bool areFactionsNeutral(Faction targetFaction, Faction sourceFaction) {
	return !areFactionsEnemy(targetFaction, sourceFaction) &&
	       !areFactionsFriendly(targetFaction, sourceFaction);
}

inline Action makeMoveToLocationAction(const Location &destination) {
	float x, y, z;
	destination.getPosition(x, y, z);

	Action action(kActionMoveToPoint);
	action.range = 0.1f;
	action.location = glm::vec3(x, y, z);
	return action;
}

inline void applyLocationToObject(Object &object, const Location &destination) {
	float x, y, z;
	destination.getPosition(x, y, z);

	object.setPosition(x, y, z);
	object.setOrientation(0.0f, 0.0f, 1.0f, destination.getFacing());
}

inline const char *getActionAnimationName(int animationID) {
	switch (animationID) {
		case  0: return "pause1";
		case  1: return "pause2";
		case  2: return "listen";
		case  3: return "meditate";
		case  4: return "worship";
		case 10: return "talk";
		case 11: return "talklooking";
		case 16: return "victory1";
		case 17: return "victory2";
		case 38: return "attack1";
		case 39: return "attack2";
		case 40: return "dodge";
		case 44: return "die";
		case 48: return "g8a1";
		case 49: return "g8a2";
		default: return nullptr;
	}
}

} // End of namespace EndarSpireSupport

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_SCRIPT_ENDAR_SPIRE_SUPPORT_H
