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
 *  Star Wars: Knights of the Old Republic engine functions handling the party.
 */

#include <memory>

#include "src/common/util.h"

#include "src/aurora/nwscript/functioncontext.h"

#include "src/engines/kotorbase/types.h"
#include "src/engines/kotorbase/object.h"
#include "src/engines/kotorbase/module.h"
#include "src/engines/kotorbase/objectcontainer.h"
#include "src/engines/kotorbase/game.h"
#include "src/engines/kotorbase/creature.h"

#include "src/engines/kotorbase/script/functions.h"

namespace Engines {

namespace KotORBase {

void Functions::isObjectPartyMember(Aurora::NWScript::FunctionContext &ctx) {
	Aurora::NWScript::Object *object = ctx.getParams()[0].getObject();

	Creature *creature = ObjectContainer::toCreature(object);

	if (!creature) {
		warning("Functions::isObjectPartyMember(): invalid creature");
		ctx.getReturn() = 0;
		return;
	}

	ctx.getReturn() = _game->getModule().isObjectPartyMember(creature);
}

void Functions::getPartyMemberByIndex(Aurora::NWScript::FunctionContext &ctx) {
	int index = ctx.getParams()[0].getInt();

	ctx.getReturn() = _game->getModule().getPartyMemberByIndex(index);
}

void Functions::getSoloMode(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = _game->getModule().isSoloMode();
}

void Functions::getCommandable(Aurora::NWScript::FunctionContext &ctx) {
	Creature *target = ObjectContainer::toCreature(ctx.getParams()[0].getObject());
	if (!target) {
		warning("Functions::getCommandable(): invalid target");
		ctx.getReturn() = 0;
		return;
	}

	ctx.getReturn() = target->isCommandable();
}

void Functions::showPartySelectionGUI(Aurora::NWScript::FunctionContext &ctx) {
	const Common::UString &exitScript = ctx.getParams()[0].getString();
	int forceNPC1 = ctx.getParams()[1].getInt();
	int forceNPC2 = ctx.getParams()[2].getInt();

	_game->getModule().showPartySelectionGUI(forceNPC1, forceNPC2);
	ScriptContainer::runScript(exitScript, ctx.getCaller(), ctx.getTriggerer());
}

void Functions::isAvailableCreature(Aurora::NWScript::FunctionContext &ctx) {
	int slot = ctx.getParams()[0].getInt();

	ctx.getReturn() = _game->getModule().isAvailableCreature(slot);
}

void Functions::addAvailableNPCByTemplate(Aurora::NWScript::FunctionContext &ctx) {
	const int slot = ctx.getParams()[0].getInt();
	const Common::UString &templ = ctx.getParams()[1].getString();

	_game->getModule().addAvailableNPCByTemplate(slot, templ);
}

void Functions::setPartyLeader(Aurora::NWScript::FunctionContext &ctx) {
	int npc = ctx.getParams()[0].getInt();
	_game->getModule().setPartyLeader(npc);
}

void Functions::setCommandable(Aurora::NWScript::FunctionContext &ctx) {
	int commandable = ctx.getParams()[0].getInt();
	Creature *target = ObjectContainer::toCreature(ctx.getParams()[1].getObject());
	if (!target) {
		warning("Functions::setCommandable(): invalid target");
		return;
	}

	target->setCommandable(commandable != 0);
}

void Functions::getPartyMemberCount(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = _game->getModule().getPartyMemberCount();
}

void Functions::setSoloMode(Aurora::NWScript::FunctionContext &ctx) {
	bool enabled = ctx.getParams()[0].getInt() != 0;
	_game->getModule().setSoloMode(enabled);
}

void Functions::isNPCPartyMember(Aurora::NWScript::FunctionContext &ctx) {
	int npc = ctx.getParams()[0].getInt();
	ctx.getReturn() = _game->getModule().isNPCPartyMember(npc);
}

void Functions::addPartyMember(Aurora::NWScript::FunctionContext &ctx) {
	int npc = ctx.getParams()[0].getInt();
	Creature *creature = ObjectContainer::toCreature(ctx.getParams()[1].getObject());
	if (!creature)
		return;
	_game->getModule().addPartyMember(npc, creature);
}

void Functions::removePartyMember(Aurora::NWScript::FunctionContext &ctx) {
	int npc = ctx.getParams()[0].getInt();
	_game->getModule().removePartyMember(npc);
}

void Functions::addAvailableNPCByObject(Aurora::NWScript::FunctionContext &ctx) {
	int npc = ctx.getParams()[0].getInt();
	Creature *creature = ObjectContainer::toCreature(ctx.getParams()[1].getObject());
	_game->getModule().addAvailableNPCByObject(npc, creature);
}

void Functions::removeAvailableNPC(Aurora::NWScript::FunctionContext &ctx) {
	int npc = ctx.getParams()[0].getInt();
	_game->getModule().removeAvailableNPC(npc);
}

void Functions::spawnAvailableNPC(Aurora::NWScript::FunctionContext &ctx) {
	int npc = ctx.getParams()[0].getInt();
	Common::UString waypointTag = ctx.getParams()[1].getString();
	_game->getModule().spawnAvailableNPC(npc, waypointTag);
}

void Functions::getInfluence(Aurora::NWScript::FunctionContext &ctx) {
	Creature *creature = ObjectContainer::toCreature(ctx.getParams()[0].getObject());
	ctx.getReturn() = creature ? creature->getInfluence() : 50;
}

void Functions::setInfluence(Aurora::NWScript::FunctionContext &ctx) {
	Creature *creature = ObjectContainer::toCreature(ctx.getParams()[0].getObject());
	int influence = ctx.getParams()[1].getInt();
	if (creature)
		creature->setInfluence(influence);
}

} // End of namespace KotORBase

} // End of namespace Engines
