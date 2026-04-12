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

#include "external/glm/vec3.hpp"
#include "external/glm/gtc/type_ptr.hpp"

#include "src/common/maths.h"

#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/actionexecutor.h"
#include "src/engines/kotorbase/area.h"
#include "src/engines/kotorbase/module.h"
#include "src/engines/kotorbase/door.h"
#include "src/engines/kotorbase/placeable.h"
#include "src/engines/kotorbase/inventory.h"

static const float kWalkDistance = 2.0f;

namespace Engines {

namespace KotORBase {

void ActionExecutor::execute(Action &action, const ExecutionContext &ctx) {
	if (!action.initialized) {
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
		case kActionUseObject:
			executeUseObject(action, ctx);
			break;
		case kActionAttackObject:
			executeAttackObject(action, ctx);
			break;
		case kActionPickUpItem:
			executePickUpItem(action, ctx);
			break;
		case kActionWait:
			executeWait(action, ctx);
			break;
		case kActionCastSpell:
			executeCastSpell(action, ctx);
			break;
		case kActionCutsceneAttack:
			executeCutsceneAttack(action, ctx);
			break;
		default:
			warning("TODO: Handle action %u", (uint)action.type);
			break;
	}
}

void ActionExecutor::executeMoveToPoint(Action &action, const ExecutionContext &ctx) {
	if (moveTo(action.location, action.range, ctx))
		ctx.creature->popAction();
}

void ActionExecutor::executeFollowLeader(Action &action, const ExecutionContext &ctx) {
	Creature *partyLeader = ctx.area->_module->getPartyLeader();

	float x, y, _;
	partyLeader->getPosition(x, y, _);

	moveTo(glm::vec2(x, y), action.range, ctx);
}

void ActionExecutor::executeOpenLock(Action &action, const ExecutionContext &ctx) {
	float x, y, _;
	action.object->getPosition(x, y, _);

	if (!moveTo(glm::vec2(x, y), action.range, ctx))
		return;

	ctx.creature->popAction();

	if (ctx.creature != ctx.area->_module->getPartyLeader()) {
		warning("ActionExecutor::executeOpenLock(): Creature is not the party leader");
		return;
	}

	Door *door = ObjectContainer::toDoor(action.object);
	if (door) {
		ctx.creature->playAnimation("unlockdr", false);
		door->unlock(ctx.creature);
		return;
	}

	Placeable *placeable = ObjectContainer::toPlaceable(action.object);
	if (placeable) {
		ctx.creature->playAnimation("unlockcntr", false);
		placeable->unlock(ctx.creature);
		return;
	}

	warning("Cannot unlock an object that is not a door or a placeable");
}

void ActionExecutor::executeUseObject(Action &action, const ExecutionContext &ctx) {
	float x, y, _;
	action.object->getPosition(x, y, _);

	if (!moveTo(glm::vec2(x, y), action.range, ctx))
		return;

	ctx.creature->popAction();

	Module *module = ctx.area->_module;

	if (ctx.creature != module->getPartyLeader()) {
		warning("ActionExecutor::executeUseObject(): Creature is not the party leader");
		return;
	}

	action.object->click(module->getPartyLeader());

	Creature *creatureObject = ObjectContainer::toCreature(action.object);
	if (creatureObject) {
		const Common::UString conversation = creatureObject->getConversation();
		if (!conversation.empty())
			module->delayConversation(conversation, creatureObject);

		return;
	}

	Situated *situated = ObjectContainer::toSituated(action.object);
	if (situated) {
		const Common::UString conversation = situated->getConversation();
		if (!conversation.empty()) {
			module->delayConversation(conversation, situated);
			return;
		}

		Placeable *placeable = ObjectContainer::toPlaceable(action.object);
		if (placeable && placeable->hasInventory()) {
			module->delayContainer(placeable);
			return;
		}
	}
}

void ActionExecutor::executeAttackObject(Action &action, const ExecutionContext &ctx) {
	float x, y, _;
	action.object->getPosition(x, y, _);

	if (!moveTo(glm::vec2(x, y), ctx.creature->getMaxAttackRange(), ctx))
		return;

	ctx.creature->popAction();
	
	if (action.choreographyFlags > 0)
		ctx.creature->performCutsceneAttack(action.object, action.choreographyFlags);
	else
		ctx.creature->startCombat(action.object, ctx.area->_module->getNextCombatRound());
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
		ctx.creature->getInventory().addItem(itemResRef);
	}
	ctx.area->removeObject(itemObject);
}

bool ActionExecutor::isLocationReached(const glm::vec2 &location, float range, const ExecutionContext &ctx) {
	float x, y, _;
	ctx.creature->getPosition(x, y, _);
	return glm::distance(glm::vec2(x, y), location) <= range;
}

bool ActionExecutor::moveTo(const glm::vec2 &location, float range, const ExecutionContext &ctx) {
	if (isLocationReached(location, range, ctx))
		return true;

	ctx.creature->makeLookAt(location.x, location.y);

	float oX, oY, oZ;
	ctx.creature->getPosition(oX, oY, oZ);
	glm::vec2 origin(oX, oY);

	glm::vec2 diff = location - origin;
	glm::vec2 dir = glm::normalize(diff);

	float dist = glm::length(diff);
	bool run = dist > kWalkDistance;
	float moveRate = run ? ctx.creature->getRunRate() : ctx.creature->getWalkRate();

	float x = origin.x + moveRate * dir.x * ctx.frameTime;
	float y = origin.y + moveRate * dir.y * ctx.frameTime;
	float z = ctx.area->evaluateElevation(x, y);

	bool haveMovement = (z != FLT_MIN) &&
	                     ctx.area->walkable(glm::vec3(oX, oY, oZ + 0.1f),
	                                        glm::vec3(x, y, z + 0.1f));

	if (haveMovement) {
		ctx.creature->playAnimation(run ? "run" : "walk", false, -1.0f);
		ctx.creature->setPosition(x, y, z);

		if (ctx.creature == ctx.area->_module->getPartyLeader())
			ctx.area->_module->movedPartyLeader();
		else
			ctx.area->notifyObjectMoved(*ctx.creature);

		diff = location - glm::vec2(x, y);
		dist = glm::length(diff);

		if (dist <= range) {
			ctx.creature->playDefaultAnimation();
			return true;
		}

	} else {
		ctx.creature->playDefaultAnimation();
	}

	return false;
}

void ActionExecutor::executeWait(Action &action, const ExecutionContext &ctx) {
	action.startTime += ctx.frameTime;

	if (action.startTime >= action.range)
		ctx.creature->popAction();
}

void ActionExecutor::executeCastSpell(Action &action, const ExecutionContext &ctx) {
	Creature *caster = ctx.creature;
	int cost = 0;

	// Determine cost (placeholder logic, normally from spells.2da)
	switch (action.actionID) {
		case 1: cost = 15; break; // Heal
		case 2: cost = 10; break; // Push
		case 3: cost = 15; break; // Speed
		default: break;
	}

	if (caster->getForcePoints() < cost) {
		debugC(Common::kDebugEngineLogic, 1, "Not enough Force Points to cast power %d", action.actionID);
		caster->popAction();
		return;
	}

	caster->setForcePoints(caster->getForcePoints() - cost);

	// Apply power effects
	switch (action.actionID) {
		case 1: // Force Heal (Party heal)
			caster->playAnimation("castheal", false);
			for (int i = 0; i < ctx.area->_module->getPartyMemberCount(); ++i) {
				Creature *member = ctx.area->_module->getPartyMemberByIndex(i);
				if (member && !member->isDead())
					member->applyEffect(Effect(kEffectHeal, 15));
			}
			break;

		case 2: // Force Push (Single target)
			if (action.object) {
				caster->playAnimation("castout", false);
				Creature *target = ObjectContainer::toCreature(action.object);
				if (target) {
					target->applyEffect(Effect(kEffectDamage, 10));
					target->applyEffect(Effect(kEffectKnockdown));
				}
			}
			break;

		case 3: // Burst of Speed (Self)
			caster->playAnimation("castself", false);
			caster->applyEffect(Effect(kEffectMovementSpeedIncrease, 50));
			break;

		case 4: // Affect Mind
			if (action.object) {
				caster->playAnimation("castself", false);
				Creature *target = ObjectContainer::toCreature(action.object);
				if (target)
					target->applyEffect(Effect(kEffectStun, 6.0f));
			}
			break;

		default:
			warning("ActionExecutor::executeCastSpell(): Unknown power ID %d", action.actionID);
			break;
	}

	caster->setForcePoints(caster->getForcePoints() - cost);
	caster->popAction();
}

void ActionExecutor::executeCutsceneAttack(Action &action, const ExecutionContext &ctx) {
	float x, y, _;
	action.object->getPosition(x, y, _);

	if (!moveTo(glm::vec2(x, y), ctx.creature->getMaxAttackRange(), ctx))
		return;

	ctx.creature->popAction();
	ctx.creature->performCutsceneAttack(action.object, action.choreographyFlags);
}

} // End of namespace KotORBase

} // End of namespace Engines
