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
 *  Star Wars: Knights of the Old Republic engine functions messing with situated objects.
 */

#include "src/aurora/nwscript/functioncontext.h"

#include "src/engines/kotorbase/action.h"
#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/door.h"
#include "src/engines/kotorbase/object.h"
#include "src/engines/kotorbase/situated.h"
#include "src/engines/kotorbase/types.h"
#include "src/engines/kotorbase/objectcontainer.h"

#include "src/engines/kotorbase/script/functions.h"

namespace Engines {

namespace KotORBase {

void Functions::getLocked(Aurora::NWScript::FunctionContext &ctx) {
	Situated *situated = ObjectContainer::toSituated(getParamObject(ctx, 0));

	ctx.getReturn() = situated ? situated->isLocked() : 0;
}

void Functions::setLocked(Aurora::NWScript::FunctionContext &ctx) {
	Situated *situated = ObjectContainer::toSituated(getParamObject(ctx, 0));
	if (situated)
		situated->setLocked(ctx.getParams()[1].getInt() != 0);
}

void Functions::getIsOpen(Aurora::NWScript::FunctionContext &ctx) {
	Situated *situated = ObjectContainer::toSituated(getParamObject(ctx, 0));

	ctx.getReturn() = situated ? situated->isOpen() : 0;
}

void Functions::getLastOpenedBy(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = (Aurora::NWScript::Object *) 0;

	Situated *situated = ObjectContainer::toSituated(ctx.getCaller());
	if (!situated)
		return;

	ctx.getReturn() = (Aurora::NWScript::Object *) situated->getLastOpenedBy();
}

void Functions::getLastClosedBy(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = (Aurora::NWScript::Object *) 0;

	Situated *situated = ObjectContainer::toSituated(ctx.getCaller());
	if (!situated)
		return;

	ctx.getReturn() = (Aurora::NWScript::Object *) situated->getLastClosedBy();
}

void Functions::getLastUsedBy(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = (Aurora::NWScript::Object *) 0;

	Situated *situated = ObjectContainer::toSituated(ctx.getCaller());
	if (!situated)
		return;

	ctx.getReturn() = (Aurora::NWScript::Object *) situated->getLastUsedBy();
}


void Functions::getTrapDisarmable(Aurora::NWScript::FunctionContext &ctx) {
	Situated *situated = ObjectContainer::toSituated(getParamObject(ctx, 0));
	ctx.getReturn() = situated && situated->getTrapDisarmable() ? 1 : 0;
}

void Functions::getTrapDetectable(Aurora::NWScript::FunctionContext &ctx) {
	Situated *situated = ObjectContainer::toSituated(getParamObject(ctx, 0));
	ctx.getReturn() = situated && situated->getTrapDetectable() ? 1 : 0;
}

void Functions::getTrapDetectedBy(Aurora::NWScript::FunctionContext &ctx) {
	Situated *situated = ObjectContainer::toSituated(getParamObject(ctx, 0));
	ctx.getReturn() = situated ? situated->getTrapDetectedBy() : static_cast<Aurora::NWScript::Object *>(nullptr);
}

void Functions::getTrapFlagged(Aurora::NWScript::FunctionContext &ctx) {
	Situated *situated = ObjectContainer::toSituated(getParamObject(ctx, 0));
	ctx.getReturn() = situated && situated->getTrapFlagged() ? 1 : 0;
}

void Functions::getTrapBaseType(Aurora::NWScript::FunctionContext &ctx) {
	Situated *situated = ObjectContainer::toSituated(getParamObject(ctx, 0));
	ctx.getReturn() = situated ? situated->getTrapBaseType() : 0;
}

void Functions::getTrapOneShot(Aurora::NWScript::FunctionContext &ctx) {
	Situated *situated = ObjectContainer::toSituated(getParamObject(ctx, 0));
	ctx.getReturn() = situated && situated->getTrapOneShot() ? 1 : 0;
}

void Functions::getTrapCreator(Aurora::NWScript::FunctionContext &ctx) {
	Situated *situated = ObjectContainer::toSituated(getParamObject(ctx, 0));
	ctx.getReturn() = situated ? situated->getTrapCreator() : static_cast<Aurora::NWScript::Object *>(nullptr);
}

void Functions::getTrapKeyTag(Aurora::NWScript::FunctionContext &ctx) {
	Situated *situated = ObjectContainer::toSituated(getParamObject(ctx, 0));
	ctx.getReturn() = situated ? situated->getTrapKeyTag() : Common::UString("");
}

void Functions::getTrapDisarmDC(Aurora::NWScript::FunctionContext &ctx) {
	Situated *situated = ObjectContainer::toSituated(getParamObject(ctx, 0));
	ctx.getReturn() = situated ? situated->getTrapDisarmDC() : 0;
}

void Functions::getTrapDetectDC(Aurora::NWScript::FunctionContext &ctx) {
	Situated *situated = ObjectContainer::toSituated(getParamObject(ctx, 0));
	ctx.getReturn() = situated ? situated->getTrapDetectDC() : 0;
}

void Functions::setTrapDetectedBy(Aurora::NWScript::FunctionContext &ctx) {
	Situated *situated = ObjectContainer::toSituated(getParamObject(ctx, 0));
	Object *detector = ObjectContainer::toObject(getParamObject(ctx, 1));
	if (situated)
		situated->setTrapDetectedBy(detector);
}

void Functions::getIsTrapped(Aurora::NWScript::FunctionContext &ctx) {
	Situated *situated = ObjectContainer::toSituated(getParamObject(ctx, 0));
	ctx.getReturn() = situated && situated->getIsTrapped() ? 1 : 0;
}

void Functions::setTrapDisabled(Aurora::NWScript::FunctionContext &ctx) {
	Situated *situated = ObjectContainer::toSituated(getParamObject(ctx, 0));
	if (situated)
		situated->setTrapDisabled();
}
void Functions::getLockKeyRequired(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = 0; }
void Functions::getLockLockable(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = 1; }
void Functions::getLockLockDC(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = 20; }

static bool creatureHasDoorKey(Creature *creature, const Situated *situated) {
	return creature && situated && situated->isKeyRequired() &&
	       creature->getInventory().hasItem(situated->getKeyTag());
}

static bool canOpenDoor(Creature *creature, Door *door) {
	if (!door || door->isOpen() || door->getPlotFlag())
		return false;
	if (!door->isLocked())
		return true;
	return creatureHasDoorKey(creature, door);
}

static bool canUnlockDoor(Creature *creature, Situated *situated) {
	if (!creature || !situated || !situated->isLocked())
		return false;
	if (situated->isKeyRequired())
		return false;
	return (20 + creature->getSkillRank(kSkillSecurity)) >= situated->getLockDC();
}

static bool canBashDoor(Door *door) {
	return door && !door->isOpen() && !door->getPlotFlag();
}

static bool canKnockDoor(Creature *creature, Situated *situated) {
	if (!creature || !situated || situated->getPlotFlag() || situated->isKeyRequired())
		return false;
	return creature->hasForcePower(93);
}

void Functions::getIsDoorActionPossible(Aurora::NWScript::FunctionContext &ctx) {
	Door *door = ObjectContainer::toDoor(getParamObject(ctx, 0));
	int action = ctx.getParams()[1].getInt();
	Creature *creature = ObjectContainer::toCreature(ctx.getCaller());

	if (!door) {
		ctx.getReturn() = 0;
		return;
	}

	switch (action) {
		case kDoorActionOpen:
			ctx.getReturn() = canOpenDoor(creature, door) ? 1 : 0;
			break;
		case kDoorActionUnlock:
			ctx.getReturn() = canUnlockDoor(creature, door) ? 1 : 0;
			break;
		case kDoorActionBash:
			ctx.getReturn() = canBashDoor(door) ? 1 : 0;
			break;
		case kDoorActionIgnore:
			ctx.getReturn() = 1;
			break;
		case kDoorActionKnock:
			ctx.getReturn() = canKnockDoor(creature, door) ? 1 : 0;
			break;
		default:
			ctx.getReturn() = 0;
			break;
	}
}

void Functions::doDoorAction(Aurora::NWScript::FunctionContext &ctx) {
	Door *door = ObjectContainer::toDoor(getParamObject(ctx, 0));
	int action = ctx.getParams()[1].getInt();
	Creature *creature = ObjectContainer::toCreature(ctx.getCaller());

	if (!door || !creature)
		return;

	creature->setBlockingDoor(door);

	switch (action) {
		case kDoorActionOpen:
			if (canOpenDoor(creature, door))
				door->open(creature);
			break;
		case kDoorActionUnlock: {
			Action lockAction(kActionOpenLock);
			lockAction.object = door;
			lockAction.range = 1.5f;
			creature->addAction(lockAction);
			break;
		}
		case kDoorActionBash: {
			Action attackAction(kActionAttackObject);
			attackAction.object = door;
			attackAction.range = creature->getMaxAttackRange();
			creature->addAction(attackAction);
			break;
		}
		case kDoorActionIgnore:
			door->setLocalInt("XOREOS_IGNORED", 1);
			break;
		case kDoorActionKnock: {
			Action spellAction(kActionCastSpell);
			spellAction.object = door;
			spellAction.actionID = 93;
			creature->addAction(spellAction);
			break;
		}
		default:
			break;
	}
}

void Functions::getBlockingDoor(Aurora::NWScript::FunctionContext &ctx) {
	Creature *creature = ObjectContainer::toCreature(ctx.getCaller());
	ctx.getReturn() = creature ? creature->getBlockingDoor() : static_cast<Aurora::NWScript::Object *>(nullptr);
}

} // End of namespace KotORBase

} // End of namespace Engines
