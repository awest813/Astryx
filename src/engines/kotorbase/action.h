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

#include <memory>

#include "external/glm/vec3.hpp"

#include "src/common/ustring.h"

#include "src/engines/kotorbase/types.h"

namespace Aurora {
	namespace NWScript {
		struct ScriptState;
	}
}

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
	int   damage { 0 };            ///< CutsceneAttack: damage dealt when the blow lands.
	float startTime { -1.0f }; ///< Time the action started (for timed actions).
	bool  initialized { false };
	bool  cheat { false };     ///< CastSpell: bCheat, cast without spending Force points.
	bool  fake { false };      ///< CastSpell: ActionCastFakeSpell*, only the casting motions.
	float timeout { 0.0f };    ///< MoveToPoint: ActionForceMove*'s fTimeout, jump there after it.
	int   step { 0 };          ///< Progress of an action done in steps (random walk, lock picking, mines).
	glm::vec3 origin { 0.0f, 0.0f, 0.0f }; ///< RandomWalk: where the walk started.
	/** Inventory tag of the item involved: OpenLock, the security tunneler used (its
	 *  Thieves' Tools bonus applies); CastSpell, the item casting the spell. */
	Common::UString item;
	bool  consumeItem { false }; ///< CastSpell: use up one of @c item from the inventory.

	/** kActionDoCommand: the script closure to run when the action is reached. */
	Common::UString script;
	std::shared_ptr<Aurora::NWScript::ScriptState> scriptState;

	Action();
	Action(ActionType type);
	Action(ActionType type, int id);
	Action(ActionType type, Object *target, int flags = 0);
};

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_ACTION_H
