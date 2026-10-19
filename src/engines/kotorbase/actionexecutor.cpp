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
 *  Creature action executor for KotOR games.
 */

#include <cfloat>

#include "external/glm/vec3.hpp"
#include "external/glm/gtc/type_ptr.hpp"

#include "src/common/util.h"
#include "src/common/maths.h"

#include "src/engines/kotorbase/creature.h"
#include "src/aurora/nwscript/objectman.h"

#include "src/engines/kotorbase/actionexecutor.h"
#include "src/common/random.h"
#include "src/common/debug.h"
#include "src/aurora/2dafile.h"
#include "src/aurora/2dareg.h"

#include "src/engines/kotorbase/area.h"
#include "src/engines/kotorbase/item.h"
#include "src/engines/kotorbase/itemactions.h"
#include "src/engines/kotorbase/module.h"
#include "src/engines/kotorbase/door.h"
#include "src/engines/kotorbase/placeable.h"
#include "src/engines/kotorbase/inventory.h"
#include "src/engines/kotorbase/objectcontainer.h"
#include "src/engines/kotorbase/script/container.h"
#include "src/engines/kotorbase/animationnames.h"
#include "src/engines/kotorbase/trigger.h"
#include "src/engines/kotorbase/trap.h"
#include "src/engines/kotorbase/talent.h"

static const float kWalkDistance = 2.0f;
/** Closer than this counts as arrived; a range of 0 is never hit exactly. */
static const float kMinMoveRange = 0.1f;
/** Give up on a blocked movement after this many seconds. */
static const float kMaxBlockedMoveTime = 3.0f;
/** A chased goal that moved this far (m) gets a new route. */
static const float kRepathDistance = 1.0f;
/** Routes are redone this often (s), so they follow doors opening and closing. */
static const float kRepathTime = 3.0f;
/** A blocked route is redone after this long (s). */
static const float kRepathWhenBlocked = 0.5f;
/** A waypoint closer than this (m) counts as reached. */
static const float kWaypointReach = 0.3f;

/** How long (s) picking a lock takes. */
static const float kUnlockTime = 1.5f;
/** How close (m) a creature walks up to a door or container to lock it. */
static const float kLockRange = 2.0f;
/** ActionForceFollowObject never comes closer than this (m). */
static const float kMinFollowDistance = 1.0f;

/** ActionRandomWalk: destinations lie this far (m) from where the walk started. */
static const float kRandomWalkMinDistance = 2.0f;
static const float kRandomWalkMaxDistance = 8.0f;
/** ActionRandomWalk: pause (s) between two walks. */
static const float kRandomWalkMinPause = 1.0f;
static const float kRandomWalkMaxPause = 4.0f;
/** ActionRandomWalk: destinations tried per walk. */
static const int   kRandomWalkAttempts = 4;
/** ActionRandomWalk: closer than this (m) to the destination counts as there. */
static const float kRandomWalkReach = 0.5f;

/** Item property: Thieves' Tools (security tunnelers), itempropdef.2da. */
static const int kItemPropertyThievesTools = 37;

namespace Engines {

namespace KotORBase {
std::map<uint32_t, ActionExecutor::SpellInfo> ActionExecutor::_spells;
bool ActionExecutor::_spellsLoaded = false;


void ActionExecutor::execute(Action &action, const ExecutionContext &ctx) {
	if (!action.initialized) {
		if (action.type != kActionPlayAnimation)
			action.startTime = 0.0f;
		action.initialized = true;
	}

	switch (action.type) {
		case kActionMoveToPoint:
			executeMoveToPoint(action, ctx);
			break;
		case kActionFollowLeader:
			executeFollowLeader(action, ctx);
			break;
		case kActionOpenLock:
			executeOpenLock(action, ctx);
			break;
		case kActionLock:
			executeLock(action, ctx);
			break;
		case kActionFollow:
			executeFollow(action, ctx);
			break;
		case kActionRandomWalk:
			executeRandomWalk(action, ctx);
			break;
		case kActionDisableTrap:
		case kActionRecoverTrap:
		case kActionFlagTrap:
		case kActionExamineTrap:
			executeTrapAction(action, ctx);
			break;
		case kActionUseObject:
			executeUseObject(action, ctx);
			break;
		case kActionAttackObject:
			executeAttackObject(action, ctx);
			break;
		case kActionPickUpItem:
			executePickUpItem(action, ctx);
			break;
		case kActionDropItem:
			executeDropItem(action, ctx);
			break;
		case kActionWait:
			executeWait(action, ctx);
			break;
		case kActionCastSpell:
			executeCastSpell(action, ctx);
			break;
		case kActionRest:
			executeRest(action, ctx);
			break;
		case kActionCutsceneAttack:
			executeCutsceneAttack(action, ctx);
			break;
		case kActionPlayAnimation:
			executePlayAnimation(action, ctx);
			break;
		case kActionOpenDoor:
			if (action.object) {
				Door *door = ObjectContainer::toDoor(action.object);
				if (door)
					door->open(ctx.creature);
			}
			ctx.creature->popAction();
			break;
		case kActionGiveItem:
		case kActionTakeItem:
			ctx.creature->popAction();
			break;
		case kActionDoCommand:
			executeDoCommand(action, ctx);
			break;
		case kActionPauseConversation:
		case kActionResumeConversation:
			ctx.area->_module->setConversationPaused(action.type == kActionPauseConversation);
			ctx.creature->popAction();
			break;
		default:
			ctx.creature->popAction();
			debugC(Common::kDebugEngineLogic, 1, "ActionExecutor: unhandled action type %u", (uint)action.type);
			break;
	}
}

void ActionExecutor::executeDoCommand(Action &action, const ExecutionContext &ctx) {
	// Pop first: the command commonly queues more actions or clears the queue.
	const Common::UString script = action.script;
	std::shared_ptr<Aurora::NWScript::ScriptState> state = action.scriptState;
	Creature *creature = ctx.creature;
	creature->popAction();

	if (!script.empty() && state)
		ScriptContainer::runScript(script, *state, creature, creature);
}

void ActionExecutor::executeMoveToPoint(Action &action, const ExecutionContext &ctx) {
	if (moveTo(action.location, action.range, ctx, action.choreographyFlags != 0))
		ctx.creature->popAction();
}

void ActionExecutor::executeFollowLeader(Action &action, const ExecutionContext &ctx) {
	Creature *partyLeader = ctx.area->_module->getPartyLeader();
	if (!partyLeader || partyLeader == ctx.creature) {
		ctx.creature->popAction();
		return;
	}

	// Following never finishes on its own; give way to anything queued after
	// it (e.g. an ActionDoCommand from a cutscene script).
	if (ctx.creature->getActionCount() > 1) {
		ctx.creature->popAction();
		return;
	}

	float x, y, _;
	partyLeader->getPosition(x, y, _);

	moveTo(glm::vec2(x, y), action.range, ctx);
}

/** The inventory a party member's finds go to (the party's), or the creature's own. */
static Inventory &getLootInventory(Module &module, Creature &creature) {
	Creature *pc = module.getPC();
	if (pc && module.isObjectPartyMember(&creature))
		return pc->getInventory();

	return creature.getInventory();
}

/** The Thieves' Tools bonus of a security tunneler, which is used up. */
static int useSecurityTunneler(const Common::UString &tag, Inventory &inventory) {
	if (tag.empty())
		return 0;

	if (!inventory.hasItem(tag))
		return 0;

	int bonus = 0;
	try {
		Item item(tag);
		bonus = item.getItemPropertyValue(kItemPropertyThievesTools, 0);
	} catch (...) {
		warning("Cannot use security tunneler \"%s\"", tag.c_str());
		return 0;
	}

	if (bonus > 0)
		inventory.removeItem(tag, 1);

	return bonus;
}

/** Turn the creature towards an object it works on. */
static void faceObject(Creature &creature, Object &target) {
	float x, y, _;
	target.getPosition(x, y, _);
	creature.makeLookAt(x, y);
}

void ActionExecutor::executeOpenLock(Action &action, const ExecutionContext &ctx) {
	Situated *situated = ObjectContainer::toSituated(action.object);
	if (!situated) {
		if (action.object)
			warning("Cannot unlock an object that is not a door or a placeable");

		ctx.creature->popAction();
		return;
	}

	Door *door = ObjectContainer::toDoor(situated);

	if (action.step == 0) {
		float x, y, _;
		situated->getPosition(x, y, _);

		if (!moveTo(glm::vec2(x, y), action.range, ctx))
			return;

		// Work on the lock for a moment, as in the original and KotOR.js.
		faceObject(*ctx.creature, *situated);
		ctx.creature->playAnimation(door ? "unlockdr" : "unlockcntr", false);

		action.step = 1;
		action.startTime = 0.0f;
		return;
	}

	action.startTime += ctx.frameTime;
	if (action.startTime < kUnlockTime)
		return;

	// popAction() destroys the action, so keep what we still need.
	const Common::UString tunneler = action.item;
	Creature *creature = ctx.creature;
	creature->popAction();
	creature->playDefaultAnimation();

	if (!situated->isLocked()) {
		if (door)
			door->open(creature);
		return;
	}

	bool unlocked = false;

	if (situated->isKeyRequired()) {
		// Only the key opens it; no skill check.
		const Common::UString &keyTag = situated->getKeyTag();
		unlocked = getLootInventory(*ctx.area->_module, *creature).hasItem(keyTag) || creature->getInventory().hasItem(keyTag);
		if (!unlocked)
			status("ActionSpeakString [%s]: A specific key is required to open this.", creature->getTag().c_str());

	} else {
		// d20 + Security against the lock DC. Out of combat a failed attempt
		// costs nothing, so the check takes 20 (as in KotOR.js).
		const int security = creature->getSkillRank(kSkillSecurity) + useSecurityTunneler(tunneler, getLootInventory(*ctx.area->_module, *creature));
		const int roll     = TrapRules::checkRoll(creature->isInCombat(), 20, RNG.getNext(1, 21));
		const int dc       = situated->getLockDC();

		unlocked = TrapRules::unlocks(security, roll, dc);
		debugC(Common::kDebugEngineLogic, 1, "Security check by \"%s\": %d + %d vs DC %d: %s",
		       creature->getTag().c_str(), roll, security, dc, unlocked ? "success" : "failure");
	}

	if (!unlocked) {
		situated->runScript(kScriptFailToOpen, situated, creature);
		return;
	}

	const uint32_t targetID = situated->getID();
	situated->setLocked(false);
	situated->runScript(kScriptUnlock, situated, creature);

	// The OnUnlock script may have destroyed the object.
	if (door && (ObjectMan.findObject(targetID) == static_cast<Aurora::NWScript::Object *>(door)))
		door->open(creature);
}

void ActionExecutor::executeLock(Action &action, const ExecutionContext &ctx) {
	Situated *situated = ObjectContainer::toSituated(action.object);
	if (!situated) {
		ctx.creature->popAction();
		return;
	}

	float x, y, _;
	situated->getPosition(x, y, _);

	if (!moveTo(glm::vec2(x, y), MAX(action.range, kLockRange), ctx))
		return;

	Creature *creature = ctx.creature;
	creature->popAction();

	faceObject(*creature, *situated);

	// Close it first, then lock it; no skill check (as in KotOR.js).
	if (Door *door = ObjectContainer::toDoor(situated))
		door->close(creature);
	else if (Placeable *placeable = ObjectContainer::toPlaceable(situated))
		placeable->close(creature);

	situated->setLocked(true);
	situated->runScript(kScriptLock, situated, creature);
}

void ActionExecutor::executeFollow(Action &action, const ExecutionContext &ctx) {
	// ActionForceFollowObject: keep up with the target until the actions are cleared.
	if (!action.object || (action.object == ctx.creature)) {
		ctx.creature->popAction();
		return;
	}

	float x, y, _;
	action.object->getPosition(x, y, _);

	moveTo(glm::vec2(x, y), MAX(action.range, kMinFollowDistance), ctx);
}

void ActionExecutor::executeRandomWalk(Action &action, const ExecutionContext &ctx) {
	// ActionRandomWalk: stroll between random reachable points near where the
	// walk started, pausing in between, until the actions are cleared.
	Creature *creature = ctx.creature;

	float cx, cy, cz;
	creature->getPosition(cx, cy, cz);

	switch (action.step) {
		case 0: // Start
			action.origin = glm::vec3(cx, cy, cz);
			action.step = 1;
			// fall through

		case 1: { // Pick a destination
			std::vector<glm::vec2> route;
			for (int attempt = 0; attempt < kRandomWalkAttempts; ++attempt) {
				const float angle    = RNG.getNext(0.0f, 2.0f * static_cast<float>(M_PI));
				const float distance = RNG.getNext(kRandomWalkMinDistance, kRandomWalkMaxDistance);

				const glm::vec2 goal(action.origin.x + distance * std::cos(angle),
				                     action.origin.y + distance * std::sin(angle));

				// Somewhere on the walkmesh that can be walked to.
				if (ctx.area->evaluateElevation(goal.x, goal.y) == FLT_MIN)
					continue;
				if (!ctx.area->findPath(glm::vec2(cx, cy), goal, route))
					continue;

				action.location = glm::vec3(goal.x, goal.y, 0.0f);
				action.step = 2;
				return;
			}

			// Nowhere to go from here right now; try again after a pause.
			action.step = 3;
			action.startTime = 0.0f;
			action.range = RNG.getNext(kRandomWalkMinPause, kRandomWalkMaxPause);
			return;
		}

		case 2: // Walk there
			if (!moveTo(glm::vec2(action.location.x, action.location.y), kRandomWalkReach, ctx, false, true))
				return;

			creature->playDefaultAnimation();
			action.step = 3;
			action.startTime = 0.0f;
			action.range = RNG.getNext(kRandomWalkMinPause, kRandomWalkMaxPause);
			return;

		default: // Pause
			action.startTime += ctx.frameTime;
			if (action.startTime >= action.range)
				action.step = 1;
			return;
	}
}

void ActionExecutor::executeTrapAction(Action &action, const ExecutionContext &ctx) {
	Creature *creature = ctx.creature;
	Object *target = action.object;

	Trigger *mine = dynamic_cast<Trigger *>(target);
	Situated *situated = ObjectContainer::toSituated(target);

	Trap *trap = mine ? &mine->getTrap() : (situated ? &situated->getTrap() : nullptr);
	if (!trap || !trap->isTrapped()) {
		creature->popAction();
		return;
	}

	// Flagging only needs the trap to have been seen.
	if (action.type == kActionFlagTrap) {
		creature->popAction();

		trap->setFlagged(true);
		if (mine)
			mine->detectTrap(creature);
		else
			trap->setDetectedBy(creature->getID());

		return;
	}

	float x, y, _;
	target->getPosition(x, y, _);

	if (!moveTo(glm::vec2(x, y), MAX(action.range, TrapRules::kDisarmRange), ctx))
		return;

	const ActionType type = action.type;
	creature->popAction();

	faceObject(*creature, *target);

	if (type == kActionExamineTrap)
		return;

	if (!trap->isActive() || !trap->isDisarmable()) {
		status("ActionSpeakString [%s]: This cannot be disarmed.", creature->getTag().c_str());
		return;
	}

	const bool recover = type == kActionRecoverTrap;

	// d20 + Demolitions against the disarm DC. Failure has consequences, so
	// out of combat the check takes 10 rather than 20.
	const int demolitions = creature->getSkillRank(kSkillDemolitions);
	const int roll = TrapRules::checkRoll(creature->isInCombat(), 10, RNG.getNext(1, 21));
	const TrapRules::DisarmResult result = TrapRules::disarm(demolitions, roll, trap->getDisarmDC(), recover);

	debugC(Common::kDebugEngineLogic, 1, "%s check by \"%s\" on \"%s\": %d + %d vs DC %d: %d",
	       recover ? "Recover" : "Disarm", creature->getTag().c_str(), target->getTag().c_str(),
	       roll, demolitions, recover ? TrapRules::getRecoverDC(trap->getDisarmDC()) : trap->getDisarmDC(),
	       (int)result);

	if (result == TrapRules::kDisarmSetOff) {
		if (mine)
			mine->fireTrap(creature);
		else
			situated->triggerTrap(creature);
		return;
	}

	if (result == TrapRules::kDisarmFailed) {
		status("ActionSpeakString [%s]: Failed to disarm the mine.", creature->getTag().c_str());
		return;
	}

	const Common::UString item = trap->getItemResRef();
	if (recover && !item.empty() && (item != "****"))
		getLootInventory(*ctx.area->_module, *creature).addItem(item);

	if (mine)
		mine->disableTrap();
	else
		situated->setTrapDisabled();

	target->runScript(kScriptDisarm, target, creature);

	// A mine is gone for good once disarmed.
	if (mine)
		ctx.area->destroyObjectDeferred(mine);
}

void ActionExecutor::executeUseObject(Action &action, const ExecutionContext &ctx) {
	if (!action.object) {
		ctx.creature->popAction();
		return;
	}

	float x, y, _;
	action.object->getPosition(x, y, _);

	if (!moveTo(glm::vec2(x, y), action.range, ctx))
		return;

	// popAction() destroys the action, so keep what we still need.
	Object *target = action.object;
	ctx.creature->popAction();

	Module *module = ctx.area->_module;

	if (ctx.creature != module->getPartyLeader()) {
		warning("ActionExecutor::executeUseObject(): Creature is not the party leader");
		return;
	}

	// The dead don't talk, but what they carried is left for the taking.
	Creature *corpse = ObjectContainer::toCreature(target);
	if (corpse && corpse->isDead()) {
		if (!corpse->isPC() && !module->isObjectPartyMember(corpse) &&
		    !corpse->getInventory().getItems().empty())
			module->delayContainer(corpse);

		return;
	}

	const uint32_t targetID = target->getID();
	target->click(module->getPartyLeader());

	// The click scripts (OnUsed, OnDialog, ...) may have destroyed the target.
	if (ObjectMan.findObject(targetID) != target)
		return;

	Creature *creatureObject = ObjectContainer::toCreature(target);
	if (creatureObject) {
		const Common::UString conversation = creatureObject->getConversation();
		if (!conversation.empty())
			module->delayConversation(conversation, creatureObject);

		return;
	}

	Situated *situated = ObjectContainer::toSituated(target);
	if (situated) {
		const Common::UString conversation = situated->getConversation();
		if (!conversation.empty()) {
			module->delayConversation(conversation, situated);
			return;
		}

		Placeable *placeable = ObjectContainer::toPlaceable(target);
		if (placeable && placeable->hasInventory()) {
			module->delayContainer(placeable);
			return;
		}
	}
}

void ActionExecutor::executeAttackObject(Action &action, const ExecutionContext &ctx) {
	if (!action.object || action.object->isDead()) {
		ctx.creature->popAction();
		return;
	}

	float x, y, _;
	action.object->getPosition(x, y, _);

	const float range = ctx.creature->getMaxAttackRange();
	if (!moveTo(glm::vec2(x, y), range, ctx))
		return;

	// popAction() destroys the action, so keep what we still need.
	Object *target = action.object;
	const int choreographyFlags = action.choreographyFlags;
	const int damage = action.damage;
	ctx.creature->popAction();

	if (choreographyFlags > 0) {
		ctx.creature->performCutsceneAttack(target, choreographyFlags, damage);
		return;
	}

	// moveTo() also "arrives" when it gives up on a blocked path. Fighting an
	// unreachable target would never land a blow, so drop it instead; the AI
	// picks a (possibly different) target again.
	if (!isLocationReached(glm::vec2(x, y), MAX(range, kMinMoveRange) + 0.01f, ctx)) {
		debugC(Common::kDebugEngineLogic, 1, "Creature \"%s\" cannot reach \"%s\"; dropping the attack",
		       ctx.creature->getTag().c_str(), target->getTag().c_str());
		if (ctx.creature->getAttackTarget() == target)
			ctx.creature->cancelCombat();
		return;
	}

	ctx.creature->startCombat(target, ctx.area->_module->getNextCombatRound());
}

void ActionExecutor::executePickUpItem(Action &action, const ExecutionContext &ctx) {
	if (!action.object)
		return;

	float x, y, _;
	action.object->getPosition(x, y, _);

	if (!moveTo(glm::vec2(x, y), action.range, ctx))
		return;

	Common::UString itemResRef = action.object->getTemplateResRef();
	if (itemResRef.empty())
		itemResRef = action.object->getTag();
	
	Object *itemObject = action.object;

	ctx.creature->popAction();
	if (!itemResRef.empty()) {
		Creature *pc = ctx.area->_module->getPC();
		if (pc)
			pc->getInventory().addItem(itemResRef);
		else
			ctx.creature->getInventory().addItem(itemResRef);
	}
	ctx.area->removeObject(itemObject);
}

void ActionExecutor::executeDropItem(Action &action, const ExecutionContext &ctx) {
	if (!action.object || !ctx.area || !ctx.creature) {
		if (ctx.creature)
			ctx.creature->popAction();
		return;
	}

	Item *itemObj = dynamic_cast<Item *>(action.object);
	if (!itemObj) {
		ctx.creature->popAction();
		return;
	}

	Common::UString tag = itemObj->getTemplateResRef();
	if (tag.empty())
		tag = itemObj->getTag();

	Module *module = ctx.area->_module;
	Creature *owner = ctx.creature;

	for (int i = static_cast<int>(kInventorySlotHead); i < static_cast<int>(kInventorySlotMAX); ++i) {
		const InventorySlot slot = static_cast<InventorySlot>(i);
		if (!ctx.creature->getCreatureInfo().isInventorySlotEquipped(slot))
			continue;

		if (ctx.creature->getCreatureInfo().getEquippedItem(slot).equalsIgnoreCase(tag)) {
			ctx.creature->equipItem("", slot);
			break;
		}
	}

	float x = action.location.x;
	float y = action.location.y;
	float z = action.location.z;
	const float elevation = ctx.area->evaluateElevation(x, y);
	if (elevation != FLT_MIN)
		z = elevation;

	dropInventoryItem(*owner, tag, 1, module, x, y, z, true);
	ctx.creature->popAction();
}

bool ActionExecutor::isLocationReached(const glm::vec2 &location, float range, const ExecutionContext &ctx) {
	float x, y, _;
	ctx.creature->getPosition(x, y, _);
	return glm::distance(glm::vec2(x, y), location) <= range;
}

bool ActionExecutor::moveTo(const glm::vec2 &location, float range, const ExecutionContext &ctx,
                            bool forceRun, bool forceWalk) {
	range = MAX(range, kMinMoveRange);

	MovePath &path = ctx.creature->getMovePath();

	if (isLocationReached(location, range, ctx)) {
		ctx.creature->resetBlockedMoveTime();
		path.clear();
		return true;
	}

	float oX, oY, oZ;
	ctx.creature->getPosition(oX, oY, oZ);
	glm::vec2 origin(oX, oY);

	// Walk a walkmesh route around walls rather than straight into them. A
	// goal that moved a lot (a chased creature) or an old route is redone.
	path.age += ctx.frameTime;
	if (!path.valid || (glm::length(path.goal - location) > kRepathDistance) || (path.age > kRepathTime)) {
		path.clear();
		path.valid = true;
		if (!ctx.area->findPath(origin, location, path.waypoints))
			path.waypoints.assign(1, location); // No known route: straight on.
	}
	path.goal = location;
	path.waypoints.back() = location;

	const glm::vec2 target = nextWaypoint(path.waypoints, origin, kWaypointReach);

	glm::vec2 diff = target - origin;
	float dist = glm::length(diff);
	if (dist < 0.0001f)
		return false;

	ctx.creature->makeLookAt(target.x, target.y);

	const glm::vec2 dir = diff / dist;

	bool run = forceRun || (!forceWalk && (glm::length(location - origin) > kWalkDistance));
	float moveRate = run ? ctx.creature->getRunRate() : ctx.creature->getWalkRate();

	// Don't overshoot the waypoint.
	const float step = MIN(moveRate * ctx.frameTime, dist);

	float x = origin.x + step * dir.x;
	float y = origin.y + step * dir.y;
	float z = ctx.area->evaluateElevation(x, y);

	bool haveMovement = (z != FLT_MIN) &&
	                     ctx.area->walkable(glm::vec3(oX, oY, oZ + 0.1f),
	                                        glm::vec3(x, y, z + 0.1f));

	if (haveMovement) {
		ctx.creature->resetBlockedMoveTime();
		ctx.creature->playAnimation(run ? "run" : "walk", false, -1.0f);
		ctx.creature->setPosition(x, y, z);

		if (ctx.creature == ctx.area->_module->getPartyLeader())
			ctx.area->_module->movedPartyLeader();
		else
			ctx.area->notifyObjectMoved(*ctx.creature);

		if (glm::length(location - glm::vec2(x, y)) <= range) {
			ctx.creature->playDefaultAnimation();
			path.clear();
			return true;
		}

	} else {
		ctx.creature->playDefaultAnimation();

		// Something (a door, a placeable) is in the way: look for another route.
		if (path.age > kRepathWhenBlocked)
			path.valid = false;

		// Rather than stall the action queue (and any cutscene waiting on it)
		// forever, give up after a while and let the action proceed from here.
		if (ctx.creature->addBlockedMoveTime(ctx.frameTime) >= kMaxBlockedMoveTime) {
			debugC(Common::kDebugEngineLogic, 1, "Creature \"%s\" is stuck; abandoning movement",
			       ctx.creature->getTag().c_str());
			ctx.creature->resetBlockedMoveTime();
			path.clear();
			return true;
		}
	}

	return false;
}

void ActionExecutor::executeWait(Action &action, const ExecutionContext &ctx) {
	action.startTime += ctx.frameTime;

	if (action.startTime >= action.range)
		ctx.creature->popAction();
}

void ActionExecutor::executeCastSpell(Action &queuedAction, const ExecutionContext &ctx) {
	// Casting always consumes the action. Pop it before anything runs: impact
	// scripts and effects (stun, ClearAllActions, ...) may clear the queue.
	const Action action = queuedAction;
	ctx.creature->popAction();

	castSpell(action, ctx);
}

/** The live item object of an equipped or carried item template. */
static Object *findCastItem(Creature &caster, const Common::UString &item) {
	for (int slot = 0; slot < kInventorySlotMAX; ++slot) {
		const InventorySlot inventorySlot = static_cast<InventorySlot>(slot);
		if (caster.getCreatureInfo().isInventorySlotEquipped(inventorySlot) &&
		    (caster.getCreatureInfo().getEquippedItem(inventorySlot) == item))
			return caster.getEquipedItem(inventorySlot);
	}

	Item *found = caster.findInventoryItemByTag(item);
	return found ? found : caster.addScriptItem(item);
}

bool ActionExecutor::castSpell(const Action &action, const ExecutionContext &ctx) {
	Creature *caster = ctx.creature;
	if (!caster || !ctx.area)
		return false;

	int cost = 0;

	if (!_spellsLoaded)
		loadSpells();

	// Powers of the side of the Force opposite to the caster's cost more.
	// bCheat and spells cast by items are free.
	const SpellInfo *spell = getSpellInfo(action.actionID);
	if (spell && !action.cheat)
		cost = getForcePowerCost(spell->cost, spell->goodEvil, caster->getAlignment());

	if (caster->getForcePoints() < cost) {
		debugC(Common::kDebugEngineLogic, 1, "Not enough Force Points to cast power %d", action.actionID);
		return false;
	}

	int saveDC = 10 + caster->getHitDice() + caster->getAbilityModifier(kAbilityWisdom);
	bool harmful = spell ? spell->hostile : false;

	// Hostile targeted powers need line of sight past closed doors (before spending FP).
	if (harmful && action.object && !ctx.area->hasLineOfSight(caster, action.object)) {
		debugC(Common::kDebugEngineLogic, 1, "Force power %d blocked by cover", action.actionID);
		return false;
	}

	caster->setForcePoints(caster->getForcePoints() - cost);
	caster->setLastForcePowerUsed(action.actionID);
	caster->playSpellCastVisuals(action.actionID);

	// The item casting the spell (GetSpellCastItem), used up when it's a consumable.
	Object *castItem = nullptr;
	if (!action.item.empty()) {
		castItem = findCastItem(*caster, action.item);
		if (action.consumeItem)
			caster->getCreatureInfo().removeInventoryItem(action.item, 1);
	}

	// A hostile power makes the caster the target's last hostile actor, so a
	// kill by the power is credited to the caster (XP, GetLastKiller).
	if (harmful) {
		Creature *targetCreature = ObjectContainer::toCreature(action.object);
		if (targetCreature && targetCreature != caster)
			targetCreature->setLastHostileActor(caster);
	}
	ctx.area->_module->setSpellScriptContext(action.actionID, caster, action.object, saveDC, harmful, castItem);
	if (!action.object)
		ctx.area->_module->setSpellScriptTargetLocation(action.location);
	ctx.area->_module->setGlobalNumber("__force_unsuccessful", 0);

	// Prefer data-driven impact scripts from spells.2da when available.
	// Hardcoded cases below are fallbacks when retail scripts are missing.
	if (spell && !spell->impactScript.empty()) {
		caster->playAnimation(harmful ? "castout" : "castself", false);
		Object *owner = caster;
		Object *triggerer = action.object ? action.object : caster;
		ScriptContainer::runScript(spell->impactScript, owner, triggerer);
		// The impact script has resolved; don't leak its context into later scripts.
		ctx.area->_module->clearSpellScriptContext();
		return true;
	}

	bool handled = true;

	// Apply power effects
	switch (action.actionID) {
		case 1: // Force Heal (Party heal)
			caster->playAnimation("castheal", false);
			for (int i = 0; i < ctx.area->_module->getPartyMemberCount(); ++i) {
				Creature *member = ctx.area->_module->getPartyMemberByIndex(i);
				if (member && !member->isDead())
					member->applyEffect(kEffectHeal, 0.0f, 15, action.actionID);
			}
			break;

		case 5: // Force Stun (Single target)
			if (action.object) {
				Creature *target = ObjectContainer::toCreature(action.object);
				if (target) {
					caster->playAnimation("castout", false);
					int level = caster->getHitDice();
					int dc = 10 + level + caster->getAbilityModifier(kAbilityWisdom);
					if (!target->rollSavingThrow(kSavingThrowWill, dc)) {
						target->applyEffect(kEffectStun, 9.0f, 0, action.actionID);
						debugC(Common::kDebugEngineLogic, 1, "Force Stun SUCCESS on %s", target->getTag().c_str());
					} else {
						ctx.area->_module->setGlobalNumber("__force_unsuccessful", 1);
					}
				}
			}
			break;

		case 2: // Force Push (Single target)
			if (action.object) {
				Creature *target = ObjectContainer::toCreature(action.object);
				if (target) {
					caster->playAnimation("castout", false);
					int level = caster->getHitDice();
					int dc = 10 + level + caster->getAbilityModifier(kAbilityWisdom);
					if (!target->rollSavingThrow(kSavingThrowFortitude, dc)) {
						target->applyEffect(kEffectKnockdown, 3.0f, 0, action.actionID);
						target->applyEffect(kEffectDamage, 0.0f, level, action.actionID);
						debugC(Common::kDebugEngineLogic, 1, "Force Push SUCCESS on %s", target->getTag().c_str());
					} else {
						// Half damage on save — still counts as resisted for script queries.
						target->applyEffect(kEffectDamage, 0.0f, level / 2, action.actionID);
						ctx.area->_module->setGlobalNumber("__force_unsuccessful", 1);
					}
				}
			}
			break;

		case 3: // Burst of Speed (Self)
			caster->playAnimation("castself", false);
			caster->applyEffect(kEffectSpeed, 0.0f, 50, action.actionID);
			break;

		case 14: // Shock (Single target lightning)
		case 15: // Force Lightning (Cone/Radius)
			if (action.object) {
				Creature *target = ObjectContainer::toCreature(action.object);
				if (target) {
					caster->playAnimation("castout", false);
					int level = caster->getHitDice();
					int damage = level * 3; // Simplified 1d6 per level approx
					
					int dc = 10 + level + caster->getAbilityModifier(kAbilityWisdom);
					bool saved = target->rollSavingThrow(kSavingThrowWill, dc);
					if (saved) {
						damage /= 2;
						ctx.area->_module->setGlobalNumber("__force_unsuccessful", 1);
					}

					target->applyEffect(kEffectDamage, 0.0f, damage, action.actionID);
					// Visual effect hook would go here
					debugC(Common::kDebugEngineLogic, 1, "Force Lightning hit %s for %d", target->getTag().c_str(), damage);
				}
			}
			break;

		case 25: // Plague (Unstoppable DoT)
			if (action.object) {
				Creature *target = ObjectContainer::toCreature(action.object);
				if (target) {
					caster->playAnimation("castout", false);
					// Plague is special: DC 100 (Unstoppable)
					target->applyEffect(kEffectPoison, 12.0f, 5, action.actionID); // 5 dmg per sec for 12s
				}
			}
			break;

		case 33: // Wound
		case 34: // Choke
			if (action.object) {
				Creature *target = ObjectContainer::toCreature(action.object);
				if (target) {
					caster->playAnimation("castout", false);
					int level = caster->getHitDice();
					int dc = 10 + level + caster->getAbilityModifier(kAbilityWisdom);
					
					if (!target->rollSavingThrow(kSavingThrowFortitude, dc)) {
						target->applyEffect(kEffectStun, 6.0f, 0, action.actionID);
						target->applyEffect(kEffectDamage, 0.0f, level * 2, action.actionID);
					} else {
						ctx.area->_module->setGlobalNumber("__force_unsuccessful", 1);
					}
				}
			}
			break;

		case 4: // Affect Mind
			if (action.object) {
				caster->playAnimation("castself", false);
				Creature *target = ObjectContainer::toCreature(action.object);
				if (target)
					target->applyEffect(kEffectStun, 6.0f, 0, action.actionID);
			}
			break;

		default:
			warning("ActionExecutor::castSpell(): Unknown power ID %d", action.actionID);
			handled = false;
			break;
	}

	ctx.area->_module->clearSpellScriptContext();
	return handled;
}

void ActionExecutor::executeRest(Action &UNUSED(action), const ExecutionContext &ctx) {
	Creature *creature = ctx.creature;
	creature->popAction();

	Module *module = ctx.area->_module;

	// As in the original, resting restores the whole party, and only when
	// nobody in the party is fighting.
	std::vector<Creature *> resting;
	if (module->isObjectPartyMember(creature)) {
		for (int i = 0; i < module->getPartyMemberCount(); ++i) {
			Creature *member = module->getPartyMemberByIndex(i);
			if (member)
				resting.push_back(member);
		}
	} else
		resting.push_back(creature);

	for (Creature *c : resting)
		if (c->isInCombat())
			return;

	for (Creature *c : resting)
		c->restoreFully();
}

void ActionExecutor::executePlayAnimation(Action &action, const ExecutionContext &ctx) {
	const Common::UString animName = getAnimationNameById(action.actionID);
	if (animName.empty()) {
		ctx.creature->popAction();
		return;
	}

	if (action.startTime < 0.0f) {
		float speed = action.range > 0.0f ? action.range : 1.0f;
		float length = -action.startTime;
		ctx.creature->playAnimation(animName, true, length, speed);
		action.startTime = length;
	}

	action.startTime -= ctx.frameTime;
	if (action.startTime <= 0.0f) {
		ctx.creature->playDefaultAnimation();
		ctx.creature->popAction();
	}
}

void ActionExecutor::executeCutsceneAttack(Action &action, const ExecutionContext &ctx) {
	if (!action.object) {
		ctx.creature->popAction();
		return;
	}

	float x, y, _;
	action.object->getPosition(x, y, _);

	if (!moveTo(glm::vec2(x, y), ctx.creature->getMaxAttackRange(), ctx))
		return;

	// popAction() destroys the action, so keep what we still need.
	Object *target = action.object;
	const int choreographyFlags = action.choreographyFlags;
	const int damage = action.damage;
	ctx.creature->popAction();
	ctx.creature->performCutsceneAttack(target, choreographyFlags, damage);
}

void ActionExecutor::loadSpells() {
	if (_spellsLoaded)
		return;

	try {
		const Aurora::TwoDAFile &twoda = TwoDAReg.get2DA("spells");
		for (size_t i = 0; i < twoda.getRowCount(); ++i) {
			const Aurora::TwoDARow &row = twoda.getRow(i);
			
			SpellInfo info;
			info.id = i;
			info.label = row.getString("label");
			if (!row.empty("name"))
				info.nameStrRef = row.getInt("name");
			info.cost = row.getInt("forcepoints");
			info.impactScript = row.getString("impactscript");
			info.hostile = row.getInt("hostile") != 0;
			info.goodEvil = row.getString("goodevil");
			
			_spells[info.id] = info;
		}
	} catch (...) {
		warning("ActionExecutor::loadSpells(): Could not load spells.2da");
	}

	_spellsLoaded = true;
}

const ActionExecutor::SpellInfo *ActionExecutor::getSpellInfo(uint32_t id) {
	if (!_spellsLoaded)
		loadSpells();

	auto it = _spells.find(id);
	if (it != _spells.end())
		return &it->second;

	return nullptr;
}

} // End of namespace KotORBase

} // End of namespace Engines
