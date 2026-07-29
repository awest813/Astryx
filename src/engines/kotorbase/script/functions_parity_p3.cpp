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
 *  Phase 3 NWScript natives previously returned by stubFunction.
 */

#include "src/common/util.h"
#include "src/common/random.h"
#include "src/common/ustring.h"
#include "src/common/string.h"

#include "src/events/events.h"

#include "src/aurora/nwscript/functioncontext.h"

#include "src/engines/kotorbase/area.h"
#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/door.h"
#include "src/engines/kotorbase/effect.h"
#include "src/engines/kotorbase/game.h"
#include "src/engines/kotorbase/item.h"
#include "src/engines/kotorbase/location.h"
#include "src/engines/kotorbase/module.h"
#include "src/engines/kotorbase/object.h"
#include "src/engines/kotorbase/objectcontainer.h"
#include "src/engines/kotorbase/placeable.h"
#include "src/engines/kotorbase/sound.h"
#include "src/engines/kotorbase/types.h"
#include "src/engines/kotorbase/action.h"

#include "src/engines/kotorbase/script/event.h"
#include "src/engines/kotorbase/script/functions.h"

namespace Engines {

namespace KotORBase {

namespace {

Aurora::NWScript::EngineType *passThroughEffect(Aurora::NWScript::FunctionContext &ctx) {
	const Effect *effect = 0;
	if (!ctx.getParams().empty())
		effect = dynamic_cast<const Effect *>(ctx.getParams()[0].getEngineType());
	return effect ? effect->clone() : new Effect(kKotOREffectVisual, 0);
}

int currentHour() {
	return static_cast<int32_t>(((EventMan.getTimestamp() / 1000) / 60) / 60) % 24;
}

} // End of anonymous namespace

void Functions::soundObjectSetFixedVariance(Aurora::NWScript::FunctionContext &ctx) {
	Object *sound = ObjectContainer::toObject(ctx.getParams()[0].getObject());
	if (!sound)
		return;
	_game->getModule().setGlobalNumber(
		Common::String::format("__snd_var_%s", sound->getTag().c_str()),
		static_cast<int>(ctx.getParams()[1].getFloat() * 1000.0f));
}

void Functions::soundObjectGetFixedVariance(Aurora::NWScript::FunctionContext &ctx) {
	Object *sound = ObjectContainer::toObject(ctx.getParams()[0].getObject());
	if (!sound) {
		ctx.getReturn() = 0.0f;
		return;
	}
	ctx.getReturn() = _game->getModule().getGlobalNumber(
		Common::String::format("__snd_var_%s", sound->getTag().c_str())) / 1000.0f;
}

void Functions::soundObjectSetVolume(Aurora::NWScript::FunctionContext &ctx) {
	Object *sound = ObjectContainer::toObject(ctx.getParams()[0].getObject());
	if (!sound)
		return;
	_game->getModule().setGlobalNumber(
		Common::String::format("__snd_vol_%s", sound->getTag().c_str()),
		ctx.getParams()[1].getInt());
}

void Functions::soundObjectSetPosition(Aurora::NWScript::FunctionContext &ctx) {
	SoundObject *sound = ObjectContainer::toSoundObject(ctx.getParams()[0].getObject());
	if (!sound)
		return;
	float x = 0.0f, y = 0.0f, z = 0.0f;
	ctx.getParams()[1].getVector(x, y, z);
	sound->setPosition(x, y, z);
}

void Functions::soundObjectGetPitchVariance(Aurora::NWScript::FunctionContext &ctx) {
	Object *sound = ObjectContainer::toObject(ctx.getParams()[0].getObject());
	if (!sound) {
		ctx.getReturn() = 0.0f;
		return;
	}
	ctx.getReturn() = _game->getModule().getGlobalNumber(
		Common::String::format("__snd_pitch_%s", sound->getTag().c_str())) / 1000.0f;
}

void Functions::soundObjectSetPitchVariance(Aurora::NWScript::FunctionContext &ctx) {
	Object *sound = ObjectContainer::toObject(ctx.getParams()[0].getObject());
	if (!sound)
		return;
	_game->getModule().setGlobalNumber(
		Common::String::format("__snd_pitch_%s", sound->getTag().c_str()),
		static_cast<int>(ctx.getParams()[1].getFloat() * 1000.0f));
}

void Functions::soundObjectGetVolume(Aurora::NWScript::FunctionContext &ctx) {
	Object *sound = ObjectContainer::toObject(ctx.getParams()[0].getObject());
	if (!sound) {
		ctx.getReturn() = 127;
		return;
	}
	int vol = _game->getModule().getGlobalNumber(
		Common::String::format("__snd_vol_%s", sound->getTag().c_str()));
	ctx.getReturn() = (vol > 0) ? vol : 127;
}

void Functions::actionForceFollowObject(Aurora::NWScript::FunctionContext &ctx) {
	Creature *caller = ObjectContainer::toCreature(ctx.getCaller());
	Object *target = ObjectContainer::toObject(ctx.getParams()[0].getObject());
	if (!caller || !target)
		return;
	Action action(kActionFollow, target);
	action.range = ctx.getParams()[1].getFloat();
	caller->addAction(action);
}

void Functions::randomName(Aurora::NWScript::FunctionContext &ctx) {
	static const char *const kNames[] = {
		"Jace", "Mira", "Doran", "Sera", "Kael", "Rynn", "Tala", "Vex"
	};
	ctx.getReturn() = Common::UString(kNames[static_cast<size_t>(RNG.getNext(0, 8))]);
}

void Functions::setItemNonEquippable(Aurora::NWScript::FunctionContext &ctx) {
	Item *item = ObjectContainer::toItem(ctx.getParams()[0].getObject());
	if (!item)
		return;
	_game->getModule().setGlobalBoolean(
		Common::String::format("__item_nonequip_%s", item->getTag().c_str()), true);
}

void Functions::getButtonMashCheck(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = _game->getModule().getGlobalBoolean("__button_mash") ? 1 : 0;
}

void Functions::setButtonMashCheck(Aurora::NWScript::FunctionContext &ctx) {
	_game->getModule().setGlobalBoolean("__button_mash", ctx.getParams()[0].getInt() != 0);
}

void Functions::getLastPlayerDied(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(_game->getModule().getPC());
}

void Functions::getLastPlayerDying(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(_game->getModule().getPC());
}

void Functions::getLastRespawnButtonPresser(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(_game->getModule().getPC());
}

void Functions::getModuleItemLost(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(nullptr);
}

void Functions::getModuleItemLostBy(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(nullptr);
}

void Functions::eventConversation(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = new Event(kEventUserDefined);
}

void Functions::eventActivateItem(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = new Event(kEventUserDefined);
}

void Functions::setEncounterDifficulty(Aurora::NWScript::FunctionContext &ctx) {
	_game->getModule().setGlobalNumber("__encounter_difficulty", ctx.getParams()[0].getInt());
}

void Functions::getEncounterDifficulty(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = _game->getModule().getGlobalNumber("__encounter_difficulty");
}

void Functions::actionExamine(Aurora::NWScript::FunctionContext &ctx) {
	Object *target = ObjectContainer::toObject(ctx.getParams()[0].getObject());
	Creature *caller = ObjectContainer::toCreature(ctx.getCaller());
	if (!caller || !target)
		return;
	caller->addAction(Action(kActionUseObject, target));
}

void Functions::getLastCommand(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = _game->getModule().getGlobalNumber("__last_command");
}

void Functions::getLastAssociateCommand(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = _game->getModule().getGlobalNumber("__last_associate_command");
}

void Functions::getLastDisarmed(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(nullptr);
}

void Functions::getLastDisturbed(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(nullptr);
}

void Functions::getLastLocked(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(nullptr);
}

void Functions::getLastUnlocked(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(nullptr);
}

void Functions::versusAlignmentEffect(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = passThroughEffect(ctx);
}

void Functions::versusRacialTypeEffect(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = passThroughEffect(ctx);
}

void Functions::versusTrapEffect(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = passThroughEffect(ctx);
}

void Functions::addMultiClass(Aurora::NWScript::FunctionContext &ctx) {
	Creature *creature = ObjectContainer::toCreature(ctx.getParams()[0].getObject());
	(void)creature;
	(void)ctx;
	// Level-up multi-classing is handled by chargen/level-up UI; accept as no-op.
}

void Functions::actionEquipMostEffectiveArmor(Aurora::NWScript::FunctionContext &ctx) {
	Creature *caller = ObjectContainer::toCreature(ctx.getCaller());
	if (caller)
		caller->equipMostDamagingMelee(); // best available stand-in until armor scoring lands
	(void)ctx;
}

void Functions::getIsDawn(Aurora::NWScript::FunctionContext &ctx) {
	int hour = currentHour();
	ctx.getReturn() = (hour >= 5 && hour < 7) ? 1 : 0;
}

void Functions::getIsDusk(Aurora::NWScript::FunctionContext &ctx) {
	int hour = currentHour();
	ctx.getReturn() = (hour >= 17 && hour < 19) ? 1 : 0;
}

void Functions::getIsEncounterCreature(Aurora::NWScript::FunctionContext &ctx) {
	Creature *creature = ObjectContainer::toCreature(ctx.getParams()[0].getObject());
	ctx.getReturn() = (creature && creature->isEnemy()) ? 1 : 0;
}

void Functions::getStartingLocation(Aurora::NWScript::FunctionContext &ctx) {
	Location *loc = new Location();
	Area *area = _game->getModule().getCurrentArea();
	if (area)
		loc->setArea(area);
	Creature *pc = _game->getModule().getPC();
	if (pc)
		*loc = pc->getLocation();
	ctx.getReturn() = loc;
}

void Functions::getIsWeaponEffective(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = 1;
}

void Functions::musicBackgroundSetDelay(Aurora::NWScript::FunctionContext &ctx) {
	_game->getModule().setGlobalNumber("__music_delay", ctx.getParams()[1].getInt());
}

void Functions::getItemActivatedTargetLocation(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = new Location();
}

void Functions::getItemActivatedTarget(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(nullptr);
}

void Functions::effectDisguise(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = new Effect(kKotOREffectVisual, ctx.getParams()[0].getInt());
}

void Functions::effectTrueSeeing(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = new Effect(kKotOREffectSeeInvisible);
}

void Functions::effectTimeStop(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = new Effect(kKotOREffectParalyze);
}

void Functions::effectHorrified(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = new Effect(kKotOREffectStunned);
}

void Functions::effectSpellLevelAbsorption(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = new Effect(kKotOREffectSpellImmunity, ctx.getParams()[0].getInt());
}

void Functions::effectDispelMagicBest(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = new Effect(kKotOREffectDispelMagicAll, ctx.getParams()[0].getInt());
}

void Functions::effectMissChance(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = new Effect(kKotOREffectConcealment, ctx.getParams()[0].getInt());
}

void Functions::effectDamageShield(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = new Effect(kKotOREffectForceShield, ctx.getParams()[0].getInt());
}

void Functions::getMaxStealthXP(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = _game->getModule().getGlobalNumber("__stealth_xp_max");
}

void Functions::setMaxStealthXP(Aurora::NWScript::FunctionContext &ctx) {
	_game->getModule().setGlobalNumber("__stealth_xp_max", ctx.getParams()[0].getInt());
}

void Functions::getCurrentStealthXP(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = _game->getModule().getGlobalNumber("__stealth_xp_pool");
}

void Functions::setCurrentStealthXP(Aurora::NWScript::FunctionContext &ctx) {
	_game->getModule().setGlobalNumber("__stealth_xp_pool", ctx.getParams()[0].getInt());
}

void Functions::getNumStackedItems(Aurora::NWScript::FunctionContext &ctx) {
	Item *item = ObjectContainer::toItem(ctx.getParams()[0].getObject());
	ctx.getReturn() = item ? item->getStackSize() : 0;
}

void Functions::getCreatureSize(Aurora::NWScript::FunctionContext &ctx) {
	Creature *creature = ObjectContainer::toCreature(ctx.getParams()[0].getObject());
	ctx.getReturn() = creature ? 3 : 0; // CREATURE_SIZE_MEDIUM
}

void Functions::getLastTrapDetected(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(nullptr);
}

void Functions::getNearestTrapToObject(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(nullptr);
}

void Functions::getAttemptedMovementTarget(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(nullptr);
}

void Functions::getBlockingCreature(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(nullptr);
}

void Functions::getChallengeRating(Aurora::NWScript::FunctionContext &ctx) {
	Creature *creature = ObjectContainer::toCreature(ctx.getParams()[0].getObject());
	ctx.getReturn() = creature ? static_cast<float>(creature->getHitDice()) : 0.0f;
}

void Functions::getFoundEnemyCreature(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(nullptr);
}

void Functions::getMovementRate(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = 0;
}

void Functions::duplicateHeadAppearance(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
}

void Functions::getIsPlaceableObjectActionPossible(Aurora::NWScript::FunctionContext &ctx) {
	Placeable *placeable = ObjectContainer::toPlaceable(ctx.getParams()[0].getObject());
	ctx.getReturn() = placeable ? 1 : 0;
}

void Functions::doPlaceableObjectAction(Aurora::NWScript::FunctionContext &ctx) {
	Placeable *placeable = ObjectContainer::toPlaceable(ctx.getParams()[0].getObject());
	Creature *caller = ObjectContainer::toCreature(ctx.getCaller());
	if (caller && placeable)
		caller->addAction(Action(kActionUseObject, placeable));
}

void Functions::exportAllCharacters(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
}

void Functions::getCreatureMovmentType(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = 0;
}

void Functions::musicBackgroundGetBattleTrack(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = _game->getModule().getGlobalNumber("__music_battle_track");
}

void Functions::getHasInventory(Aurora::NWScript::FunctionContext &ctx) {
	Placeable *placeable = ObjectContainer::toPlaceable(ctx.getParams()[0].getObject());
	if (placeable) {
		ctx.getReturn() = placeable->hasInventory() ? 1 : 0;
		return;
	}
	Creature *creature = ObjectContainer::toCreature(ctx.getParams()[0].getObject());
	ctx.getReturn() = creature ? 1 : 0;
}

void Functions::getStrRefSoundDuration(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = 0.0f;
}

void Functions::setFormation(Aurora::NWScript::FunctionContext &ctx) {
	_game->getModule().setGlobalNumber("__formation_id", ctx.getParams()[2].getInt());
	_game->getModule().setGlobalNumber("__formation_line", ctx.getParams()[3].getInt());
}

void Functions::setForcePowerUnsuccessful(Aurora::NWScript::FunctionContext &ctx) {
	_game->getModule().setGlobalNumber("__force_unsuccessful", ctx.getParams()[0].getInt());
}

void Functions::setAreaFogColor(Aurora::NWScript::FunctionContext &ctx) {
	_game->getModule().setGlobalNumber("__fog_r", static_cast<int>(ctx.getParams()[1].getFloat() * 255.0f));
	_game->getModule().setGlobalNumber("__fog_g", static_cast<int>(ctx.getParams()[2].getFloat() * 255.0f));
	_game->getModule().setGlobalNumber("__fog_b", static_cast<int>(ctx.getParams()[3].getFloat() * 255.0f));
}

void Functions::changeItemCost(Aurora::NWScript::FunctionContext &ctx) {
	_game->getModule().setGlobalNumber(
		Common::String::format("__item_cost_%s", ctx.getParams()[0].getString().c_str()),
		static_cast<int>(ctx.getParams()[1].getFloat()));
}

void Functions::getIsLiveContentAvailable(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = 0;
}

void Functions::getIsPoisoned(Aurora::NWScript::FunctionContext &ctx) {
	Creature *creature = ObjectContainer::toCreature(ctx.getParams()[0].getObject());
	(void)creature;
	ctx.getReturn() = 0;
}

void Functions::cancelPostDialogCharacterSwitch(Aurora::NWScript::FunctionContext &ctx) {
	_game->getModule().setGlobalBoolean("__cancel_dialog_switch", true);
	(void)ctx;
}

void Functions::suppressStatusSummaryEntry(Aurora::NWScript::FunctionContext &ctx) {
	_game->getModule().setGlobalNumber("__suppress_status", ctx.getParams()[0].getInt());
}

void Functions::getCheatCode(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = 0;
}

void Functions::setMusicVolume(Aurora::NWScript::FunctionContext &ctx) {
	_game->getModule().setGlobalNumber("__music_volume",
		static_cast<int>(ctx.getParams()[0].getFloat() * 100.0f));
}

void Functions::createItemOnFloor(Aurora::NWScript::FunctionContext &ctx) {
	const Common::UString &resRef = ctx.getParams()[0].getString();
	Location *loc = dynamic_cast<Location *>(ctx.getParams()[1].getEngineType());
	int stackSize = ctx.getParams()[2].getInt();
	if (stackSize <= 0)
		stackSize = 1;

	Creature *pc = _game->getModule().getPC();
	if (!pc) {
		ctx.getReturn() = static_cast<Aurora::NWScript::Object *>(nullptr);
		return;
	}

	pc->getInventory().addItem(resRef, stackSize);
	Item *item = pc->addScriptItem(resRef);
	(void)loc;
	ctx.getReturn() = item ? static_cast<Aurora::NWScript::Object *>(item)
	                       : static_cast<Aurora::NWScript::Object *>(nullptr);
}

void Functions::setAvailableNPCId(Aurora::NWScript::FunctionContext &ctx) {
	_game->getModule().setGlobalNumber("__available_npc_id", ctx.getParams()[0].getInt());
}

void Functions::yavinHackCloseDoor(Aurora::NWScript::FunctionContext &ctx) {
	Door *door = ObjectContainer::toDoor(ctx.getParams()[0].getObject());
	if (door)
		door->close(_game->getModule().getPC());
}

} // End of namespace KotORBase

} // End of namespace Engines
