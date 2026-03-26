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
 *  Star Wars: Knights of the Old Republic engine functions operating on the current module.
 */

#include "src/common/maths.h"
#include "src/common/util.h"

#include "src/aurora/nwscript/functioncontext.h"

#include "src/engines/kotorbase/module.h"
#include "src/engines/kotorbase/objectcontainer.h"
#include "src/engines/kotorbase/game.h"

#include "src/engines/kotorbase/script/functions.h"

namespace Engines {

namespace KotORBase {

void Functions::getModule(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = (Aurora::NWScript::Object *) &_game->getModule();
}

void Functions::getFirstPC(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = (Aurora::NWScript::Object *) _game->getModule().getPC();
}

void Functions::getNextPC(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = (Aurora::NWScript::Object *) 0;
}

void Functions::getPCSpeaker(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = (Aurora::NWScript::Object *) _game->getModule().getPC();
}

void Functions::getIsConversationActive(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = _game->getModule().isConversationActive();
}

void Functions::switchPlayerCharacter(Aurora::NWScript::FunctionContext &ctx) {
	const int npc = ctx.getParams()[0].getInt();
	_game->getModule().setPartyLeader(npc);
	ctx.getReturn() = 1;
}

void Functions::setTime(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	// Time-of-day simulation is not yet modeled; keep script flow alive.
}

void Functions::setAreaUnescapable(Aurora::NWScript::FunctionContext &ctx) {
	const bool unescapable = ctx.getParams()[0].getInt() != 0;
	_game->getModule().setGlobalBoolean("__area_unescapable", unescapable);
}

void Functions::getAreaUnescapable(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = _game->getModule().getGlobalBoolean("__area_unescapable") ? 1 : 0;
}

void Functions::getCurrentForcePoints(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = 0;
}

void Functions::getMaxForcePoints(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = 0;
}

void Functions::pauseGame(Aurora::NWScript::FunctionContext &ctx) {
	const bool paused = ctx.getParams()[0].getInt() != 0;
	_game->getModule().setGlobalBoolean("__game_paused", paused);
}

void Functions::setPlayerRestrictMode(Aurora::NWScript::FunctionContext &ctx) {
	_game->getModule().setGlobalNumber("__player_restrict_mode", ctx.getParams()[0].getInt());
}

void Functions::getPlayerRestrictMode(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = _game->getModule().getGlobalNumber("__player_restrict_mode");
}

void Functions::setCameraFacing(Aurora::NWScript::FunctionContext &ctx) {
	float yaw = ctx.getParams()[0].getFloat();
	if ((yaw < -6.5f) || (yaw > 6.5f))
		yaw = Common::deg2rad(yaw);
	_game->getModule().setCameraYaw(yaw);
}

void Functions::getListenPatternNumber(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = 0;
}

void Functions::getLastSpeaker(Aurora::NWScript::FunctionContext &ctx) {
	Aurora::NWScript::Object *speaker = ctx.getCaller();
	if (!speaker)
		speaker = (Aurora::NWScript::Object *) _game->getModule().getPC();

	ctx.getReturn() = speaker;
}

void Functions::getPartyAIStyle(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = 0;
}

void Functions::getNPCAIStyle(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = 0;
}

void Functions::shipBuild(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = 0;
}

void Functions::startNewModule(Aurora::NWScript::FunctionContext &ctx) {
	const Common::UString mod = ctx.getParams()[0].getString();

	if (!_game->hasModule(mod)) {
		warning("Can't start module \"%s\": No such module", mod.c_str());
		return;
	}

	_game->getModule().load(mod);
}

void Functions::setGlobalFadeOut(Aurora::NWScript::FunctionContext &ctx) {
	float wait = ctx.getParams()[0].getFloat();
	float run = ctx.getParams()[1].getFloat();
	float r = ctx.getParams()[2].getFloat();
	float g = ctx.getParams()[3].getFloat();
	float b = ctx.getParams()[4].getFloat();

	Graphics::Aurora::FadeQuad &fadeQuad = _game->getModule().getFadeQuad();

	fadeQuad.setColor(r, g, b);
	fadeQuad.setWaitTime(wait);
	fadeQuad.setRunTime(run);
	fadeQuad.fadeOut();
}

void Functions::setGlobalFadeIn(Aurora::NWScript::FunctionContext &ctx) {
	float wait = ctx.getParams()[0].getFloat();
	float run = ctx.getParams()[1].getFloat();
	float r = ctx.getParams()[2].getFloat();
	float g = ctx.getParams()[3].getFloat();
	float b = ctx.getParams()[4].getFloat();

	Graphics::Aurora::FadeQuad &fadeQuad = _game->getModule().getFadeQuad();

	fadeQuad.setColor(r, g, b);
	fadeQuad.setWaitTime(wait);
	fadeQuad.setRunTime(run);
	fadeQuad.fadeIn();
}

void Functions::getLoadFromSaveGame(Aurora::NWScript::FunctionContext &ctx) {
	// Returns TRUE if the current module was entered by loading a saved game
	// rather than by a normal module transition.
	ctx.getReturn() = _game->getModule().isLoadedFromSaveGame() ? 1 : 0;
}

void Functions::showLevelUpGUI(Aurora::NWScript::FunctionContext &ctx) {
	// ShowLevelUpGUI() — presents the level-up screen for the PC.
	// The full level-up GUI is out of scope for Milestone 2; log and return.
	warning("Functions::showLevelUpGUI: level-up GUI not yet implemented");
	ctx.getReturn() = 0;
}

void Functions::openStore(Aurora::NWScript::FunctionContext &ctx) {
	// OpenStore(object oStore, object oPC, int nBonusMarkUp=0, int nBonusMarkDown=0)
	// Merchant UI is out of scope for Milestone 2; log so scripts are not blocked.
	warning("Functions::openStore: merchant GUI not yet implemented");
}

void Functions::setReturnStrref(Aurora::NWScript::FunctionContext &ctx) {
	bool show = (ctx.getParams()[0].getInt() != 0);
	int returnStrref = ctx.getParams()[1].getInt();
	int returnQueryStrref = ctx.getParams()[2].getInt();

	if (show) {
		_game->getModule().setReturnStrref(returnStrref);
		_game->getModule().setReturnQueryStrref(returnQueryStrref);
	} else {
		// if the travel system is deactivated change the string to "Travelsystem deactivated"
		_game->getModule().setReturnStrref(38550);
	}
	_game->getModule().setReturnEnabled(show);
}

void Functions::getTransitionTarget(Aurora::NWScript::FunctionContext &ctx) {
	// Simple stub that allows transition checks to fail cleanly rather than crash.
	// We'd look up the transition destination tag here if implemented fully.
	ctx.getReturn() = (Aurora::NWScript::Object *) nullptr;
}

void Functions::getModuleName(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = _game->getModule().getName();
}

void Functions::getModuleFileName(Aurora::NWScript::FunctionContext &ctx) {
	// GetModuleFileName() -> string
	// Returns the resource reference (filename without extension) of the
	// currently loaded module, e.g. "end_m01aa".
	ctx.getReturn() = _game->getModule().getResRef();
}

void Functions::setAreaTransitionBMP(Aurora::NWScript::FunctionContext &ctx) {
	int bmp = ctx.getParams()[0].getInt();
	int strref = ctx.getParams()[1].getInt();
	info("Transition BMP set: %d (strref %d)", bmp, strref);
}

void Functions::endGame(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	// Graceful fallback: end the current module session and let the engine
	// return to menu flow instead of halting on missing script support.
	_game->getModule().exit();
}

void Functions::showGalaxyMap(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	_game->getModule().showGalaxyMap();
}

void Functions::setPlanetSelectable(Aurora::NWScript::FunctionContext &ctx) {
	int planet = ctx.getParams()[0].getInt();
	bool selectable = ctx.getParams()[1].getInt() != 0;

	_game->getModule().setPlanetSelectable(planet, selectable);
}

void Functions::getPlanetSelectable(Aurora::NWScript::FunctionContext &ctx) {
	int planet = ctx.getParams()[0].getInt();
	ctx.getReturn() = _game->getModule().getPlanetSelectable(planet) ? 1 : 0;
}

void Functions::setPlanetAvailable(Aurora::NWScript::FunctionContext &ctx) {
	int planet = ctx.getParams()[0].getInt();
	bool available = ctx.getParams()[1].getInt() != 0;

	_game->getModule().setPlanetAvailable(planet, available);
}

void Functions::getPlanetAvailable(Aurora::NWScript::FunctionContext &ctx) {
	int planet = ctx.getParams()[0].getInt();
	ctx.getReturn() = _game->getModule().getPlanetAvailable(planet) ? 1 : 0;
}

void Functions::getSelectedPlanet(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = _game->getModule().getSelectedPlanet();
}

} // End of namespace KotORBase

} // End of namespace Engines
