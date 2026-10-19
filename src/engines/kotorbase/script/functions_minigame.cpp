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

#include <cstdlib>

#include "external/glm/vec3.hpp"

#include "src/common/debug.h"
#include "src/common/ustring.h"
#include "src/common/string.h"

#include "src/aurora/nwscript/functioncontext.h"

#include "src/engines/kotorbase/module.h"
#include "src/engines/kotorbase/game.h"
#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/object.h"
#include "src/engines/kotorbase/objectcontainer.h"
#include "src/engines/kotorbase/swoopminigame.h"

#include "src/engines/kotorbase/script/container.h"
#include "src/engines/kotorbase/script/functions.h"

namespace Engines {

namespace KotORBase {

void Functions::playPazaak(Aurora::NWScript::FunctionContext &ctx) {
	// PlayPazaak(int nOpponentPazaakDeck, string sEndScript, int nMaxWager,
	//            int bShowTutorial=FALSE, object oOpponent=OBJECT_INVALID)
	const int opponentDeck = ctx.getParams()[0].getInt();
	const Common::UString endScript = ctx.getParams()[1].getString();
	const int maxWager = ctx.getParams()[2].getInt();
	const bool showTutorial = ctx.getParams()[3].getInt() != 0;
	Object *opponent = ObjectContainer::toObject(getParamObject(ctx, 4));

	debugC(Common::kDebugEngineLogic, 1, "PlayPazaak: Deck %d, EndScript \"%s\", MaxWager %d, Tutorial %d, Opponent %s",
	       opponentDeck, endScript.c_str(), maxWager, showTutorial ? 1 : 0,
	       opponent ? opponent->getTag().c_str() : "INVALID");

	_game->getModule().setGlobalNumber("__pazaak_wager", maxWager);
	_game->getModule().setGlobalNumber("__pazaak_side", opponentDeck);

	// The match runs modally inside the encounter handler.
	_game->getModule().signalEncounter("pazaak_start");

	// Retail runs sEndScript once the match is over; GetLastPazaakResult()
	// is valid at that point.
	if (!endScript.empty()) {
		Aurora::NWScript::Object *owner = ctx.getCaller();
		ScriptContainer::runScript(endScript, owner,
		                           opponent ? static_cast<Aurora::NWScript::Object *>(opponent) : owner);
	}
}

void Functions::getLastPazaakResult(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = _game->getModule().getGlobalNumber("__pazaak_result");
}

// .--- SWMG: swoop race and turret minigames
//
// The state lives in SwoopMinigame; objects are the minigame's own script
// objects (player, enemies, obstacles), see MinigameRuntime.

namespace {

SwoopMinigame &swmg() {
	return SwoopMinigame::get();
}

glm::vec3 getVectorParam(Aurora::NWScript::FunctionContext &ctx, size_t n) {
	float x = 0.0f, y = 0.0f, z = 0.0f;
	ctx.getParams()[n].getVector(x, y, z);
	return glm::vec3(x, y, z);
}

void setVectorReturn(Aurora::NWScript::FunctionContext &ctx, const glm::vec3 &v) {
	ctx.getReturn().setVector(v.x, v.y, v.z);
}

Aurora::NWScript::Object *asScriptObject(MinigameObject *object) {
	return object;
}

bool isMinigameKind(Aurora::NWScript::Object *object, MinigameObjectKind kind) {
	MinigameObject *minigameObject = swmg().toMinigameObject(object);
	return minigameObject && (minigameObject->getMinigameKind() == kind);
}

} // End of anonymous namespace

void Functions::swmgSetLateralAccelerationPerSecond(Aurora::NWScript::FunctionContext &ctx) {
	swmg().setLateralAcceleration(ctx.getParams()[0].getFloat());
}

void Functions::swmgGetLateralAccelerationPerSecond(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = swmg().getLateralAcceleration();
}

void Functions::swmgSetSpeedBlurEffect(Aurora::NWScript::FunctionContext &ctx) {
	swmg().setSpeedBlur(ctx.getParams()[0].getInt() != 0, ctx.getParams()[1].getFloat());
}

void Functions::swmgGetLastEvent(Aurora::NWScript::FunctionContext &ctx) {
	// OnAnimEvent: the name of the animation event.
	ctx.getReturn() = swmg().getLastEvent();
}

void Functions::swmgGetLastEventModelName(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = swmg().getLastEventModelName();
}

void Functions::swmgGetObjectByName(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = asScriptObject(swmg().getObjectByName(ctx.getParams()[0].getString()));
}

void Functions::swmgPlayAnimation(Aurora::NWScript::FunctionContext &ctx) {
	// SWMG_PlayAnimation(object oObject, string sAnimName, int bLooping=1, int bQueue=0, int bOverlay=0)
	MinigameObject *object = swmg().toMinigameObject(getParamObject(ctx, 0));
	if (object)
		swmg().playAnimation(*object, ctx.getParams()[1].getString(), ctx.getParams()[2].getInt() != 0);
}

void Functions::swmgRemoveAnimation(Aurora::NWScript::FunctionContext &ctx) {
	MinigameObject *object = swmg().toMinigameObject(getParamObject(ctx, 0));
	if (object)
		swmg().removeAnimation(*object, ctx.getParams()[1].getString());
}

void Functions::swmgGetLastBulletHitDamage(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = swmg().getLastBulletHitDamage();
}

void Functions::swmgGetLastBulletHitTarget(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = static_cast<int32_t>(swmg().getLastBulletHitTarget());
}

void Functions::swmgGetLastBulletHitShooter(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = asScriptObject(swmg().getLastBulletHitShooter());
}

void Functions::swmgGetLastBulletHitPart(Aurora::NWScript::FunctionContext &ctx) {
	// Hits are tested against the target's sphere, not its model parts.
	ctx.getReturn() = Common::UString();
}

void Functions::swmgGetLastBulletFiredDamage(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = swmg().getLastBulletFiredDamage();
}

void Functions::swmgGetLastBulletFiredTarget(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = static_cast<int32_t>(swmg().getLastBulletFiredTarget());
}

void Functions::swmgGetLastFollowerHit(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = asScriptObject(swmg().getLastFollowerHit());
}

void Functions::swmgGetLastObstacleHit(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = asScriptObject(swmg().getLastObstacleHit());
}

void Functions::swmgGetLastHPChange(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = swmg().getLastHPChange();
}

void Functions::swmgAdjustFollowerHitPoints(Aurora::NWScript::FunctionContext &ctx) {
	// SWMG_AdjustFollowerHitPoints(object oFollower, int nHP, int nAbsolute=FALSE)
	ctx.getReturn() = swmg().adjustHitPoints(getParamObject(ctx, 0), ctx.getParams()[1].getInt(),
	                                         ctx.getParams()[2].getInt() != 0);
}

void Functions::swmgSetFollowerHitPoints(Aurora::NWScript::FunctionContext &ctx) {
	swmg().adjustHitPoints(getParamObject(ctx, 0), ctx.getParams()[1].getInt(), true);
}

// The default implementations, called from the object's own event script.

void Functions::swmgOnBulletHit(Aurora::NWScript::FunctionContext &ctx) {
	if (MinigameObject *self = swmg().toMinigameObject(ctx.getCaller()))
		swmg().defaultBulletHit(*self);
}

void Functions::swmgOnObstacleHit(Aurora::NWScript::FunctionContext &ctx) {
	if (MinigameObject *self = swmg().toMinigameObject(ctx.getCaller()))
		swmg().defaultObstacleHit(*self);
}

void Functions::swmgOnDeath(Aurora::NWScript::FunctionContext &ctx) {
	if (MinigameObject *self = swmg().toMinigameObject(ctx.getCaller()))
		swmg().defaultDeath(*self);
}

void Functions::swmgOnDamage(Aurora::NWScript::FunctionContext &ctx) {
	if (MinigameObject *self = swmg().toMinigameObject(ctx.getCaller()))
		swmg().defaultDamage(*self);
}

void Functions::swmgGetObjectName(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = swmg().getObjectName(getParamObject(ctx, 0));
}

void Functions::swmgIsFollower(Aurora::NWScript::FunctionContext &ctx) {
	// As in KotOR.js: the enemies riding the tracks, not the player.
	ctx.getReturn() = isMinigameKind(getParamObject(ctx, 0), kMinigameObjectEnemy) ? 1 : 0;
}

void Functions::swmgIsPlayer(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = isMinigameKind(getParamObject(ctx, 0), kMinigameObjectPlayer) ? 1 : 0;
}

void Functions::swmgIsEnemy(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = isMinigameKind(getParamObject(ctx, 0), kMinigameObjectEnemy) ? 1 : 0;
}

void Functions::swmgIsTrigger(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = swmg().isTrigger(getParamObject(ctx, 0)) ? 1 : 0;
}

void Functions::swmgIsObstacle(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = isMinigameKind(getParamObject(ctx, 0), kMinigameObjectObstacle) ? 1 : 0;
}

void Functions::swmgGetCameraNearClip(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = swmg().getCameraNearClip();
}

void Functions::swmgGetCameraFarClip(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = swmg().getCameraFarClip();
}

void Functions::swmgSetCameraClip(Aurora::NWScript::FunctionContext &ctx) {
	swmg().setCameraClip(ctx.getParams()[0].getFloat(), ctx.getParams()[1].getFloat());
}

void Functions::swmgGetPlayer(Aurora::NWScript::FunctionContext &ctx) {
	if (swmg().isActive())
		ctx.getReturn() = asScriptObject(swmg().getPlayer());
	else
		ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(_game->getModule().getPC());
}

void Functions::swmgGetEnemyCount(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = static_cast<int32_t>(swmg().getEnemyCount());
}

void Functions::swmgGetEnemy(Aurora::NWScript::FunctionContext &ctx) {
	const int index = ctx.getParams()[0].getInt();
	ctx.getReturn() = asScriptObject((index < 0) ? nullptr : swmg().getEnemy(static_cast<size_t>(index)));
}

void Functions::swmgGetObstacleCount(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = static_cast<int32_t>(swmg().getObstacleCount());
}

void Functions::swmgGetObstacle(Aurora::NWScript::FunctionContext &ctx) {
	const int index = ctx.getParams()[0].getInt();
	ctx.getReturn() = asScriptObject((index < 0) ? nullptr : swmg().getObstacle(static_cast<size_t>(index)));
}

void Functions::swmgGetHitPoints(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = swmg().getHitPoints(getParamObject(ctx, 0));
}

void Functions::swmgGetMaxHitPoints(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = swmg().getMaxHitPoints(getParamObject(ctx, 0));
}

void Functions::swmgSetMaxHitPoints(Aurora::NWScript::FunctionContext &ctx) {
	swmg().setMaxHitPoints(getParamObject(ctx, 0), ctx.getParams()[1].getInt());
}

void Functions::swmgGetSphereRadius(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = swmg().getSphereRadius(getParamObject(ctx, 0));
}

void Functions::swmgSetSphereRadius(Aurora::NWScript::FunctionContext &ctx) {
	swmg().setSphereRadius(getParamObject(ctx, 0), ctx.getParams()[1].getFloat());
}

void Functions::swmgGetNumLoops(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = swmg().getNumLoops(getParamObject(ctx, 0));
}

void Functions::swmgSetNumLoops(Aurora::NWScript::FunctionContext &ctx) {
	swmg().setNumLoops(getParamObject(ctx, 0), ctx.getParams()[1].getInt());
}

void Functions::swmgGetPosition(Aurora::NWScript::FunctionContext &ctx) {
	setVectorReturn(ctx, swmg().getPosition(getParamObject(ctx, 0)));
}

void Functions::swmgGetGunBankCount(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = swmg().getGunBankCount(getParamObject(ctx, 0));
}

// Gun bank functions: (object oFollower, int nGunBank[, value])

void Functions::swmgGetGunBankBulletModel(Aurora::NWScript::FunctionContext &ctx) {
	const MinigameGunBank *bank = swmg().getGunBank(getParamObject(ctx, 0), ctx.getParams()[1].getInt());
	ctx.getReturn() = bank ? bank->desc.bullet.model : Common::UString();
}

void Functions::swmgGetGunBankGunModel(Aurora::NWScript::FunctionContext &ctx) {
	const MinigameGunBank *bank = swmg().getGunBank(getParamObject(ctx, 0), ctx.getParams()[1].getInt());
	ctx.getReturn() = bank ? bank->desc.gunModel : Common::UString();
}

void Functions::swmgGetGunBankDamage(Aurora::NWScript::FunctionContext &ctx) {
	const MinigameGunBank *bank = swmg().getGunBank(getParamObject(ctx, 0), ctx.getParams()[1].getInt());
	ctx.getReturn() = bank ? bank->desc.bullet.damage : 0;
}

void Functions::swmgGetGunBankTimeBetweenShots(Aurora::NWScript::FunctionContext &ctx) {
	const MinigameGunBank *bank = swmg().getGunBank(getParamObject(ctx, 0), ctx.getParams()[1].getInt());
	ctx.getReturn() = bank ? bank->desc.bullet.rateOfFire : 0.0f;
}

void Functions::swmgGetGunBankLifespan(Aurora::NWScript::FunctionContext &ctx) {
	const MinigameGunBank *bank = swmg().getGunBank(getParamObject(ctx, 0), ctx.getParams()[1].getInt());
	ctx.getReturn() = bank ? bank->desc.bullet.lifespan : 0.0f;
}

void Functions::swmgGetGunBankSpeed(Aurora::NWScript::FunctionContext &ctx) {
	const MinigameGunBank *bank = swmg().getGunBank(getParamObject(ctx, 0), ctx.getParams()[1].getInt());
	ctx.getReturn() = bank ? bank->desc.bullet.speed : 0.0f;
}

void Functions::swmgGetGunBankTarget(Aurora::NWScript::FunctionContext &ctx) {
	const MinigameGunBank *bank = swmg().getGunBank(getParamObject(ctx, 0), ctx.getParams()[1].getInt());
	ctx.getReturn() = bank ? static_cast<int32_t>(bank->desc.bullet.targetType) : 0;
}

void Functions::swmgGetGunBankHorizontalSpread(Aurora::NWScript::FunctionContext &ctx) {
	const MinigameGunBank *bank = swmg().getGunBank(getParamObject(ctx, 0), ctx.getParams()[1].getInt());
	ctx.getReturn() = bank ? bank->desc.horizontalSpread : 0.0f;
}

void Functions::swmgGetGunBankVerticalSpread(Aurora::NWScript::FunctionContext &ctx) {
	const MinigameGunBank *bank = swmg().getGunBank(getParamObject(ctx, 0), ctx.getParams()[1].getInt());
	ctx.getReturn() = bank ? bank->desc.verticalSpread : 0.0f;
}

void Functions::swmgGetGunBankSensingRadius(Aurora::NWScript::FunctionContext &ctx) {
	const MinigameGunBank *bank = swmg().getGunBank(getParamObject(ctx, 0), ctx.getParams()[1].getInt());
	ctx.getReturn() = bank ? bank->desc.sensingRadius : 0.0f;
}

void Functions::swmgGetGunBankInaccuracy(Aurora::NWScript::FunctionContext &ctx) {
	const MinigameGunBank *bank = swmg().getGunBank(getParamObject(ctx, 0), ctx.getParams()[1].getInt());
	ctx.getReturn() = bank ? bank->desc.inaccuracy : 0.0f;
}

void Functions::swmgIsGunBankTargetting(Aurora::NWScript::FunctionContext &ctx) {
	const MinigameGunBank *bank = swmg().getGunBank(getParamObject(ctx, 0), ctx.getParams()[1].getInt());
	ctx.getReturn() = (bank && bank->targeting) ? 1 : 0;
}

void Functions::swmgSetGunBankBulletModel(Aurora::NWScript::FunctionContext &ctx) {
	if (MinigameGunBank *bank = swmg().getGunBank(getParamObject(ctx, 0), ctx.getParams()[1].getInt()))
		bank->desc.bullet.model = ctx.getParams()[2].getString();
}

void Functions::swmgSetGunBankGunModel(Aurora::NWScript::FunctionContext &ctx) {
	if (MinigameGunBank *bank = swmg().getGunBank(getParamObject(ctx, 0), ctx.getParams()[1].getInt()))
		bank->desc.gunModel = ctx.getParams()[2].getString();
}

void Functions::swmgSetGunBankDamage(Aurora::NWScript::FunctionContext &ctx) {
	if (MinigameGunBank *bank = swmg().getGunBank(getParamObject(ctx, 0), ctx.getParams()[1].getInt()))
		bank->desc.bullet.damage = ctx.getParams()[2].getInt();
}

void Functions::swmgSetGunBankTimeBetweenShots(Aurora::NWScript::FunctionContext &ctx) {
	if (MinigameGunBank *bank = swmg().getGunBank(getParamObject(ctx, 0), ctx.getParams()[1].getInt()))
		bank->desc.bullet.rateOfFire = ctx.getParams()[2].getFloat();
}

void Functions::swmgSetGunBankLifespan(Aurora::NWScript::FunctionContext &ctx) {
	if (MinigameGunBank *bank = swmg().getGunBank(getParamObject(ctx, 0), ctx.getParams()[1].getInt()))
		bank->desc.bullet.lifespan = ctx.getParams()[2].getFloat();
}

void Functions::swmgSetGunBankSpeed(Aurora::NWScript::FunctionContext &ctx) {
	if (MinigameGunBank *bank = swmg().getGunBank(getParamObject(ctx, 0), ctx.getParams()[1].getInt()))
		bank->desc.bullet.speed = ctx.getParams()[2].getFloat();
}

void Functions::swmgSetGunBankTarget(Aurora::NWScript::FunctionContext &ctx) {
	if (MinigameGunBank *bank = swmg().getGunBank(getParamObject(ctx, 0), ctx.getParams()[1].getInt()))
		bank->desc.bullet.targetType = static_cast<uint32_t>(ctx.getParams()[2].getInt());
}

void Functions::swmgSetGunBankHorizontalSpread(Aurora::NWScript::FunctionContext &ctx) {
	if (MinigameGunBank *bank = swmg().getGunBank(getParamObject(ctx, 0), ctx.getParams()[1].getInt()))
		bank->desc.horizontalSpread = ctx.getParams()[2].getFloat();
}

void Functions::swmgSetGunBankVerticalSpread(Aurora::NWScript::FunctionContext &ctx) {
	if (MinigameGunBank *bank = swmg().getGunBank(getParamObject(ctx, 0), ctx.getParams()[1].getInt()))
		bank->desc.verticalSpread = ctx.getParams()[2].getFloat();
}

void Functions::swmgSetGunBankSensingRadius(Aurora::NWScript::FunctionContext &ctx) {
	if (MinigameGunBank *bank = swmg().getGunBank(getParamObject(ctx, 0), ctx.getParams()[1].getInt()))
		bank->desc.sensingRadius = ctx.getParams()[2].getFloat();
}

void Functions::swmgSetGunBankInaccuracy(Aurora::NWScript::FunctionContext &ctx) {
	if (MinigameGunBank *bank = swmg().getGunBank(getParamObject(ctx, 0), ctx.getParams()[1].getInt()))
		bank->desc.inaccuracy = ctx.getParams()[2].getFloat();
}

// The player

void Functions::swmgGetPlayerOffset(Aurora::NWScript::FunctionContext &ctx) {
	// Translation in a swoop race, rotation (degrees) in a turret.
	setVectorReturn(ctx, swmg().getPlayerOffset());
}

void Functions::swmgSetPlayerOffset(Aurora::NWScript::FunctionContext &ctx) {
	swmg().setPlayerOffset(getVectorParam(ctx, 0));
}

void Functions::swmgGetPlayerInvincibility(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = swmg().getPlayerInvincibility();
}

void Functions::swmgSetPlayerInvincibility(Aurora::NWScript::FunctionContext &ctx) {
	swmg().setPlayerInvincibility(ctx.getParams()[0].getFloat());
}

void Functions::swmgGetPlayerSpeed(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = swmg().getSpeed();
}

void Functions::swmgSetPlayerSpeed(Aurora::NWScript::FunctionContext &ctx) {
	swmg().setSpeed(ctx.getParams()[0].getFloat());
}

void Functions::swmgGetPlayerMinSpeed(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = swmg().getMinSpeed();
}

void Functions::swmgSetPlayerMinSpeed(Aurora::NWScript::FunctionContext &ctx) {
	swmg().setMinSpeed(ctx.getParams()[0].getFloat());
}

void Functions::swmgGetPlayerMaxSpeed(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = swmg().getMaxSpeed();
}

void Functions::swmgSetPlayerMaxSpeed(Aurora::NWScript::FunctionContext &ctx) {
	swmg().setMaxSpeed(ctx.getParams()[0].getFloat());
}

void Functions::swmgGetPlayerAccelerationPerSecond(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = swmg().getAccelerationPerSecond();
}

void Functions::swmgSetPlayerAccelerationPerSecond(Aurora::NWScript::FunctionContext &ctx) {
	swmg().setAccelerationPerSecond(ctx.getParams()[0].getFloat());
}

void Functions::swmgGetPlayerTunnelPos(Aurora::NWScript::FunctionContext &ctx) {
	setVectorReturn(ctx, swmg().getTunnelPos());
}

void Functions::swmgSetPlayerTunnelPos(Aurora::NWScript::FunctionContext &ctx) {
	swmg().setTunnelPos(getVectorParam(ctx, 0));
}

void Functions::swmgGetPlayerTunnelNeg(Aurora::NWScript::FunctionContext &ctx) {
	setVectorReturn(ctx, swmg().getTunnelNeg());
}

void Functions::swmgSetPlayerTunnelNeg(Aurora::NWScript::FunctionContext &ctx) {
	swmg().setTunnelNeg(getVectorParam(ctx, 0));
}

void Functions::swmgGetPlayerTunnelInfinite(Aurora::NWScript::FunctionContext &ctx) {
	setVectorReturn(ctx, swmg().getTunnelInfinite());
}

void Functions::swmgSetPlayerTunnelInfinite(Aurora::NWScript::FunctionContext &ctx) {
	swmg().setTunnelInfinite(getVectorParam(ctx, 0));
}

void Functions::swmgGetPlayerOrigin(Aurora::NWScript::FunctionContext &ctx) {
	setVectorReturn(ctx, swmg().getPlayerOrigin());
}

void Functions::swmgSetPlayerOrigin(Aurora::NWScript::FunctionContext &ctx) {
	swmg().setPlayerOrigin(getVectorParam(ctx, 0));
}

void Functions::swmgGetIsInvulnerable(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = swmg().isInvulnerable(getParamObject(ctx, 0)) ? 1 : 0;
}

void Functions::swmgStartInvulnerability(Aurora::NWScript::FunctionContext &ctx) {
	swmg().startInvulnerability(getParamObject(ctx, 0));
}

// Track follower sounds: (object oFollower, int nSound[, int value])

void Functions::swmgGetSoundFrequency(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = swmg().getSound(getParamObject(ctx, 0), ctx.getParams()[1].getInt()).frequency;
}

void Functions::swmgSetSoundFrequency(Aurora::NWScript::FunctionContext &ctx) {
	swmg().getSound(getParamObject(ctx, 0), ctx.getParams()[1].getInt()).frequency = ctx.getParams()[2].getInt();
}

void Functions::swmgGetSoundFrequencyIsRandom(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = swmg().getSound(getParamObject(ctx, 0), ctx.getParams()[1].getInt()).random ? 1 : 0;
}

void Functions::swmgSetSoundFrequencyIsRandom(Aurora::NWScript::FunctionContext &ctx) {
	swmg().getSound(getParamObject(ctx, 0), ctx.getParams()[1].getInt()).random = ctx.getParams()[2].getInt() != 0;
}

void Functions::swmgGetSoundVolume(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = swmg().getSound(getParamObject(ctx, 0), ctx.getParams()[1].getInt()).volume;
}

void Functions::swmgSetSoundVolume(Aurora::NWScript::FunctionContext &ctx) {
	swmg().getSound(getParamObject(ctx, 0), ctx.getParams()[1].getInt()).volume = ctx.getParams()[2].getInt();
}

// KotOR II additions

void Functions::swmgGetSwoopUpgrade(Aurora::NWScript::FunctionContext &ctx) {
	// SWMG_GetSwoopUpgrade(int nSlot): the swoop upgrade screen isn't implemented.
	ctx.getReturn() = _game->getModule().getGlobalNumber(
		Common::String::format("__swmg_swoop_upgrade_%d", ctx.getParams()[0].getInt()));
}

void Functions::swmgGetTrackPosition(Aurora::NWScript::FunctionContext &ctx) {
	setVectorReturn(ctx, swmg().getPosition(getParamObject(ctx, 0)));
}

void Functions::swmgSetFollowerPosition(Aurora::NWScript::FunctionContext &UNUSED(ctx)) {
	// Track followers are placed by their track; nothing to move.
}

void Functions::swmgDestroyMiniGameObject(Aurora::NWScript::FunctionContext &ctx) {
	if (MinigameObject *object = swmg().toMinigameObject(getParamObject(ctx, 0)))
		swmg().defaultDeath(*object);
}

void Functions::swmgSetJumpSpeed(Aurora::NWScript::FunctionContext &UNUSED(ctx)) {
	// Jumping isn't simulated.
}

void Functions::swmgPlayerApplyForce(Aurora::NWScript::FunctionContext &ctx) {
	swmg().setPlayerOffset(swmg().getPlayerOffset() + getVectorParam(ctx, 0));
}

} // End of namespace KotORBase

} // End of namespace Engines
