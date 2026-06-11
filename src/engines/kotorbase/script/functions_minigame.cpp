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
 *  NWScript functions for KotOR minigames (Pazaak, SWMG).
 */

#include "src/common/debug.h"

#include "src/aurora/nwscript/functioncontext.h"

#include "src/engines/kotorbase/module.h"
#include "src/engines/kotorbase/game.h"
#include "src/engines/kotorbase/objectcontainer.h"
#include "src/engines/kotorbase/swoopminigame.h"

#include "src/engines/kotorbase/script/functions.h"

namespace Engines {

namespace KotORBase {

void Functions::playPazaak(Aurora::NWScript::FunctionContext &ctx) {
	int maxWager = ctx.getParams()[0].getInt();
	int wagerSide = ctx.getParams()[1].getInt();
	Object *opponent = ObjectContainer::toObject(ctx.getParams()[2].getObject());

	debugC(Common::kDebugEngineLogic, 1, "PlayPazaak: MaxWager %d, Side %d, Opponent %s",
	       maxWager, wagerSide, opponent ? opponent->getTag().c_str() : "INVALID");

	_game->getModule().setGlobalNumber("__pazaak_wager", maxWager);
	_game->getModule().setGlobalNumber("__pazaak_side", wagerSide);

	_game->getModule().signalEncounter("pazaak_start");
}

void Functions::getLastPazaakResult(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = _game->getModule().getGlobalNumber("__pazaak_result");
}

void Functions::swmgSetLateralAccelerationPerSecond(Aurora::NWScript::FunctionContext &ctx) {
	const float accel = ctx.getParams()[0].getFloat();
	SwoopMinigame::get().setLateralAcceleration(accel);
}

void Functions::swmgGetLateralAccelerationPerSecond(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = SwoopMinigame::get().getLateralAcceleration();
}

void Functions::swmgSetPlayerSpeed(Aurora::NWScript::FunctionContext &ctx) {
	SwoopMinigame::get().setSpeed(ctx.getParams()[0].getFloat());
}

void Functions::swmgGetPlayerSpeed(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = SwoopMinigame::get().getSpeed();
}

void Functions::swmgSetPlayerMaxSpeed(Aurora::NWScript::FunctionContext &ctx) {
	SwoopMinigame::get().setMaxSpeed(ctx.getParams()[0].getFloat());
}

void Functions::swmgGetPlayerMaxSpeed(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = SwoopMinigame::get().getMaxSpeed();
}

void Functions::swmgOnObstacleHit(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	SwoopMinigame::get().onObstacleHit();
	_game->getModule().setGlobalNumber("__swmg_last_event", SwoopMinigame::get().getLastEvent());
	_game->getModule().signalEncounter("swmg_obstacle_hit");
}

void Functions::swmgGetLastEvent(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = SwoopMinigame::get().getLastEvent();
}

void Functions::swmgSetSpeedBlurEffect(Aurora::NWScript::FunctionContext &ctx) {
	bool enabled = ctx.getParams()[0].getInt() != 0;
	_game->getModule().setGlobalBoolean("__swmg_blur_enabled", enabled);
}

void Functions::swmgGetLastBulletHitPart(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = _game->getModule().getGlobalNumber("__swmg_bullet_hit_part");
}

void Functions::swmgIsGunBankTargetting(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = _game->getModule().getGlobalBoolean("__swmg_gunbank_targeting") ? 1 : 0;
}

void Functions::swmgGetPlayerOffset(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = SwoopMinigame::get().getPlayerOffset();
}

void Functions::swmgSetPlayerOffset(Aurora::NWScript::FunctionContext &ctx) {
	SwoopMinigame::get().setPlayerOffset(ctx.getParams()[0].getFloat());
}

void Functions::swmgGetPlayerTunnelPos(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = SwoopMinigame::get().getTunnelPos();
}

void Functions::swmgSetPlayerTunnelPos(Aurora::NWScript::FunctionContext &ctx) {
	SwoopMinigame::get().setTunnelPos(ctx.getParams()[0].getFloat());
}

void Functions::swmgGetPlayerTunnelNeg(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = SwoopMinigame::get().getTunnelNeg();
}

void Functions::swmgSetPlayerTunnelNeg(Aurora::NWScript::FunctionContext &ctx) {
	SwoopMinigame::get().setTunnelNeg(ctx.getParams()[0].getFloat());
}

void Functions::swmgGetEnemyCount(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = _game->getModule().getGlobalNumber("__swmg_enemy_count");
}

void Functions::swmgGetEnemy(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = (Aurora::NWScript::Object *) nullptr;
}

} // End of namespace KotORBase

} // End of namespace Engines
