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
 *  Star Wars: Knights of the Old Republic engine functions assigning actions to objects.
 */

#include "external/glm/geometric.hpp"

#include "src/common/error.h"
#include "src/common/util.h"
#include "src/common/string.h"

#include "src/aurora/nwscript/functioncontext.h"
#include "src/aurora/talkman.h"

#include "src/engines/kotorbase/types.h"
#include "src/engines/kotorbase/door.h"
#include "src/engines/kotorbase/item.h"
#include "src/engines/kotorbase/location.h"
#include "src/engines/kotorbase/module.h"
#include "src/engines/kotorbase/objectcontainer.h"
#include "src/engines/kotorbase/game.h"
#include "src/engines/kotorbase/action.h"
#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/talent.h"

#include "src/engines/kotorbase/script/functions.h"

namespace Engines {

namespace KotORBase {

void Functions::getCurrentAction(Aurora::NWScript::FunctionContext &ctx) {
	Creature *object = ObjectContainer::toCreature(ctx.getParams()[0].getObject());
	if (!object) {
		warning("Functions::getCurrentAction(): invalid object");
		ctx.getReturn() = kActionQueueEmpty;
		return;
	}

	const Action *action = object->getCurrentAction();
	if (!action)
		ctx.getReturn() = kActionQueueEmpty;
	else
		ctx.getReturn() = action->type;
}

void Functions::assignCommand(Aurora::NWScript::FunctionContext &ctx) {
	Common::UString script = ctx.getScriptName();
	if (script.empty()) {
		warning("Functions::assignCommand(): missing script name");
		return;
	}

	const Aurora::NWScript::ScriptState &state = ctx.getParams()[1].getScriptState();

	_game->getModule().delayScript(script, state, getParamObject(ctx, 0), ctx.getTriggerer(), 0);
}

void Functions::delayCommand(Aurora::NWScript::FunctionContext &ctx) {
	Common::UString script = ctx.getScriptName();
	if (script.empty()) {
		warning("Functions::delayCommand(): missing script name");
		return;
	}

	uint32_t delay = ctx.getParams()[0].getFloat() * 1000;

	const Aurora::NWScript::ScriptState &state = ctx.getParams()[1].getScriptState();

	_game->getModule().delayScript(script, state, ctx.getCaller(), ctx.getTriggerer(), delay);
}

void Functions::actionStartConversation(Aurora::NWScript::FunctionContext &ctx) {
	const Common::UString &convName = ctx.getParams()[1].getString();
	_game->getModule().startConversation(convName, ctx.getCaller());
}

void Functions::actionOpenDoor(Aurora::NWScript::FunctionContext &ctx) {
	Aurora::NWScript::Object *object = ctx.getParams()[0].getObject();
	Door *door = ObjectContainer::toDoor(object);
	if (!door) {
		warning("Functions::actionOpenDoor(): object is not a door");
		return;
	}

	door->open(0);
}

void Functions::actionCloseDoor(Aurora::NWScript::FunctionContext &ctx) {
	Aurora::NWScript::Object *object = ctx.getParams()[0].getObject();
	Door *door = ObjectContainer::toDoor(object);
	if (!door) {
		warning("Functions::actionCloseDoor(): object is not a door");
		return;
	}

	door->close(0);
}

void Functions::actionMoveToObject(Aurora::NWScript::FunctionContext &ctx) {
	Creature *caller = ObjectContainer::toCreature(ctx.getCaller());
	if (!caller) {
		warning("Functions::actionMoveToObject(): invalid caller");
		return;
	}

	Object *object = ObjectContainer::toObject(ctx.getParams()[0].getObject());
	if (!object)
		object = _game->getModule().getPartyLeader();
	if (!object) {
		warning("Functions::actionMoveToObject(): no target object or party leader");
		return;
	}

	float range = ctx.getParams()[2].getFloat();

	float x, y, z;
	object->getPosition(x, y, z);

	Action action(kActionMoveToPoint);
	action.range = range;
	action.location = glm::vec3(x, y, z);

	caller->addAction(action);
}

void Functions::actionMoveToLocation(Aurora::NWScript::FunctionContext &ctx) {
	// ActionMoveToLocation(location lDestination, int bRun=FALSE)
	// Queue a move-to-point action targeting the given Location engine type.
	Creature *caller = ObjectContainer::toCreature(ctx.getCaller());
	if (!caller)
		return;

	Location *dest = ObjectContainer::toLocation(ctx.getParams()[0].getEngineType());
	if (!dest)
		return;

	float x, y, z;
	dest->getPosition(x, y, z);

	Action action(kActionMoveToPoint);
	action.range = 0.1f; // arrive within 0.1 units of the target location
	action.location = glm::vec3(x, y, z);

	caller->addAction(action);
}

void Functions::actionRandomWalk(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	// Graceful no-op for scripts that only request ambient movement.
}

void Functions::actionMoveAwayFromObject(Aurora::NWScript::FunctionContext &ctx) {
	Creature *caller = ObjectContainer::toCreature(ctx.getCaller());
	Object *target = ObjectContainer::toObject(ctx.getParams()[0].getObject());
	if (!caller || !target)
		return;

	float callerX, callerY, callerZ;
	float targetX, targetY, targetZ;
	caller->getPosition(callerX, callerY, callerZ);
	target->getPosition(targetX, targetY, targetZ);

	const float distance = MAX<float>(1.0f, ctx.getParams()[2].getFloat());
	glm::vec3 away(callerX - targetX, callerY - targetY, callerZ - targetZ);
	if (glm::length(away) < 0.001f)
		away = glm::vec3(1.0f, 0.0f, 0.0f);
	away = glm::normalize(away) * distance;

	Action action(kActionMoveToPoint);
	action.range = 0.1f;
	action.location = glm::vec3(callerX, callerY, callerZ) + away;
	caller->addAction(action);
}

void Functions::actionForceMoveToObject(Aurora::NWScript::FunctionContext &ctx) {
	// Alias of ActionMoveToObject for our simple pathfinding
	actionMoveToObject(ctx);
}

void Functions::actionForceMoveToLocation(Aurora::NWScript::FunctionContext &ctx) {
	// Alias of ActionMoveToLocation
	actionMoveToLocation(ctx);
}

void Functions::actionFollowLeader(Aurora::NWScript::FunctionContext &ctx) {
	Creature *caller = ObjectContainer::toCreature(ctx.getCaller());
	if (!caller) {
		warning("Functions::actionFollowLeader(): invalid caller");
		return;
	}

	Action action(kActionFollowLeader);
	action.range = 1.0f;

	caller->addAction(action);
}

void Functions::cancelCombat(Aurora::NWScript::FunctionContext &ctx) {
	Creature *caller = ObjectContainer::toCreature(ctx.getCaller());
	if (caller)
		caller->cancelCombat();
}

void Functions::getAttemptedAttackTarget(Aurora::NWScript::FunctionContext &ctx) {
	Creature *creature = ObjectContainer::toCreature(ctx.getParams()[0].getObject());
	if (creature)
		ctx.getReturn() = (Aurora::NWScript::Object *) creature->getAttemptedAttackTarget();
	else
		ctx.getReturn() = (Aurora::NWScript::Object *) nullptr;
}

void Functions::getAttemptedSpellTarget(Aurora::NWScript::FunctionContext &ctx) {
	Creature *creature = ObjectContainer::toCreature(getParamObject(ctx, 0));
	if (creature)
		ctx.getReturn() = (Aurora::NWScript::Object *) creature->getAttemptedAttackTarget();
	else
		ctx.getReturn() = (Aurora::NWScript::Object *) nullptr;
}


void Functions::clearAllActions(Aurora::NWScript::FunctionContext &ctx) {
	// ClearAllActions operates on the calling object only.  Falling back to the
	// party leader when the caller is null would clear the wrong creature's queue
	// during cinematic sequences where NPCs (not the PC) are the callers.
	Creature *caller = ObjectContainer::toCreature(ctx.getCaller());
	if (!caller)
		return;

	caller->clearActions();
}

void Functions::setFacing(Aurora::NWScript::FunctionContext &ctx) {
	float facing = ctx.getParams()[0].getFloat();

	Object *object = ObjectContainer::toObject(ctx.getCaller());
	if (object)
		object->setOrientation(0.0f, 0.0f, 1.0f, facing);
}

void Functions::actionEquipItem(Aurora::NWScript::FunctionContext &ctx) {
	Item *item = dynamic_cast<Item *>(ObjectContainer::toObject(ctx.getParams()[0].getObject()));
	int slot = ctx.getParams()[1].getInt();

	Creature *caller = ObjectContainer::toCreature(ctx.getCaller());
	if (!caller || !item)
		return;

	Common::UString resRef = item->getTemplateResRef();
	if (resRef.empty())
		resRef = item->getTag();

	caller->equipItem(resRef, static_cast<InventorySlot>(slot));
}

void Functions::actionUnequipItem(Aurora::NWScript::FunctionContext &ctx) {
	Item *item = dynamic_cast<Item *>(ObjectContainer::toObject(ctx.getParams()[0].getObject()));

	Creature *caller = ObjectContainer::toCreature(ctx.getCaller());
	if (!caller || !item)
		return;

	// In the real engine this would be queued, but we can safely handle it directly.
	for (int i = static_cast<int>(kInventorySlotHead); i < static_cast<int>(kInventorySlotMAX); ++i) {
		InventorySlot slot = static_cast<InventorySlot>(i);
		if (caller->getEquipedItem(slot) == item) {
			caller->equipItem("", slot);
			break;
		}
	}
}

void Functions::actionPickUpItem(Aurora::NWScript::FunctionContext &ctx) {
	// Pick up an item from a container or the ground into caller's inventory.
	// For the Endar Spire milestone, items are primarily created via
	// CreateItemOnObject, so this is a lightweight queue-based stub.
	Object *itemObj = ObjectContainer::toObject(ctx.getParams()[0].getObject());
	Creature *caller = ObjectContainer::toCreature(ctx.getCaller());
	if (!caller || !itemObj)
		return;

	Action action(kActionPickUpItem);
	action.object = itemObj;
	action.range  = 1.5f;   // Must be within 1.5 units to pick up
	caller->addAction(action);
}

void Functions::actionAttack(Aurora::NWScript::FunctionContext &ctx) {
	Object *target = ObjectContainer::toObject(ctx.getParams()[0].getObject());
	Creature *caller = ObjectContainer::toCreature(ctx.getCaller());
	if (!caller || !target)
		return;

	Action action(kActionAttackObject);
	action.object = target;
	caller->addAction(action);
}

void Functions::getLastAttacker(Aurora::NWScript::FunctionContext &ctx) {
	Creature *creature = ObjectContainer::toCreature(ctx.getParams()[0].getObject());
	ctx.getReturn() = creature ? creature->getLastHostileActor()
	                           : static_cast<Aurora::NWScript::Object *>(nullptr);
}

void Functions::actionSpeakString(Aurora::NWScript::FunctionContext &ctx) {
	// In the original engine, ActionSpeakString queues a spoken line above
	// the creature's head. We log it so script logic is not silently skipped.
	const Common::UString &str = ctx.getParams()[0].getString();

	Object *caller = ObjectContainer::toObject(ctx.getCaller());
	const Common::UString tag = caller ? caller->getTag() : Common::UString("(unknown)");

	status("ActionSpeakString [%s]: %s", tag.c_str(), str.c_str());
}

void Functions::speakString(Aurora::NWScript::FunctionContext &ctx) {
	actionSpeakString(ctx);
}

void Functions::actionSpeakStringByStrRef(Aurora::NWScript::FunctionContext &ctx) {
	const uint32_t strRef = static_cast<uint32_t>(ctx.getParams()[0].getInt());
	Common::UString text = TalkMan.getString(strRef);
	if (text.empty())
		text = Common::String::format("<strref:%u>", strRef);

	Object *caller = ObjectContainer::toObject(ctx.getCaller());
	const Common::UString who = caller ? caller->getTag() : Common::UString("(unknown)");
	status("ActionSpeakStringByStrRef [%s]: %s", who.c_str(), text.c_str());
}

static Common::UString animIDToName(int animID) {
	switch (animID) {
		case  0: return "pause1";      // ANIMATION_LOOPING_PAUSE
		case  1: return "pause2";      // ANIMATION_LOOPING_PAUSE2
		case  2: return "listen";      // ANIMATION_LOOPING_LISTEN
		case  3: return "meditate";    // ANIMATION_LOOPING_MEDITATE
		case  4: return "worship";     // ANIMATION_LOOPING_WORSHIP
		case  5: return "drunk";       // ANIMATION_LOOPING_DRUNK
		case  6: return "talk_injured"; // ANIMATION_LOOPING_TALK_INJURED
		case  7: return "listen_injured"; // ANIMATION_LOOPING_LISTEN_INJURED
		case  8: return "treatinjury"; // ANIMATION_LOOPING_TREAT_INJURY
		case  9: return "getlow";      // ANIMATION_LOOPING_GET_LOW
		case 10: return "talk";        // ANIMATION_LOOPING_TALK_NORMAL
		case 11: return "talklooking"; // ANIMATION_LOOPING_TALK_PLEADING
		case 12: return "deadf";       // ANIMATION_LOOPING_DEAD_FRONT
		case 13: return "deadb";       // ANIMATION_LOOPING_DEAD_BACK
		case 14: return "conjure1";    // ANIMATION_LOOPING_CONJURE1
		case 15: return "conjure2";    // ANIMATION_LOOPING_CONJURE2
		case 16: return "victory1";    // ANIMATION_LOOPING_VICTORY1
		case 17: return "victory2";    // ANIMATION_LOOPING_VICTORY2
		case 18: return "victory3";    // ANIMATION_LOOPING_VICTORY3
		case 19: return "getmid";      // ANIMATION_LOOPING_GET_MID
		case 38: return "attack1";     // ANIMATION_FIREFORGET_ATTACK1
		case 39: return "attack2";     // ANIMATION_FIREFORGET_ATTACK2
		case 40: return "dodge";       // ANIMATION_FIREFORGET_DODGE
		case 41: return "attack3";     // ANIMATION_FIREFORGET_ATTACK3
		case 44: return "die";         // ANIMATION_FIREFORGET_SPASM
		case 45: return "dead";        // ANIMATION_FIREFORGET_DEAD (fall)
		case 48: return "g8a1";        // ANIMATION_FIREFORGET_DODGE_DUCK
		case 49: return "g8a2";        // ANIMATION_FIREFORGET_DODGE_SIDE
		case 56: return "castout";     // ANIMATION_FIREFORGET_CAST_OUT_HAND
		case 57: return "castin";      // ANIMATION_FIREFORGET_CAST_IN_HAND
		case 58: return "castarea";    // ANIMATION_FIREFORGET_CAST_AREA
		default: return "";            // Unknown; caller ignores empty string
	}
}

void Functions::actionPlayAnimation(Aurora::NWScript::FunctionContext &ctx) {
	// ActionPlayAnimation queues after pending actions (action form).
	Creature *caller = ObjectContainer::toCreature(ctx.getCaller());
	if (!caller)
		return;

	const int animID = ctx.getParams()[0].getInt();
	const Common::UString animName = animIDToName(animID);
	if (animName.empty())
		return;

	float speed  = ctx.getParams()[1].getFloat();
	float length = ctx.getParams()[2].getFloat();
	caller->playAnimation(animName, true, length, speed > 0.0f ? speed : 1.0f);
}

void Functions::playAnimation(Aurora::NWScript::FunctionContext &ctx) {
	// PlayAnimation is the immediate form: it fires now regardless of the action
	// queue.  Uses the same ID→name table but bypasses any pending action state.
	Creature *caller = ObjectContainer::toCreature(ctx.getCaller());
	if (!caller)
		return;

	const int animID = ctx.getParams()[0].getInt();
	const Common::UString animName = animIDToName(animID);
	if (animName.empty())
		return;

	float speed  = ctx.getParams()[1].getFloat();
	float length = ctx.getParams()[2].getFloat();
	// restart=false: immediate play does not loop-restart a running animation.
	caller->playAnimation(animName, false, length, speed > 0.0f ? speed : 1.0f);
}

void Functions::actionJumpToObject(Aurora::NWScript::FunctionContext &ctx) {
	// Immediately teleport the caller to the position of the target object.
	// This is the action-queued variant of JumpToObject; for the Endar Spire
	// milestone both variants behave identically (instant teleport, no path).
	jumpToObject(ctx);
}

void Functions::actionJumpToLocation(Aurora::NWScript::FunctionContext &ctx) {
	// Immediately teleport the caller to the given location.
	// Delegates to the already-implemented JumpToLocation function.
	jumpToLocation(ctx);
}

void Functions::actionPauseConversation(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
}

void Functions::actionResumeConversation(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
}

void Functions::actionInteractObject(Aurora::NWScript::FunctionContext &ctx) {
	Object *target = ObjectContainer::toObject(ctx.getParams()[0].getObject());
	Creature *caller = ObjectContainer::toCreature(ctx.getCaller());
	if (!caller || !target)
		return;

	Action action(kActionUseObject);
	action.object = target;
	action.range = 1.5f;
	caller->addAction(action);
}

void Functions::actionUseFeat(Aurora::NWScript::FunctionContext &ctx) {
	Creature *caller = ObjectContainer::toCreature(ctx.getCaller());
	Object *target = ObjectContainer::toObject(ctx.getParams()[1].getObject());
	if (!caller)
		return;

	const int featID = ctx.getParams()[0].getInt();
	caller->setLastCombatFeatUsed(featID);

	if (target) {
		caller->setAttemptedAttackTarget(target);

		Action action(kActionAttackObject);
		action.object = target;
		caller->addAction(action);
	}
}

void Functions::actionUseTalentOnObject(Aurora::NWScript::FunctionContext &ctx) {
	const Talent *talent = ObjectContainer::toTalent(ctx.getParams()[0].getEngineType());
	Object *target = ObjectContainer::toObject(ctx.getParams()[1].getObject());
	Creature *caller = ObjectContainer::toCreature(ctx.getCaller());
	if (!caller || !talent || !talent->isValid())
		return;

	if (talent->getType() == kTalentTypeFeat) {
		caller->setLastCombatFeatUsed(talent->getID());
		if (target) {
			caller->setAttemptedAttackTarget(target);

			Action action(kActionAttackObject);
			action.object = target;
			caller->addAction(action);
		}
		return;
	}

	if (talent->getType() == kTalentTypeSkill && target) {
		Action action(kActionUseObject);
		action.object = target;
		action.range = 1.5f;
		caller->addAction(action);
	}
}

void Functions::actionUseTalentAtLocation(Aurora::NWScript::FunctionContext &ctx) {
	const Talent *talent = ObjectContainer::toTalent(ctx.getParams()[0].getEngineType());
	Location *target = ObjectContainer::toLocation(ctx.getParams()[1].getEngineType());
	Creature *caller = ObjectContainer::toCreature(ctx.getCaller());
	if (!caller || !talent || !talent->isValid() || !target)
		return;

	if (talent->getType() == kTalentTypeFeat)
		caller->setLastCombatFeatUsed(talent->getID());

	float x, y, z;
	target->getPosition(x, y, z);

	Action action(kActionMoveToPoint);
	action.range = 0.1f;
	action.location = glm::vec3(x, y, z);
	caller->addAction(action);
}

void Functions::actionWait(Aurora::NWScript::FunctionContext &ctx) {
	// Queue a wait action with the specified duration (seconds).
	// The action executor pops it immediately after the first tick; this is
	// sufficient for any script that only needs to yield briefly.
	Creature *caller = ObjectContainer::toCreature(ctx.getCaller());
	if (!caller)
		return;

	// Store the wait duration in the `range` field (repurposed as a float
	// payload; the executor currently completes the wait on the first tick).
	Action action(kActionWait);
	action.range = ctx.getParams()[0].getFloat();
	caller->addAction(action);
}

void Functions::getUserActionsPending(Aurora::NWScript::FunctionContext &ctx) {
	// Returns TRUE if the calling creature has any actions in its queue.
	// Cinematic scripts use this in wait loops:
	//   while (GetUserActionsPending()) { ActionWait(0.1); }
	// Returning a hardcoded 0 caused those loops to never execute.
	Creature *caller = ObjectContainer::toCreature(ctx.getCaller());
	ctx.getReturn() = (caller && caller->getCurrentAction()) ? 1 : 0;
}

void Functions::noClicksFor(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
}

void Functions::actionPutDownItem(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	// Inventory dropping is not yet modeled; keep script flow alive.
}

void Functions::actionCastSpellAtObject(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	// Spell casting is currently out of scope for this progression slice.
}

void Functions::actionBarkString(Aurora::NWScript::FunctionContext &ctx) {
	const uint32_t strRef = static_cast<uint32_t>(ctx.getParams()[0].getInt());
	Common::UString text = TalkMan.getString(strRef);
	if (text.empty())
		text = Common::String::format("<strref:%u>", strRef);

	Object *caller = ObjectContainer::toObject(ctx.getCaller());
	const Common::UString who = caller ? caller->getTag() : Common::UString("(unknown)");
	status("ActionBarkString [%s]: %s", who.c_str(), text.c_str());
}

} // End of namespace KotORBase

} // End of namespace Engines
