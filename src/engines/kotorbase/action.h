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
 *  Creature action in KotOR games.
 */

#ifndef ENGINES_KOTORBASE_ACTION_H
#define ENGINES_KOTORBASE_ACTION_H

#include "external/glm/vec3.hpp"

#include "src/engines/kotorbase/types.h"

namespace Engines {

namespace KotORBase {

class Object;

struct Action {
	ActionType type;
	glm::vec3 location;
	Object *object;
	float range;
	int   actionID { -1 }; ///< Spell, feat or power ID.
	int   choreographyFlags { 0 }; ///< Cinematic attack modifiers (CutsceneAttack).
	float startTime { -1.0f }; ///< Time the action started (for timed actions).
	bool  initialized { false };

	Action();
	Action(ActionType type);
	Action(ActionType type, int id);
	Action(ActionType type, Object *target, int flags = 0);
};

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_ACTION_H
