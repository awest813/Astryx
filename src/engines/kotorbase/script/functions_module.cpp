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

#include "src/aurora/nwscript/functioncontext.h"

#include "src/engines/kotorbase/types.h"
#include "src/engines/kotorbase/module.h"
#include "src/engines/kotorbase/objectcontainer.h"
#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/creatureinfo.h"
#include "src/engines/kotorbase/game.h"

#include "src/engines/kotor/encounters_dan.h"
#include "src/engines/kotor/encounters_end.h"

namespace Engines {

namespace KotORBase {

void Functions::signalEncounter(Aurora::NWScript::FunctionContext &ctx) {
	Common::UString encounterID = ctx.getParams()[0].getString();
	Module &module = _game->getModule();

	module.signalEncounter(encounterID);
}

void Functions::actionWorkbench(Aurora::NWScript::FunctionContext &ctx) {
	_game->getModule().setGlobalBoolean("__open_workbench", true);
}

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

void Functions::musicBackgroundGetNightTrack(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = _game->getModule().getMusicNightTrack();
}

void Functions::musicBattlePlay(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	_game->getModule().playMusicBattle();
}

void Functions::musicBattleStop(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	_game->getModule().stopMusicBattle();
}

void Functions::musicBattleChange(Aurora::NWScript::FunctionContext &ctx) {
	int track = ctx.getParams()[1].getInt();
	_game->getModule().setMusicBattleTrack(track);
}

void Functions::setPartyAIStyle(Aurora::NWScript::FunctionContext &ctx) {
	int style = ctx.getParams()[0].getInt();
	debug("SetPartyAIStyle: %d", style);
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
	// SetCameraMode(object oPlayer, int nCameraMode)
	Object *player = ObjectContainer::toObject(ctx.getParams()[0].getObject());
	int mode = ctx.getParams()[1].getInt();

	_game->getModule().setCameraMode(static_cast<CameraMode>(mode), player);
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
	// blendTime ignored in simple implementation
	_game->getModule().setCameraTarget(target);
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
	// SetLockOrientationInDialog(object oCreature, int nValue)
	// Prevents the creature from rotating to face the conversation partner.
	// Stub: no orientation tracking per-creature yet.
	(void)ctx;
}

void Functions::setLockHeadFollowInDialog(Aurora::NWScript::FunctionContext &ctx) {
	// SetLockHeadFollowInDialog(object oCreature, int nValue)
	// Prevents head-tracking look-at behaviour during a dialog.
	// Stub: no head-tracking system yet.
	(void)ctx;
}

void Functions::setCameraFacing(Aurora::NWScript::FunctionContext &ctx) {
	// void SetCameraFacing(float fDirection, float fDistance = -1.0f, float fPitch = -1.0f, int nTransitionType = CAMERA_TRANSITION_TYPE_SNAP)
	float direction = ctx.getParams()[0].getFloat();
	float distance = ctx.getParams()[1].getFloat();
	float pitch = ctx.getParams()[2].getFloat();

	if (direction >= 0.0f)
		_game->getModule().setCameraYaw(Common::deg2rad(direction));
	
	if (distance >= 0.0f)
		_game->getModule().setCameraDistance(distance);

	if (pitch >= 0.0f)
		_game->getModule().setCameraPitch(pitch);
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

void Functions::showLevelUpGUI(Aurora::NWScript::FunctionContext &) {
	// presents the level-up screen for the PC.
	_game->showLevelUpGUI();
}

void Functions::popUpGUIPanel(Aurora::NWScript::FunctionContext &ctx) {
	int panel = ctx.getParams()[0].getInt();
	debug("PopUpGUIPanel: %d", panel);
	_game->getModule().showGUIPanel(panel);
}

void Functions::popUpDeathGUIPanel(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	debug("PopUpDeathGUIPanel");
	_game->getModule().showDeathGUI();
}

void Functions::addJournalQuestEntry(Aurora::NWScript::FunctionContext &ctx) {
	const Common::UString quest = ctx.getParams()[0].getString();
	const uint32_t state = ctx.getParams()[1].getUint();
	_game->getModule().addJournalQuestEntry(quest, state);
}

void Functions::openStore(Aurora::NWScript::FunctionContext &ctx) {
	// OpenStore(object oStore, object oPC, int nBonusMarkUp=0, int nMarkDown=0)
	// Presents the store GUI to the player.
	Common::UString tag = ctx.getParams()[0].getString();
	_game->openStoreGUI(tag);
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

void Functions::floatingTextStringOnCreature(Aurora::NWScript::FunctionContext &ctx) {
	// void FloatingTextStringOnCreature(string sText, object oCreatureToFloatOn, int bBroadcastToParty = TRUE)
	const Common::UString &text = ctx.getParams()[0].getString();
	Object *object = ObjectContainer::toObject(ctx.getParams()[1].getObject());

	if (!object || text.empty())
		return;

	status("FloatingText [%s]: %s", object->getTag().c_str(), text.c_str());
}

void Functions::floatingTextStrRefOnCreature(Aurora::NWScript::FunctionContext &ctx) {
	// void FloatingTextStrRefOnCreature(int nStrRef, object oCreatureToFloatOn, int bBroadcastToParty = TRUE)
	int strRef = ctx.getParams()[0].getInt();
	Object *object = ObjectContainer::toObject(ctx.getParams()[1].getObject());

	if (!object)
		return;

	Common::UString text = TalkMan.getString(strRef);
	if (!text.empty())
		status("FloatingTextStrRef [%s]: %s", object->getTag().c_str(), text.c_str());
}

void Functions::addJournalWorldEntry(Aurora::NWScript::FunctionContext &ctx) {
	// void AddJournalWorldEntry(string sTag, string sText, object oPC = OBJECT_SELF)
	const Common::UString &tag = ctx.getParams()[0].getString();
	const Common::UString &text = ctx.getParams()[1].getString();

	debug("World Journal Entry Added [%s]: %s", tag.c_str(), text.c_str());
	// We'd store this in the journal state if we had a world-entry specific list
}

void Functions::addJournalWorldEntryStrref(Aurora::NWScript::FunctionContext &ctx) {
	// void AddJournalWorldEntryStrref(string sTag, int nStrRef, object oPC = OBJECT_SELF)
	const Common::UString &tag = ctx.getParams()[0].getString();
	int strRef = ctx.getParams()[1].getInt();

	Common::UString text = TalkMan.getString(strRef);
	debug("World Journal Entry Added [%s]: %s", tag.c_str(), text.c_str());
}

void Functions::deleteJournalWorldEntry(Aurora::NWScript::FunctionContext &ctx) {
	const Common::UString &tag = ctx.getParams()[0].getString();
	debug("World Journal Entry Deleted [%s]", tag.c_str());
}

void Functions::deleteJournalWorldAllEntries(Aurora::NWScript::FunctionContext &ctx) {
	(void)ctx;
	debug("All World Journal Entries Deleted");
}


void Functions::doSinglePlayerAutoSave(Aurora::NWScript::FunctionContext &ctx) {
	debugC(Common::kDebugEngineLogic, 1, "DoSinglePlayerAutoSave triggered");
	// Trigger autosave logic
}

void Functions::setTutorialWindowsEnabled(Aurora::NWScript::FunctionContext &ctx) {
	int enabled = ctx.getParams()[0].getInt();
	debugC(Common::kDebugEngineLogic, 1, "SetTutorialWindowsEnabled: %d", enabled);
}

void Functions::showTutorialWindow(Aurora::NWScript::FunctionContext &ctx) {
	int windowId = ctx.getParams()[0].getInt();
	debugC(Common::kDebugEngineLogic, 1, "ShowTutorialWindow: %d", windowId);
}

void Functions::startCreditSequence(Aurora::NWScript::FunctionContext &ctx) {
	debugC(Common::kDebugEngineLogic, 1, "StartCreditSequence");
}

void Functions::isCreditSequenceInProgress(Aurora::NWScript::FunctionContext &ctx) {
	ctx.getReturn() = 0; // Not implemented
}

void Functions::saveNPCState(Aurora::NWScript::FunctionContext &ctx) {
	debugC(Common::kDebugEngineLogic, 1, "SaveNPCState");
}


void Functions::getPCLevellingUp(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = (Aurora::NWScript::Object *)nullptr; }
void Functions::setPlaceableIllumination(Aurora::NWScript::FunctionContext &ctx) {}
void Functions::getPlaceableIllumination(Aurora::NWScript::FunctionContext &ctx) { ctx.getReturn() = 0; }
void Functions::resetDialogState(Aurora::NWScript::FunctionContext &ctx) {}
void Functions::holdWorldFadeInForDialog(Aurora::NWScript::FunctionContext &ctx) {}

} // End of namespace KotORBase
} // End of namespace Engines
