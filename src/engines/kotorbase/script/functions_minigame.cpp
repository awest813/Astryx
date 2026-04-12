#include "src/common/debug.h"
#include "src/aurora/nwscript/functioncontext.h"

#include "src/engines/kotorbase/module.h"
#include "src/engines/kotorbase/game.h"
#include "src/engines/kotorbase/objectcontainer.h"

#include "src/engines/kotorbase/script/functions.h"

namespace Engines {

namespace KotORBase {

void Functions::playPazaak(Aurora::NWScript::FunctionContext &ctx) {
	// void PlayPazaak(int nMaxWager, int nWagerSide, object oOpponent = OBJECT_INVALID)
	int maxWager = ctx.getParams()[0].getInt();
	int wagerSide = ctx.getParams()[1].getInt();
	Object *opponent = ObjectContainer::toObject(ctx.getParams()[2].getObject());

	debug("PlayPazaak: MaxWager %d, Side %d, Opponent %s", 
	      maxWager, wagerSide, opponent ? opponent->getTag().c_str() : "INVALID");

	_game->getModule().setGlobalNumber("__pazaak_wager", maxWager);
	_game->getModule().setGlobalNumber("__pazaak_side", wagerSide);
	
	// Signal a start encounter for Pazaak
	_game->getModule().signalEncounter("pazaak_start");
}

void Functions::getLastPazaakResult(Aurora::NWScript::FunctionContext &ctx) {
	// Returns 1 for win, 0 for loss/draw
	ctx.getReturn() = _game->getModule().getGlobalNumber("__pazaak_result");
}

void Functions::swmgSetLateralAccelerationPerSecond(Aurora::NWScript::FunctionContext &ctx) {
	float accel = ctx.getParams()[0].getFloat();
	_game->getModule().setGlobalNumber("__swmg_lateral_accel", static_cast<int>(accel * 100)); // Store scaled
}

void Functions::swmgGetLateralAccelerationPerSecond(Aurora::NWScript::FunctionContext &ctx) {
	float accel = _game->getModule().getGlobalNumber("__swmg_lateral_accel") / 100.0f;
	ctx.getReturn() = accel;
}

void Functions::swmgSetPlayerSpeed(Aurora::NWScript::FunctionContext &ctx) {
	float speed = ctx.getParams()[0].getFloat();
	_game->getModule().setGlobalNumber("__swmg_player_speed", static_cast<int>(speed * 100));
}

void Functions::swmgGetPlayerSpeed(Aurora::NWScript::FunctionContext &ctx) {
	float speed = _game->getModule().getGlobalNumber("__swmg_player_speed") / 100.0f;
	ctx.getReturn() = speed;
}

void Functions::swmgSetPlayerMaxSpeed(Aurora::NWScript::FunctionContext &ctx) {
	float maxSpeed = ctx.getParams()[0].getFloat();
	_game->getModule().setGlobalNumber("__swmg_player_max_speed", static_cast<int>(maxSpeed * 100));
}

void Functions::swmgGetPlayerMaxSpeed(Aurora::NWScript::FunctionContext &ctx) {
	float maxSpeed = _game->getModule().getGlobalNumber("__swmg_player_max_speed") / 100.0f;
	ctx.getReturn() = maxSpeed;
}

void Functions::swmgOnObstacleHit(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	// SWMG_OnObstacleHit() is a trigger for swoop scripts to handle damage/slowdown.
	// We signal a generic minigame event to the module.
	_game->getModule().signalEncounter("swmg_obstacle_hit");
}

void Functions::swmgGetLastEvent(Aurora::NWScript::FunctionContext &ctx) {
	// Simple stub: usually returns event codes (1=Hit, 2=Boost, etc)
	ctx.getReturn() = _game->getModule().getGlobalNumber("__swmg_last_event");
}

void Functions::swmgSetSpeedBlurEffect(Aurora::NWScript::FunctionContext &ctx) {
	// SWMG_SetSpeedBlurEffect(int bEnabled)
	bool enabled = ctx.getParams()[0].getInt() != 0;
	_game->getModule().setGlobalBoolean("__swmg_blur_enabled", enabled);
}


void Functions::swmgGetLastBulletHitPart(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = 0; }
void Functions::swmgIsGunBankTargetting(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = 0; }
void Functions::swmgGetPlayerOffset(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = 0.0f; }
void Functions::swmgSetPlayerOffset(Aurora::NWScript::FunctionContext &ctx) {}
void Functions::swmgGetEnemyCount(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = 0; }
void Functions::swmgGetEnemy(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = (Aurora::NWScript::Object *)nullptr; }

} // End of namespace KotORBase

} // End of namespace Engines
