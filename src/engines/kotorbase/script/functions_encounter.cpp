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
 *  KotOR NWScript encounter and upgrade-screen helpers.
 */

#include "src/aurora/nwscript/functioncontext.h"

#include "src/engines/kotorbase/object.h"
#include "src/engines/kotorbase/objectcontainer.h"
#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/encounter.h"
#include "src/engines/kotorbase/area.h"
#include "src/engines/kotorbase/game.h"
#include "src/engines/kotorbase/module.h"
#include "src/engines/kotorbase/script/functions.h"

namespace Engines {

namespace KotORBase {

static Encounter *toEncounter(Aurora::NWScript::Object *object) {
	return dynamic_cast<Encounter *>(object);
}

void Functions::showUpgradeScreen(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	_game->getModule().setGlobalBoolean("__open_workbench", true);
}

void Functions::getEncounterActive(Aurora::NWScript::FunctionContext &ctx) {
	Encounter *encounter = toEncounter(getParamObject(ctx, 0));
	ctx.getReturn() = (encounter && encounter->isActive()) ? 1 : 0;
}

void Functions::setEncounterActive(Aurora::NWScript::FunctionContext &ctx) {
	const int active = ctx.getParams()[0].getInt();
	Encounter *encounter = toEncounter(getParamObject(ctx, 1));
	if (encounter)
		encounter->setActive(active != 0);
}

void Functions::getEncounterSpawnsMax(Aurora::NWScript::FunctionContext &ctx) {
	Encounter *encounter = toEncounter(getParamObject(ctx, 0));
	ctx.getReturn() = encounter ? encounter->getSpawnsMax() : 0;
}

void Functions::setEncounterSpawnsMax(Aurora::NWScript::FunctionContext &ctx) {
	const int maxSpawns = ctx.getParams()[0].getInt();
	Encounter *encounter = toEncounter(getParamObject(ctx, 1));
	if (encounter)
		encounter->setSpawnsMax(maxSpawns);
}

void Functions::getEncounterSpawnsCurrent(Aurora::NWScript::FunctionContext &ctx) {
	Encounter *encounter = toEncounter(getParamObject(ctx, 0));
	ctx.getReturn() = encounter ? encounter->getSpawnsCurrent() : 0;
}

void Functions::setEncounterSpawnsCurrent(Aurora::NWScript::FunctionContext &ctx) {
	const int current = ctx.getParams()[0].getInt();
	Encounter *encounter = toEncounter(getParamObject(ctx, 1));
	if (encounter)
		encounter->setSpawnsCurrent(current);
}

void Functions::setEncounterDifficulty(Aurora::NWScript::FunctionContext &ctx) {
	// SetEncounterDifficulty(int nEncounterDifficulty, object oEncounter = OBJECT_SELF)
	const int difficulty = ctx.getParams()[0].getInt();
	Encounter *encounter = toEncounter(getParamObject(ctx, 1));
	if (encounter)
		encounter->setDifficultyIndex(difficulty);
}

void Functions::getEncounterDifficulty(Aurora::NWScript::FunctionContext &ctx) {
	Encounter *encounter = toEncounter(getParamObject(ctx, 0));
	ctx.getReturn() = encounter ? encounter->getDifficultyIndex() : 0;
}

void Functions::getIsEncounterCreature(Aurora::NWScript::FunctionContext &ctx) {
	// Was the creature spawned by an encounter (and is it still counted by it)?
	Creature *creature = ObjectContainer::toCreature(getParamObject(ctx, 0));
	Area *area = _game->getModule().getCurrentArea();

	ctx.getReturn() = (creature && area && area->findEncounterOwning(*creature)) ? 1 : 0;
}

} // End of namespace KotORBase

} // End of namespace Engines
