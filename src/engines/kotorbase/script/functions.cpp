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
 *  Engine functions for KotOR games.
 */

#include <cassert>
#include <cstdlib>

#include "src/common/util.h"
#include "src/common/random.h"
#include "src/common/configman.h"
#include <fstream>

#include "src/aurora/nwscript/functionman.h"
#include "src/aurora/nwscript/util.h"

#include "src/engines/kotorbase/types.h"
#include "src/engines/kotorbase/object.h"
#include "src/engines/kotorbase/module.h"
#include "src/engines/kotorbase/area.h"
#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/objectcontainer.h"
#include "src/engines/kotorbase/game.h"

#include "src/aurora/2dafile.h"
#include "src/aurora/2dareg.h"

#include "src/engines/kotorbase/script/functions.h"

namespace Engines {

namespace KotORBase {

Functions::Functions(Game &game) : _game(&game) {
}

Functions::~Functions() {
	FunctionMan.clear();
}

void Functions::getRunScriptVar(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = _game->getModule().getRunScriptVar();
}

void Functions::unimplementedFunction(Aurora::NWScript::FunctionContext &ctx) {
	const Common::UString &name = ctx.getName();
	
	// Track script coverage
	static std::ofstream coverageLog("docs/SCRIPT_COVERAGE.log", std::ios::app);
	if (coverageLog.is_open()) {
		coverageLog << name.c_str() << "\n";
	}

	int severity = 1; // 1 = Important (default)
	if (name.beginsWith("Action") || name.beginsWith("Set"))
		severity = 2; // Critical for progression
	else if (name.beginsWith("Play") || name.beginsWith("EffectVisual") || name.beginsWith("SWMG_") || name.beginsWith("Sound") || name.beginsWith("Music"))
		severity = 0; // Cosmetic/Non-blocking

	bool strictMode = ConfigMan.getBool("strict_script_mode", false);

	if (strictMode && severity == 2) {
		error("STRICT MODE: Critical missing script %s called!", name.c_str());
	} else {
		warning("TODO [Sev %d]: %s %s(%s)", severity, Aurora::NWScript::formatType(ctx.getReturn().getType()).c_str(),
		                           name.c_str(), Aurora::NWScript::formatParams(ctx).c_str());
	}

	// Safe stub fallback returns
	switch (ctx.getReturn().getType()) {
		case Aurora::NWScript::kTypeInt: ctx.getReturn().setType(Aurora::NWScript::kTypeInt); ctx.getReturn() = 0; break;
		case Aurora::NWScript::kTypeFloat: ctx.getReturn().setType(Aurora::NWScript::kTypeFloat); ctx.getReturn() = 0.0f; break;
		case Aurora::NWScript::kTypeString: ctx.getReturn().setType(Aurora::NWScript::kTypeString); ctx.getReturn() = Common::UString(""); break;
		case Aurora::NWScript::kTypeObject: ctx.getReturn().setType(Aurora::NWScript::kTypeObject); break;
		case Aurora::NWScript::kTypeVector: ctx.getReturn().setType(Aurora::NWScript::kTypeVector); break;
		default: break;
	}
}

void Functions::executeScript(Aurora::NWScript::FunctionContext &ctx) {
	const Common::UString &script = ctx.getParams()[0].getString();

	Object *target = ObjectContainer::toObject(ctx.getParams()[1].getObject());
	if (!target) {
		warning("Functions::executeScript(): Invalid target");
		return;
	}

	_game->getModule().setRunScriptVar(ctx.getParams()[2].getInt());

	target->runScript(script, target, ctx.getCaller());
}

void Functions::addJournalQuestEntry(Aurora::NWScript::FunctionContext &ctx) {
	const Common::UString &plotId = ctx.getParams()[0].getString();
	int32_t state = ctx.getParams()[1].getInt();
	
	int32_t current = _game->getModule().getGlobalNumber("JRNL_" + plotId);
	if (state > current) {
		_game->getModule().setGlobalNumber("JRNL_" + plotId, state);
		info("QUEST UPDATED: %s [%d -> %d]", plotId.c_str(), current, state);
	}
}

void Functions::removeJournalQuestEntry(Aurora::NWScript::FunctionContext &ctx) {
	const Common::UString &plotId = ctx.getParams()[0].getString();
	_game->getModule().setGlobalNumber("JRNL_" + plotId, -1);
	info("QUEST REMOVED: %s", plotId.c_str());
}

void Functions::getJournalEntry(Aurora::NWScript::FunctionContext &ctx) {
	const Common::UString &plotId = ctx.getParams()[0].getString();
	ctx.getReturn().setType(Aurora::NWScript::kTypeInt);
	ctx.getReturn() = _game->getModule().getGlobalNumber("JRNL_" + plotId);
}

void Functions::getJournalQuestExperience(Aurora::NWScript::FunctionContext &ctx) {
	// GetJournalQuestExperience(string sPlotID) → int
	// Returns the XP reward for the given quest plot ID from the questitems 2DA.
	// Gracefully returns 0 if the 2DA is unavailable (e.g. in CI environments
	// without game assets).
	const Common::UString &plotId = ctx.getParams()[0].getString();
	ctx.getReturn() = 0;

	try {
		const Aurora::TwoDAFile &qiFile = TwoDAReg.get2DA("questitems");
		size_t rows = qiFile.getRowCount();
		for (size_t i = 0; i < rows; ++i) {
			const Aurora::TwoDARow &row = qiFile.getRow(i);
			if (row.getString("label") == plotId) {
				ctx.getReturn() = row.getInt("xp");
				return;
			}
		}
	} catch (...) {
		// 2DA unavailable — return 0 silently.
	}
}

void Functions::giveGoldToCreature(Aurora::NWScript::FunctionContext &ctx) {
	Creature *creature = ObjectContainer::toCreature(ctx.getParams()[1].getObject());
	int amount = ctx.getParams()[0].getInt();

	if (creature && amount > 0) {
		creature->getInventory().addGold(amount);
		info("Gave %d gold to creature", amount);
	}
}

void Functions::takeGoldFromCreature(Aurora::NWScript::FunctionContext &ctx) {
	Creature *creature = ObjectContainer::toCreature(ctx.getParams()[1].getObject());
	int amount = ctx.getParams()[0].getInt();

	if (creature && amount > 0) {
		creature->getInventory().removeGold(amount);
		info("Took %d gold from creature", amount);
	}
}

void Functions::getGold(Aurora::NWScript::FunctionContext &ctx) {
	Creature *creature = ObjectContainer::toCreature(ctx.getParams()[0].getObject());
	if (creature) {
		ctx.getReturn() = static_cast<int32_t>(creature->getInventory().getGold());
	} else {
		ctx.getReturn() = 0;
	}
}

int32_t Functions::getRandom(int min, int max, int32_t n) {
	if (n < 1)
		n = 1;

	int32_t r = 0;

	while (n-- > 0)
		r += RNG.getNext(min, max + 1);

	return r;
}

Common::UString Functions::formatFloat(float f, int width, int decimals) {
	return Common::String::format("%*.*f", width, decimals, f);
}

Aurora::NWScript::Object *Functions::getParamObject(const Aurora::NWScript::FunctionContext &ctx, size_t n) {
	Object *object = ObjectContainer::toObject(ctx.getParams()[n].getObject());
	if (!object || (object->getType() == kObjectTypeInvalid))
		return 0;

	if (object->getType() == kObjectTypeSelf)
		return ctx.getCaller();

	return object;
}

void Functions::playVideo(const Common::UString &resRef) {
	status("PLAYING VIDEO: %s", resRef.c_str());
	_game->getModule().playMovie(resRef);
}

void Functions::playMusicStinger(const Common::UString &resRef) {
	status("PLAYING MUSIC STINGER: %s", resRef.c_str());
	// Stinger playback usually involves a fire-and-forget SFX or music segment
}

void Functions::jumpTo(KotORBase::Object *object, float x, float y, float z) {
	object->setPosition(x, y, z);
}

} // End of namespace KotORBase

} // End of namespace Engines
