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
#include "src/common/ustring.h"
#include "src/common/string.h"

#include "src/aurora/nwscript/functioncontext.h"

#include "src/engines/kotorbase/module.h"
#include "src/engines/kotorbase/game.h"
#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/object.h"
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


static Common::UString swmgObjectName(Aurora::NWScript::Object *obj) {
	KotORBase::Object *o = ObjectContainer::toObject(obj);
	if (!o)
		return Common::UString();
	if (!o->getTag().empty())
		return o->getTag();
	return o->getName();
}

static int swmgObjectKindMatches(Aurora::NWScript::Object *obj, int kind) {
	const Common::UString name = swmgObjectName(obj);
	if (name.empty())
		return 0;
	return SwoopMinigame::get().getObjectKind(name) == kind ? 1 : 0;
}

void Functions::swmgGetLastEventModelName(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = SwoopMinigame::get().getLastEventModelName();
}

void Functions::swmgGetObjectByName(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(nullptr);
}

void Functions::swmgPlayAnimation(Aurora::NWScript::FunctionContext &ctx) {
	Aurora::NWScript::Object *obj = ctx.getParams()[0].getObject();
	Common::UString name = swmgObjectName(obj);
	if (!name.empty())
		SwoopMinigame::get().setObjectName(name);
}

void Functions::swmgGetLastBulletHitDamage(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = static_cast<int32_t>(SwoopMinigame::get().getLastBulletHitDamage());
}

void Functions::swmgGetLastBulletHitTarget(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = _game->getModule().getGlobalNumber("__swmg_bullet_hit_target");
}

void Functions::swmgGetLastBulletHitShooter(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(nullptr);
}

void Functions::swmgAdjustFollowerHitPoints(Aurora::NWScript::FunctionContext &ctx) {
	int amount = ctx.getParams()[1].getInt();
	SwoopMinigame::get().adjustFollowerHitPoints(static_cast<float>(amount));
	ctx.getReturn() = static_cast<int32_t>(SwoopMinigame::get().getFollowerHitPoints());
}

void Functions::swmgOnBulletHit(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	float dmg = static_cast<float>(_game->getModule().getGlobalNumber("__swmg_bullet_damage"));
	if (dmg <= 0.0f)
		dmg = 10.0f;
	SwoopMinigame::get().onBulletHit(dmg);
	_game->getModule().setGlobalNumber("__swmg_last_event", SwoopMinigame::get().getLastEvent());
}

void Functions::swmgGetLastFollowerHit(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(nullptr);
}

void Functions::swmgGetLastObstacleHit(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(nullptr);
}

void Functions::swmgGetLastBulletFiredDamage(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = static_cast<int32_t>(SwoopMinigame::get().getLastBulletFiredDamage());
}

void Functions::swmgGetLastBulletFiredTarget(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = _game->getModule().getGlobalNumber("__swmg_bullet_fired_target");
}

void Functions::swmgGetObjectName(Aurora::NWScript::FunctionContext &ctx) {
	Aurora::NWScript::Object *obj = ctx.getParams()[0].getObject();
	Common::UString name = swmgObjectName(obj);
	if (name.empty())
		name = SwoopMinigame::get().getObjectName();
	ctx.getReturn() = name;
}

void Functions::swmgOnDeath(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	SwoopMinigame::get().onDeath();
	_game->getModule().setGlobalNumber("__swmg_last_event", SwoopMinigame::get().getLastEvent());
	_game->getModule().signalEncounter("swmg_death");
}

void Functions::swmgIsFollower(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = swmgObjectKindMatches(ctx.getParams()[0].getObject(), 1);
}

void Functions::swmgIsPlayer(Aurora::NWScript::FunctionContext &ctx) {
	Aurora::NWScript::Object *obj = ctx.getParams()[0].getObject();
	if (!obj) {
		ctx.getReturn() = 1;
		return;
	}
	int kind = SwoopMinigame::get().getObjectKind(swmgObjectName(obj));
	ctx.getReturn() = (kind == 0 || kind < 0) ? 1 : 0;
}

void Functions::swmgIsEnemy(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = swmgObjectKindMatches(ctx.getParams()[0].getObject(), 2);
}

void Functions::swmgIsTrigger(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = swmgObjectKindMatches(ctx.getParams()[0].getObject(), 3);
}

void Functions::swmgIsObstacle(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = swmgObjectKindMatches(ctx.getParams()[0].getObject(), 4);
}

void Functions::swmgSetFollowerHitPoints(Aurora::NWScript::FunctionContext &ctx) {
	SwoopMinigame::get().setFollowerHitPoints(static_cast<float>(ctx.getParams()[1].getInt()));
}

void Functions::swmgOnDamage(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	float amount = static_cast<float>(_game->getModule().getGlobalNumber("__swmg_damage"));
	if (amount <= 0.0f)
		amount = 1.0f;
	SwoopMinigame::get().onDamage(amount);
	_game->getModule().setGlobalNumber("__swmg_last_event", SwoopMinigame::get().getLastEvent());
}

void Functions::swmgGetLastHPChange(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = static_cast<int32_t>(SwoopMinigame::get().getLastHPChange());
}

void Functions::swmgRemoveAnimation(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
}

void Functions::swmgGetCameraNearClip(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = SwoopMinigame::get().getCameraNearClip();
}

void Functions::swmgGetCameraFarClip(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = SwoopMinigame::get().getCameraFarClip();
}

void Functions::swmgSetCameraClip(Aurora::NWScript::FunctionContext &ctx) {
	SwoopMinigame::get().setCameraClip(ctx.getParams()[0].getFloat(), ctx.getParams()[1].getFloat());
}

void Functions::swmgGetPlayer(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(_game->getModule().getPC());
}

void Functions::swmgGetObstacleCount(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = SwoopMinigame::get().getObstacleCount();
}

void Functions::swmgGetObstacle(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(nullptr);
}

void Functions::swmgGetHitPoints(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = static_cast<int32_t>(SwoopMinigame::get().getHitPoints());
}

void Functions::swmgGetMaxHitPoints(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = static_cast<int32_t>(SwoopMinigame::get().getMaxHitPoints());
}

void Functions::swmgSetMaxHitPoints(Aurora::NWScript::FunctionContext &ctx) {
	SwoopMinigame::get().setMaxHitPoints(static_cast<float>(ctx.getParams()[1].getInt()));
}

void Functions::swmgGetSphereRadius(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = SwoopMinigame::get().getSphereRadius();
}

void Functions::swmgSetSphereRadius(Aurora::NWScript::FunctionContext &ctx) {
	SwoopMinigame::get().setSphereRadius(ctx.getParams()[1].getFloat());
}

void Functions::swmgGetNumLoops(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = SwoopMinigame::get().getNumLoops();
}

void Functions::swmgSetNumLoops(Aurora::NWScript::FunctionContext &ctx) {
	SwoopMinigame::get().setNumLoops(ctx.getParams()[1].getInt());
}

void Functions::swmgGetPosition(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	float x, y, z;
	SwoopMinigame::get().getPlayerOrigin(x, y, z);
	ctx.getReturn().setVector(x + SwoopMinigame::get().getPlayerOffset(), y, z);
}

void Functions::swmgGetGunBankCount(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = SwoopMinigame::get().getGunBankCount();
}

void Functions::swmgGetGunBankBulletModel(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = SwoopMinigame::get().getGunBank(ctx.getParams()[1].getInt()).bulletModel;
}

void Functions::swmgGetGunBankGunModel(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = SwoopMinigame::get().getGunBank(ctx.getParams()[1].getInt()).gunModel;
}

void Functions::swmgGetGunBankDamage(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = static_cast<int32_t>(SwoopMinigame::get().getGunBank(ctx.getParams()[1].getInt()).damage);
}

void Functions::swmgGetGunBankTimeBetweenShots(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = SwoopMinigame::get().getGunBank(ctx.getParams()[1].getInt()).timeBetweenShots;
}

void Functions::swmgGetGunBankLifespan(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = SwoopMinigame::get().getGunBank(ctx.getParams()[1].getInt()).lifespan;
}

void Functions::swmgGetGunBankSpeed(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = SwoopMinigame::get().getGunBank(ctx.getParams()[1].getInt()).speed;
}

void Functions::swmgGetGunBankTarget(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = 0;
}

void Functions::swmgSetGunBankBulletModel(Aurora::NWScript::FunctionContext &ctx) {
	SwoopMinigame::get().getGunBank(ctx.getParams()[1].getInt()).bulletModel = ctx.getParams()[2].getString();
}

void Functions::swmgSetGunBankGunModel(Aurora::NWScript::FunctionContext &ctx) {
	SwoopMinigame::get().getGunBank(ctx.getParams()[1].getInt()).gunModel = ctx.getParams()[2].getString();
}

void Functions::swmgSetGunBankDamage(Aurora::NWScript::FunctionContext &ctx) {
	SwoopMinigame::get().getGunBank(ctx.getParams()[1].getInt()).damage = static_cast<float>(ctx.getParams()[2].getInt());
}

void Functions::swmgSetGunBankTimeBetweenShots(Aurora::NWScript::FunctionContext &ctx) {
	SwoopMinigame::get().getGunBank(ctx.getParams()[1].getInt()).timeBetweenShots = ctx.getParams()[2].getFloat();
}

void Functions::swmgSetGunBankLifespan(Aurora::NWScript::FunctionContext &ctx) {
	SwoopMinigame::get().getGunBank(ctx.getParams()[1].getInt()).lifespan = ctx.getParams()[2].getFloat();
}

void Functions::swmgSetGunBankSpeed(Aurora::NWScript::FunctionContext &ctx) {
	SwoopMinigame::get().getGunBank(ctx.getParams()[1].getInt()).speed = ctx.getParams()[2].getFloat();
}

void Functions::swmgSetGunBankTarget(Aurora::NWScript::FunctionContext &ctx) {
	SwoopMinigame::get().getGunBank(ctx.getParams()[1].getInt()).target =
		Common::String::format("%d", ctx.getParams()[2].getInt());
}

void Functions::swmgGetPlayerInvincibility(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = SwoopMinigame::get().isInvincible() ? 1.0f : 0.0f;
}

void Functions::swmgGetPlayerMinSpeed(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = SwoopMinigame::get().getMinSpeed();
}

void Functions::swmgGetPlayerAccelerationPerSecond(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = SwoopMinigame::get().getAccelerationPerSecond();
}

void Functions::swmgSetPlayerInvincibility(Aurora::NWScript::FunctionContext &ctx) {
	SwoopMinigame::get().setInvincible(ctx.getParams()[0].getFloat() != 0.0f);
}

void Functions::swmgSetPlayerMinSpeed(Aurora::NWScript::FunctionContext &ctx) {
	SwoopMinigame::get().setMinSpeed(ctx.getParams()[0].getFloat());
}

void Functions::swmgSetPlayerAccelerationPerSecond(Aurora::NWScript::FunctionContext &ctx) {
	SwoopMinigame::get().setAccelerationPerSecond(ctx.getParams()[0].getFloat());
}

void Functions::swmgGetPlayerOrigin(Aurora::NWScript::FunctionContext &ctx) {
	float x, y, z;
	SwoopMinigame::get().getPlayerOrigin(x, y, z);
	ctx.getReturn().setVector(x, y, z);
}

void Functions::swmgSetPlayerOrigin(Aurora::NWScript::FunctionContext &ctx) {
	float x = 0.0f, y = 0.0f, z = 0.0f;
	ctx.getParams()[0].getVector(x, y, z);
	SwoopMinigame::get().setPlayerOrigin(x, y, z);
}

void Functions::swmgGetGunBankHorizontalSpread(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = SwoopMinigame::get().getGunBank(ctx.getParams()[1].getInt()).horizontalSpread;
}

void Functions::swmgGetGunBankVerticalSpread(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = SwoopMinigame::get().getGunBank(ctx.getParams()[1].getInt()).verticalSpread;
}

void Functions::swmgGetGunBankSensingRadius(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = SwoopMinigame::get().getGunBank(ctx.getParams()[1].getInt()).sensingRadius;
}

void Functions::swmgGetGunBankInaccuracy(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = SwoopMinigame::get().getGunBank(ctx.getParams()[1].getInt()).inaccuracy;
}

void Functions::swmgSetGunBankHorizontalSpread(Aurora::NWScript::FunctionContext &ctx) {
	SwoopMinigame::get().getGunBank(ctx.getParams()[1].getInt()).horizontalSpread = ctx.getParams()[2].getFloat();
}

void Functions::swmgSetGunBankVerticalSpread(Aurora::NWScript::FunctionContext &ctx) {
	SwoopMinigame::get().getGunBank(ctx.getParams()[1].getInt()).verticalSpread = ctx.getParams()[2].getFloat();
}

void Functions::swmgSetGunBankSensingRadius(Aurora::NWScript::FunctionContext &ctx) {
	SwoopMinigame::get().getGunBank(ctx.getParams()[1].getInt()).sensingRadius = ctx.getParams()[2].getFloat();
}

void Functions::swmgSetGunBankInaccuracy(Aurora::NWScript::FunctionContext &ctx) {
	SwoopMinigame::get().getGunBank(ctx.getParams()[1].getInt()).inaccuracy = ctx.getParams()[2].getFloat();
}

void Functions::swmgGetIsInvulnerable(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = SwoopMinigame::get().isInvulnerable() ? 1 : 0;
}

void Functions::swmgStartInvulnerability(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	SwoopMinigame::get().startInvulnerability(3.0f);
}

void Functions::swmgGetSoundFrequency(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = static_cast<int32_t>(SwoopMinigame::get().getSoundFrequency());
}

void Functions::swmgSetSoundFrequency(Aurora::NWScript::FunctionContext &ctx) {
	SwoopMinigame::get().setSoundFrequency(static_cast<float>(ctx.getParams()[2].getInt()));
}

void Functions::swmgGetSoundFrequencyIsRandom(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = SwoopMinigame::get().getSoundFrequencyIsRandom() ? 1 : 0;
}

void Functions::swmgSetSoundFrequencyIsRandom(Aurora::NWScript::FunctionContext &ctx) {
	SwoopMinigame::get().setSoundFrequencyIsRandom(ctx.getParams()[2].getInt() != 0);
}

void Functions::swmgGetSoundVolume(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = static_cast<int32_t>(SwoopMinigame::get().getSoundVolume() * 127.0f);
}

void Functions::swmgSetSoundVolume(Aurora::NWScript::FunctionContext &ctx) {
	SwoopMinigame::get().setSoundVolume(static_cast<float>(ctx.getParams()[2].getInt()) / 127.0f);
}

void Functions::swmgGetPlayerTunnelInfinite(Aurora::NWScript::FunctionContext &ctx) {
	const float v = SwoopMinigame::get().getTunnelInfinite() ? 1.0f : 0.0f;
	ctx.getReturn().setVector(v, 0.0f, 0.0f);
}

void Functions::swmgSetPlayerTunnelInfinite(Aurora::NWScript::FunctionContext &ctx) {
	float x = 0.0f, y = 0.0f, z = 0.0f;
	ctx.getParams()[0].getVector(x, y, z);
	SwoopMinigame::get().setTunnelInfinite(x != 0.0f || y != 0.0f || z != 0.0f);
}

void Functions::swmgGetSwoopUpgrade(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = _game->getModule().getGlobalNumber("__swmg_swoop_upgrade");
}

void Functions::swmgGetTrackPosition(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	float x, y, z;
	SwoopMinigame::get().getPlayerOrigin(x, y, z);
	ctx.getReturn().setVector(x, y, z);
}

void Functions::swmgSetFollowerPosition(Aurora::NWScript::FunctionContext &ctx) {
	float x = 0.0f, y = 0.0f, z = 0.0f;
	if (!ctx.getParams().empty())
		ctx.getParams()[0].getVector(x, y, z);
	_game->getModule().setGlobalNumber("__swmg_follower_x", static_cast<int>(x * 1000.0f));
	_game->getModule().setGlobalNumber("__swmg_follower_y", static_cast<int>(y * 1000.0f));
	_game->getModule().setGlobalNumber("__swmg_follower_z", static_cast<int>(z * 1000.0f));
}

void Functions::swmgDestroyMiniGameObject(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
}

void Functions::swmgSetJumpSpeed(Aurora::NWScript::FunctionContext &ctx) {
	_game->getModule().setGlobalNumber("__swmg_jump_speed",
		static_cast<int>(ctx.getParams()[0].getFloat() * 1000.0f));
}

void Functions::swmgPlayerApplyForce(Aurora::NWScript::FunctionContext &ctx) {
	float x = 0.0f, y = 0.0f, z = 0.0f;
	if (!ctx.getParams().empty())
		ctx.getParams()[0].getVector(x, y, z);
	SwoopMinigame::get().setPlayerOffset(SwoopMinigame::get().getPlayerOffset() + x);
	(void)y;
	(void)z;
}

} // End of namespace KotORBase

} // End of namespace Engines
