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
#include "src/common/configman.h"
#include "src/common/debug.h"

#include "src/aurora/nwscript/functioncontext.h"
#include "src/aurora/nwscript/objectman.h"

#include "src/engines/kotorbase/types.h"
#include "src/engines/kotorbase/area.h"
#include "src/engines/kotorbase/module.h"
#include "src/engines/kotorbase/objectcontainer.h"
#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/door.h"
#include "src/engines/kotorbase/waypoint.h"
#include "src/engines/kotorbase/store.h"

#include "src/aurora/2dareg.h"
#include "src/aurora/2dafile.h"
#include "src/engines/kotorbase/creatureinfo.h"
#include "src/engines/kotorbase/game.h"
#include "src/aurora/talkman.h"
#include "src/engines/kotorbase/script/functions.h"


namespace Engines {

namespace KotORBase {

void Functions::actionWorkbench(Aurora::NWScript::FunctionContext &ctx) {
	_game->getModule().setGlobalBoolean("__open_workbench", true);
}

void Functions::getModule(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = (Aurora::NWScript::Object *) &_game->getModule();
}

void Functions::getFirstPC(Aurora::NWScript::FunctionContext &ctx) {
	// The character the player controls: the party leader (as in KotOR.js),
	// which is the player character unless the player switched to a companion.
	Module &module = _game->getModule();
	Creature *leader = module.getPartyLeader();

	ctx.getReturn() = (Aurora::NWScript::Object *) (leader ? leader : module.getPC());
}

void Functions::getNextPC(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = (Aurora::NWScript::Object *) 0;
}

void Functions::getPCSpeaker(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = (Aurora::NWScript::Object *) _game->getModule().getPC();
}

// musicBackgroundGetNightTrack is implemented in functions_sound.cpp

void Functions::musicBattlePlay(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	Area *area = _game->getModule().getCurrentArea();
	if (area)
		area->playBattleMusic();
}

void Functions::musicBattleStop(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	Area *area = _game->getModule().getCurrentArea();
	if (area)
		area->playAmbientMusic();
}

void Functions::musicBattleChange(Aurora::NWScript::FunctionContext &ctx) {
	int track = ctx.getParams()[1].getInt();
	Area *area = _game->getModule().getCurrentArea();
	if (area)
		area->setMusicBattleTrack(track);
}

void Functions::setPartyAIStyle(Aurora::NWScript::FunctionContext &ctx) {
	int style = ctx.getParams()[0].getInt();
	debugC(Common::kDebugEngineLogic, 1, "SetPartyAIStyle: %d", style);
	_game->getModule().setPartyAIStyle(style);
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
	_game->getModule().setAreaUnescapable(ctx.getParams()[0].getInt() != 0);
}

void Functions::getAreaUnescapable(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = _game->getModule().getAreaUnescapable() ? 1 : 0;
}

void Functions::getCurrentForcePoints(Aurora::NWScript::FunctionContext &ctx) {
	Creature *creature = ObjectContainer::toCreature(getParamObject(ctx, 0));
	ctx.getReturn() = creature ? creature->getForcePoints() : 0;
}

void Functions::getMaxForcePoints(Aurora::NWScript::FunctionContext &ctx) {
	Creature *creature = ObjectContainer::toCreature(getParamObject(ctx, 0));
	ctx.getReturn() = creature ? creature->getMaxForcePoints() : 0;
}

void Functions::pauseGame(Aurora::NWScript::FunctionContext &ctx) {
	// As KotOR.js: a generic pause, as if the player had pressed the pause key.
	if (ctx.getParams()[0].getInt() != 0)
		_game->getModule().pauseGame(kPauseReasonGeneric);
	else
		_game->getModule().unpauseGame();
}

void Functions::setPlayerRestrictMode(Aurora::NWScript::FunctionContext &ctx) {
	_game->getModule().setGlobalNumber("__player_restrict_mode", ctx.getParams()[0].getInt());
}

void Functions::getPlayerRestrictMode(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = _game->getModule().getGlobalNumber("__player_restrict_mode");
}

void Functions::getGameDifficulty(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	int d = ConfigMan.getInt("difficulty", 1);
	if (d < 0)
		d = 0;
	if (d > 2)
		d = 2;
	ctx.getReturn() = d;
}

void Functions::getDifficultyModifier(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	int d = ConfigMan.getInt("difficulty", 1);
	if (d < 0)
		d = 0;
	if (d > 2)
		d = 2;
	// Simple scaling hook for combat scripts (retail uses a richer table).
	float m = 1.0f;
	if (d == 0)
		m = 0.8f;
	else if (d == 2)
		m = 1.2f;
	ctx.getReturn() = m;
}

void Functions::setCameraMode(Aurora::NWScript::FunctionContext &ctx) {
	// SetCameraMode(object oPlayer, int nCameraMode): the CAMERA_MODE_*
	// constants (chase, top-down, stiff chase) select the *gameplay* camera,
	// not a cinematic shot. Hand the camera back to the player, unless a
	// conversation is framing the scene.
	(void)ctx;

	Module &module = _game->getModule();
	if (!module.isInConversation())
		module.restoreGameplayCamera(0.0f);
}

void Functions::setCutsceneMode(Aurora::NWScript::FunctionContext &ctx) {
	// SetCutsceneMode(int bEnabled)
	bool enabled = ctx.getParams()[0].getInt() != 0;
	_game->getModule().setCutsceneMode(enabled);
}

void Functions::setPlayerInputEnabled(Aurora::NWScript::FunctionContext &ctx) {
	// SetPlayerInputEnabled(int bEnabled)
	bool enabled = ctx.getParams()[0].getInt() != 0;
	_game->getModule().setPlayerInputEnabled(enabled);
}

void Functions::setCameraTarget(Aurora::NWScript::FunctionContext &ctx) {
	// SetCameraTarget(object oTarget)
	Object *target = ObjectContainer::toObject(ctx.getParams()[0].getObject());
	_game->getModule().setCameraTarget(target);
}

void Functions::cameraTransitionToTarget(Aurora::NWScript::FunctionContext &ctx) {
	// CameraTransitionToTarget(float fBlendTime)
	float blendTime = ctx.getParams()[0].getFloat();
	_game->getModule().cameraTransitionToTarget(blendTime);
}

void Functions::cameraMoveAlongPath(Aurora::NWScript::FunctionContext &ctx) {
	// CameraMoveAlongPath(object oStart, object oEnd, float fDuration)
	Object *start = ObjectContainer::toObject(ctx.getParams()[0].getObject());
	Object *end = ObjectContainer::toObject(ctx.getParams()[1].getObject());
	float duration = ctx.getParams()[2].getFloat();
	_game->getModule().cameraMoveAlongPath(start, end, duration);
}

void Functions::cameraHold(Aurora::NWScript::FunctionContext &ctx) {
	// CameraHold(float fDuration)
	float duration = ctx.getParams()[0].getFloat();
	_game->getModule().cameraHold(duration);
}

void Functions::cameraLookAtObject(Aurora::NWScript::FunctionContext &ctx) {
	// CameraLookAtObject(object oTarget, float fBlendTime)
	Object *target = ObjectContainer::toObject(ctx.getParams()[0].getObject());
	float blendTime = ctx.getParams()[1].getFloat();
	_game->getModule().enterCinematicMode();
	_game->getModule().setCinematicFocus(target);
	// Honour the requested blend so the look-at eases in rather than snapping.
	_game->getModule().cameraTransitionToTarget(blendTime);
}

void Functions::restoreGameplayCamera(Aurora::NWScript::FunctionContext &ctx) {
	// RestoreGameplayCamera(float fBlendTime)
	float blendTime = ctx.getParams()[0].getFloat();
	_game->getModule().restoreGameplayCamera(blendTime);
}

void Functions::playMusicStinger(Aurora::NWScript::FunctionContext &ctx) {
	// PlayMusicStinger(string sStinger)
	Common::UString stinger = ctx.getParams()[0].getString();
	_game->getModule().playMusicStinger(stinger);
}

void Functions::setLockOrientationInDialog(Aurora::NWScript::FunctionContext &ctx) {
	Creature *creature = ObjectContainer::toCreature(getParamObject(ctx, 0));
	if (!creature)
		return;

	creature->setLockOrientationInDialog(ctx.getParams()[1].getInt() != 0);
}

void Functions::setLockHeadFollowInDialog(Aurora::NWScript::FunctionContext &ctx) {
	Creature *creature = ObjectContainer::toCreature(getParamObject(ctx, 0));
	if (!creature)
		return;

	creature->setLockHeadFollowInDialog(ctx.getParams()[1].getInt() != 0);
}

void Functions::setCameraFacing(Aurora::NWScript::FunctionContext &ctx) {
	// void SetCameraFacing(float fDirection) -- KotOR I and II take only the
	// direction; distance/pitch/transition are NWN-only parameters.
	float direction = ctx.getParams()[0].getFloat();

	if (direction >= 0.0f)
		_game->getModule().setCameraYaw(Common::deg2rad(direction));
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
	ctx.getReturn() = _game->getModule().getPartyAIStyle();
}

void Functions::getNPCAIStyle(Aurora::NWScript::FunctionContext &ctx) {
	// k_inc_generic picks companions' tactics by this (NPC_AISTYLE_*)
	const Object *object = ObjectContainer::toCreature(getParamObject(ctx, 0));
	ctx.getReturn() = object ? object->getLocalInt("__npc_ai_style") : 0;
}

void Functions::shipBuild(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	ctx.getReturn() = 0;
}

void Functions::startNewModule(Aurora::NWScript::FunctionContext &ctx) {
	const Common::UString mod = ctx.getParams()[0].getString();
	const Common::UString entry = ctx.getParams()[1].getString();

	if (!_game->hasModule(mod)) {
		warning("Can't start module \"%s\": No such module", mod.c_str());
		return;
	}

	const ObjectType entryType = entry.empty() ? kObjectTypeAll : kObjectTypeWaypoint;
	_game->getModule().load(mod, entry, entryType);
}

void Functions::setGlobalFadeOut(Aurora::NWScript::FunctionContext &ctx) {
	float wait = ctx.getParams()[0].getFloat();
	float run = ctx.getParams()[1].getFloat();
	float r = ctx.getParams()[2].getFloat();
	float g = ctx.getParams()[3].getFloat();
	float b = ctx.getParams()[4].getFloat();

	Graphics::Aurora::FadeQuad &fadeQuad = _game->getModule().getFadeQuad();

	// A scripted fade takes over from HoldWorldFadeInForDialog (as KotOR.js).
	_game->getModule().releaseWorldFadeInForDialog();

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

	// A scripted fade takes over from HoldWorldFadeInForDialog (as KotOR.js).
	_game->getModule().releaseWorldFadeInForDialog();

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

void Functions::showLevelUpGUI(Aurora::NWScript::FunctionContext &) {
	_game->showLevelUpGUI(nullptr);
}

void Functions::popUpGUIPanel(Aurora::NWScript::FunctionContext &ctx) {
	// PopUpGUIPanel(object oPC, int nGUIPanel) -- single-player: oPC is the PC.
	int panel = ctx.getParams()[1].getInt();
	debugC(Common::kDebugEngineLogic, 1, "PopUpGUIPanel: %d", panel);
	_game->getModule().showGUIPanel(panel);
}

void Functions::popUpDeathGUIPanel(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	debugC(Common::kDebugEngineLogic, 1, "PopUpDeathGUIPanel");
	_game->getModule().showDeathGUI();
}

void Functions::displayFeedBackText(Aurora::NWScript::FunctionContext &ctx) {
	Object *creature = ObjectContainer::toObject(getParamObject(ctx, 0));
	const int textConstant = ctx.getParams()[1].getInt();
	if (!creature)
		return;

	Common::UString text;
	try {
		const Aurora::TwoDAFile &feedback = TwoDAReg.get2DA("feedbacktext");
		if (textConstant >= 0 && static_cast<size_t>(textConstant) < feedback.getRowCount()) {
			const uint32_t strRef = static_cast<uint32_t>(feedback.getRow(textConstant).getInt("STRREF"));
			if (strRef != Aurora::kStrRefInvalid)
				text = TalkMan.getString(strRef);
		}
	} catch (...) {
	}

	if (text.empty())
		text = Common::String::format("<feedback:%d>", textConstant);

	_game->getModule().showFloatingText(creature, text);
}

void Functions::addJournalQuestEntry(Aurora::NWScript::FunctionContext &ctx) {
	const Common::UString quest = ctx.getParams()[0].getString();
	const uint32_t state = ctx.getParams()[1].getInt();
	_game->getModule().addJournalQuestEntry(quest, state);
}

void Functions::setMapPinEnabled(Aurora::NWScript::FunctionContext &ctx) {
	Waypoint *waypoint = ObjectContainer::toWaypoint(getParamObject(ctx, 0));
	if (!waypoint)
		return;

	waypoint->enableMapNote(ctx.getParams()[1].getInt() != 0);
	_game->getModule().updateMinimap();
}

void Functions::setJournalQuestEntryPicture(Aurora::NWScript::FunctionContext &ctx) {
	const Common::UString &quest = ctx.getParams()[0].getString();
	Object *source = ObjectContainer::toObject(getParamObject(ctx, 1));
	const uint32_t state = static_cast<uint32_t>(ctx.getParams()[2].getInt());
	const bool enabled = ctx.getParams()[3].getInt() != 0;

	Common::UString portrait;
	if (source)
		portrait = source->getPortrait();

	_game->getModule().setJournalQuestEntryPicture(quest, state, portrait, enabled);
}

void Functions::openStore(Aurora::NWScript::FunctionContext &ctx) {
	// OpenStore(object oStore, object oPC, int nBonusMarkUp=0, int nMarkDown=0)
	// Presents the store GUI to the player. The store is addressed by object;
	// the GUI path looks stores up by tag. The bonuses add to the store's own
	// percentages for this visit only.
	Object *store = ObjectContainer::toObject(getParamObject(ctx, 0));
	if (!store) {
		warning("Functions::openStore(): invalid store object");
		return;
	}

	const uint32_t storeID = store->getID();
	Store *shop = ObjectContainer::toStore(store);
	if (shop)
		shop->setBonusMarks(ctx.getParams()[2].getInt(), ctx.getParams()[3].getInt());

	_game->openStoreGUI(store->getTag());

	// The GUI may have been closed by a script that destroyed the store.
	if (shop && ObjectMan.findObject(storeID) == shop)
		shop->setBonusMarks(0, 0);
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
	Door *door = ObjectContainer::toDoor(getParamObject(ctx, 0));
	if (!door) {
		ctx.getReturn() = (Aurora::NWScript::Object *) nullptr;
		return;
	}

	ctx.getReturn() = door->getTransitionTarget();
}

void Functions::getModuleName(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = _game->getModule().getName();
}

void Functions::setAreaTransitionBMP(Aurora::NWScript::FunctionContext &ctx) {
	// SetAreaTransitionBMP(int nPredefinedAreaTransition, string sCustomAreaTransitionBMP="")
	int bmp = ctx.getParams()[0].getInt();
	const Common::UString &custom = ctx.getParams()[1].getString();
	info("Transition BMP set: %d (custom \"%s\")", bmp, custom.c_str());
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

void Functions::floatingTextStringOnCreature(Aurora::NWScript::FunctionContext &ctx) {
	const Common::UString &text = ctx.getParams()[0].getString();
	Object *object = ObjectContainer::toObject(ctx.getParams()[1].getObject());

	if (!object || text.empty())
		return;

	_game->getModule().showFloatingText(object, text);
}

void Functions::floatingTextStrRefOnCreature(Aurora::NWScript::FunctionContext &ctx) {
	int strRef = ctx.getParams()[0].getInt();
	Object *object = ObjectContainer::toObject(ctx.getParams()[1].getObject());

	if (!object)
		return;

	Common::UString text = TalkMan.getString(strRef);
	if (text.empty())
		text = Common::String::format("<strref:%d>", strRef);

	_game->getModule().showFloatingText(object, text);
}

void Functions::addJournalWorldEntry(Aurora::NWScript::FunctionContext &ctx) {
	// AddJournalWorldEntry(int nIndex, string szEntry, string szTitle="World Entry")
	// World entries are keyed by their index; the journal GUI uses the first
	// line of the text as the list label, so prepend the title there.
	const Common::UString tag = Common::String::format("%d", ctx.getParams()[0].getInt());
	const Common::UString &entry = ctx.getParams()[1].getString();
	const Common::UString &title = ctx.getParams()[2].getString();
	const Common::UString text = title.empty() ? entry : (title + "\n" + entry);

	_game->getModule().addJournalWorldEntry(tag, text);
	debugC(Common::kDebugEngineLogic, 1, "World Journal Entry Added [%s]: %s", tag.c_str(), text.c_str());
}

void Functions::addJournalWorldEntryStrref(Aurora::NWScript::FunctionContext &ctx) {
	// AddJournalWorldEntryStrref(int strref, int strrefTitle)
	// Keyed by the entry strref so DeleteJournalWorldEntryStrref() finds it.
	const int strRef = ctx.getParams()[0].getInt();
	const int titleStrRef = ctx.getParams()[1].getInt();
	const Common::UString tag = Common::String::format("<strref:%d>", strRef);

	Common::UString text = TalkMan.getString(strRef);
	if (text.empty())
		text = tag;

	const Common::UString title = TalkMan.getString(titleStrRef);
	if (!title.empty())
		text = title + "\n" + text;

	_game->getModule().addJournalWorldEntry(tag, text);
	debugC(Common::kDebugEngineLogic, 1, "World Journal Entry Added [%s]: %s", tag.c_str(), text.c_str());
}

void Functions::deleteJournalWorldEntry(Aurora::NWScript::FunctionContext &ctx) {
	// DeleteJournalWorldEntry(int nIndex)
	const Common::UString tag = Common::String::format("%d", ctx.getParams()[0].getInt());
	_game->getModule().deleteJournalWorldEntry(tag);
	debugC(Common::kDebugEngineLogic, 1, "World Journal Entry Deleted [%s]", tag.c_str());
}

void Functions::deleteJournalWorldEntryStrref(Aurora::NWScript::FunctionContext &ctx) {
	const int strRef = ctx.getParams()[0].getInt();
	_game->getModule().deleteJournalWorldEntryByStrref(strRef);
	debugC(Common::kDebugEngineLogic, 1, "World Journal Entry Deleted by Strref [%d]", strRef);
}

void Functions::deleteJournalWorldAllEntries(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	_game->getModule().deleteJournalWorldAllEntries();
	debugC(Common::kDebugEngineLogic, 1, "All World Journal Entries Deleted");
}


void Functions::doSinglePlayerAutoSave(Aurora::NWScript::FunctionContext &UNUSED(ctx)) {
	debugC(Common::kDebugEngineLogic, 1, "DoSinglePlayerAutoSave triggered");
	_game->getModule().requestAutoSave();
}

void Functions::setTutorialWindowsEnabled(Aurora::NWScript::FunctionContext &ctx) {
	int enabled = ctx.getParams()[0].getInt();
	debugC(Common::kDebugEngineLogic, 1, "SetTutorialWindowsEnabled: %d", enabled);
	_game->getModule().setTutorialWindowsEnabled(enabled != 0);
}

void Functions::showTutorialWindow(Aurora::NWScript::FunctionContext &ctx) {
	int windowId = ctx.getParams()[0].getInt();
	debugC(Common::kDebugEngineLogic, 1, "ShowTutorialWindow: %d", windowId);
	_game->getModule().requestTutorialWindow(windowId);
}

void Functions::startCreditSequence(Aurora::NWScript::FunctionContext &ctx) {
	debugC(Common::kDebugEngineLogic, 1, "StartCreditSequence");
}

void Functions::isCreditSequenceInProgress(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = 0; // Not implemented
}

void Functions::saveNPCState(Aurora::NWScript::FunctionContext &ctx) {
	_game->getModule().saveNPCState(ctx.getParams()[0].getInt());
}


void Functions::getPCLevellingUp(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = (Aurora::NWScript::Object *)nullptr; }
void Functions::setPlaceableIllumination(Aurora::NWScript::FunctionContext &ctx) {}
void Functions::getPlaceableIllumination(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = 0; }
void Functions::resetDialogState(Aurora::NWScript::FunctionContext &ctx) {}
void Functions::holdWorldFadeInForDialog(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	_game->getModule().holdWorldFadeInForDialog();
}

} // End of namespace KotORBase
} // End of namespace Engines
