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
#include "src/engines/kotorbase/game.h"
#include "src/engines/kotorbase/module.h"
#include "src/engines/kotorbase/script/functions.h"

namespace Engines {

namespace KotORBase {

static const Common::UString kEncounterActiveKey("XOREOS_ENC_ACTIVE");
static const Common::UString kEncounterSpawnMaxKey("XOREOS_ENC_SPAWN_MAX");
static const Common::UString kEncounterSpawnCurKey("XOREOS_ENC_SPAWN_CUR");

static Object *getEncounterObject(Aurora::NWScript::FunctionContext &ctx, size_t paramIndex = 0) {
	return ObjectContainer::toObject(getParamObject(ctx, paramIndex));
}

void Functions::showUpgradeScreen(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	_game->getModule().setGlobalBoolean("__open_workbench", true);
}

void Functions::getEncounterActive(Aurora::NWScript::FunctionContext &ctx) {
	Object *encounter = getEncounterObject(ctx);
	ctx.getReturn() = (encounter && encounter->getLocalInt(kEncounterActiveKey) != 0) ? 1 : 0;
}

void Functions::setEncounterActive(Aurora::NWScript::FunctionContext &ctx) {
	int active = ctx.getParams()[0].getInt();
	Object *encounter = getEncounterObject(ctx, 1);
	if (encounter)
		encounter->setLocalInt(kEncounterActiveKey, active != 0 ? 1 : 0);
}

void Functions::getEncounterSpawnsMax(Aurora::NWScript::FunctionContext &ctx) {
	Object *encounter = getEncounterObject(ctx);
	ctx.getReturn() = encounter ? encounter->getLocalInt(kEncounterSpawnMaxKey) : 0;
}

void Functions::setEncounterSpawnsMax(Aurora::NWScript::FunctionContext &ctx) {
	int maxSpawns = ctx.getParams()[0].getInt();
	Object *encounter = getEncounterObject(ctx, 1);
	if (encounter)
		encounter->setLocalInt(kEncounterSpawnMaxKey, maxSpawns);
}

void Functions::getEncounterSpawnsCurrent(Aurora::NWScript::FunctionContext &ctx) {
	Object *encounter = getEncounterObject(ctx);
	ctx.getReturn() = encounter ? encounter->getLocalInt(kEncounterSpawnCurKey) : 0;
}

void Functions::setEncounterSpawnsCurrent(Aurora::NWScript::FunctionContext &ctx) {
	int current = ctx.getParams()[0].getInt();
	Object *encounter = getEncounterObject(ctx, 1);
	if (encounter)
		encounter->setLocalInt(kEncounterSpawnCurKey, current);
}

} // End of namespace KotORBase

} // End of namespace Engines
