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

#ifndef ENGINES_KOTORBASE_SCRIPT_FUNCTIONS_H
#define ENGINES_KOTORBASE_SCRIPT_FUNCTIONS_H

#include <cstddef>
#include <map>
#include <vector>

#include "src/common/ustring.h"

#include "src/aurora/nwscript/types.h"

namespace Aurora {
	namespace NWScript {
		class FunctionContext;
		class Object;
	}
}

namespace Engines {

namespace KotORBase {

class Game;
class Area;
class Object;
class Event;

class Functions {
public:
	Functions(Game &game);
	virtual ~Functions();

protected:
	typedef void (Functions::*funcPtr)(Aurora::NWScript::FunctionContext &ctx);

	struct FunctionPointer {
		uint32_t id;
		const char *name;
		funcPtr func;
	};

	struct FunctionSignature {
		uint32_t id;
		Aurora::NWScript::Type returnType;
		Aurora::NWScript::Type parameters[15];
	};

	struct FunctionDefaults {
		uint32_t id;
		const Aurora::NWScript::Variable *defaults[14];
	};

	virtual void registerFunctions() = 0;

	// Utility methods

	void jumpTo(Object *object, float x, float y, float z);

	static int32_t getRandom(int min, int max, int32_t n = 1);

	static Common::UString formatFloat(float f, int width = 18, int decimals = 9);

	static Aurora::NWScript::Object *getParamObject(const Aurora::NWScript::FunctionContext &ctx, size_t n);

	// Engine functions

	void getRunScriptVar(Aurora::NWScript::FunctionContext &ctx);

	void unimplementedFunction(Aurora::NWScript::FunctionContext &ctx);
	void executeScript(Aurora::NWScript::FunctionContext &ctx);

	// Math, functions_math.cpp

	void abs (Aurora::NWScript::FunctionContext &ctx);
	void fabs(Aurora::NWScript::FunctionContext &ctx);

	void cos(Aurora::NWScript::FunctionContext &ctx);
	void sin(Aurora::NWScript::FunctionContext &ctx);
	void tan(Aurora::NWScript::FunctionContext &ctx);

	void acos(Aurora::NWScript::FunctionContext &ctx);
	void asin(Aurora::NWScript::FunctionContext &ctx);
	void atan(Aurora::NWScript::FunctionContext &ctx);

	void log (Aurora::NWScript::FunctionContext &ctx);
	void pow (Aurora::NWScript::FunctionContext &ctx);
	void sqrt(Aurora::NWScript::FunctionContext &ctx);

	void random(Aurora::NWScript::FunctionContext &ctx);

	void d2  (Aurora::NWScript::FunctionContext &ctx);
	void d3  (Aurora::NWScript::FunctionContext &ctx);
	void d4  (Aurora::NWScript::FunctionContext &ctx);
	void d6  (Aurora::NWScript::FunctionContext &ctx);
	void d8  (Aurora::NWScript::FunctionContext &ctx);
	void d10 (Aurora::NWScript::FunctionContext &ctx);
	void d12 (Aurora::NWScript::FunctionContext &ctx);
	void d20 (Aurora::NWScript::FunctionContext &ctx);
	void d100(Aurora::NWScript::FunctionContext &ctx);

	void intToFloat(Aurora::NWScript::FunctionContext &ctx);
	void floatToInt(Aurora::NWScript::FunctionContext &ctx);

	void vector         (Aurora::NWScript::FunctionContext &ctx);
	void vectorMagnitude(Aurora::NWScript::FunctionContext &ctx);
	void vectorNormalize(Aurora::NWScript::FunctionContext &ctx);
	void roundsToSeconds(Aurora::NWScript::FunctionContext &ctx);
	void hoursToSeconds(Aurora::NWScript::FunctionContext &ctx);
	void turnsToSeconds(Aurora::NWScript::FunctionContext &ctx);
	void feetToMeters(Aurora::NWScript::FunctionContext &ctx);
	void yardsToMeters(Aurora::NWScript::FunctionContext &ctx);

	// Strings, functions_string.cpp

	void writeTimestampedLogEntry(Aurora::NWScript::FunctionContext &ctx);

	void sendMessageToPC(Aurora::NWScript::FunctionContext &ctx);

	void printInteger(Aurora::NWScript::FunctionContext &ctx);
	void printFloat  (Aurora::NWScript::FunctionContext &ctx);
	void printString (Aurora::NWScript::FunctionContext &ctx);
	void printObject (Aurora::NWScript::FunctionContext &ctx);
	void printVector (Aurora::NWScript::FunctionContext &ctx);

	void intToString   (Aurora::NWScript::FunctionContext &ctx);
	void floatToString (Aurora::NWScript::FunctionContext &ctx);
	void objectToString(Aurora::NWScript::FunctionContext &ctx);

	void intToHexString(Aurora::NWScript::FunctionContext &ctx);

	void stringToInt  (Aurora::NWScript::FunctionContext &ctx);
	void stringToFloat(Aurora::NWScript::FunctionContext &ctx);

	void getStringLength(Aurora::NWScript::FunctionContext &ctx);

	void getStringUpperCase(Aurora::NWScript::FunctionContext &ctx);
	void getStringLowerCase(Aurora::NWScript::FunctionContext &ctx);

	void getStringRight(Aurora::NWScript::FunctionContext &ctx);
	void getStringLeft (Aurora::NWScript::FunctionContext &ctx);

	void insertString (Aurora::NWScript::FunctionContext &ctx);
	void getSubString (Aurora::NWScript::FunctionContext &ctx);
	void findSubString(Aurora::NWScript::FunctionContext &ctx);

	void getStringByStrRef(Aurora::NWScript::FunctionContext &ctx);

	// Module functions, functions_module.cpp

	void getModule(Aurora::NWScript::FunctionContext &ctx);
	void getFirstPC(Aurora::NWScript::FunctionContext &ctx);
	void getNextPC(Aurora::NWScript::FunctionContext &ctx);
	void getPCSpeaker(Aurora::NWScript::FunctionContext &ctx);
	void getIsConversationActive(Aurora::NWScript::FunctionContext &ctx);
	void switchPlayerCharacter(Aurora::NWScript::FunctionContext &ctx);
	void setTime(Aurora::NWScript::FunctionContext &ctx);
	void setAreaUnescapable(Aurora::NWScript::FunctionContext &ctx);
	void getAreaUnescapable(Aurora::NWScript::FunctionContext &ctx);
	void getCurrentForcePoints(Aurora::NWScript::FunctionContext &ctx);
	void getMaxForcePoints(Aurora::NWScript::FunctionContext &ctx);
	void pauseGame(Aurora::NWScript::FunctionContext &ctx);
	void setPlayerRestrictMode(Aurora::NWScript::FunctionContext &ctx);
	void getPlayerRestrictMode(Aurora::NWScript::FunctionContext &ctx);
	void getGameDifficulty(Aurora::NWScript::FunctionContext &ctx);
	void getDifficultyModifier(Aurora::NWScript::FunctionContext &ctx);
	void setCameraMode(Aurora::NWScript::FunctionContext &ctx);
	void setLockOrientationInDialog(Aurora::NWScript::FunctionContext &ctx);
	void setLockHeadFollowInDialog(Aurora::NWScript::FunctionContext &ctx);
	void setCameraFacing(Aurora::NWScript::FunctionContext &ctx);
	void getListenPatternNumber(Aurora::NWScript::FunctionContext &ctx);
	void getLastSpeaker(Aurora::NWScript::FunctionContext &ctx);
	void getPartyAIStyle(Aurora::NWScript::FunctionContext &ctx);
	void getNPCAIStyle(Aurora::NWScript::FunctionContext &ctx);
	void shipBuild(Aurora::NWScript::FunctionContext &ctx);

	void startNewModule(Aurora::NWScript::FunctionContext &ctx);

	void setGlobalFadeOut(Aurora::NWScript::FunctionContext &ctx);
	void setGlobalFadeIn(Aurora::NWScript::FunctionContext &ctx);
	void setReturnStrref(Aurora::NWScript::FunctionContext &ctx);

	// General object functions, functions_object.cpp

	void getClickingObject(Aurora::NWScript::FunctionContext &ctx);
	void getEnteringObject(Aurora::NWScript::FunctionContext &ctx);
	void getExitingObject(Aurora::NWScript::FunctionContext &ctx);
	void getIsObjectValid(Aurora::NWScript::FunctionContext &ctx);
	void getIsPC(Aurora::NWScript::FunctionContext &ctx);
	void getObjectByTag(Aurora::NWScript::FunctionContext &ctx);
	void getMinOneHP(Aurora::NWScript::FunctionContext &ctx);
	void getCurrentHitPoints(Aurora::NWScript::FunctionContext &ctx);
	void getMaxHitPoints(Aurora::NWScript::FunctionContext &ctx);
	void getItemInSlot(Aurora::NWScript::FunctionContext &ctx);
	void getNearestCreature(Aurora::NWScript::FunctionContext &ctx);
	void getNearestObject(Aurora::NWScript::FunctionContext &ctx);
	void getNearestObjectByTag(Aurora::NWScript::FunctionContext &ctx);
	void getSpellTargetObject(Aurora::NWScript::FunctionContext &ctx);
	void getTag(Aurora::NWScript::FunctionContext &ctx);

	void getArea    (Aurora::NWScript::FunctionContext &ctx);
	void getLocation(Aurora::NWScript::FunctionContext &ctx);
	void getPositionFromLocation(Aurora::NWScript::FunctionContext &ctx);
	void getFacingFromLocation(Aurora::NWScript::FunctionContext &ctx);

	void jumpToLocation(Aurora::NWScript::FunctionContext &ctx);
	void jumpToObject  (Aurora::NWScript::FunctionContext &ctx);

	void locationCreate(Aurora::NWScript::FunctionContext &ctx);

	void getWaypointByTag(Aurora::NWScript::FunctionContext &ctx);

	void setMinOneHP(Aurora::NWScript::FunctionContext &ctx);
	void setMaxHitPoints(Aurora::NWScript::FunctionContext &ctx);

	void getStandardFaction(Aurora::NWScript::FunctionContext &ctx);
	void changeToStandardFaction(Aurora::NWScript::FunctionContext &ctx);
	void getReputation(Aurora::NWScript::FunctionContext &ctx);
	void adjustReputation(Aurora::NWScript::FunctionContext &ctx);
	void changeFaction(Aurora::NWScript::FunctionContext &ctx);

	void getFirstObjectInShape(Aurora::NWScript::FunctionContext &ctx);
	void getNextObjectInShape(Aurora::NWScript::FunctionContext &ctx);

	void createObject(Aurora::NWScript::FunctionContext &ctx);

	void getModuleItemAcquired(Aurora::NWScript::FunctionContext &ctx);
	void getModuleItemAcquiredFrom(Aurora::NWScript::FunctionContext &ctx);
	void setCustomToken(Aurora::NWScript::FunctionContext &ctx);

	void getLastPerceived(Aurora::NWScript::FunctionContext &ctx);
	void getLastPerceptionHeard(Aurora::NWScript::FunctionContext &ctx);
	void getLastPerceptionInaudible(Aurora::NWScript::FunctionContext &ctx);
	void getLastPerceptionSeen(Aurora::NWScript::FunctionContext &ctx);
	void getLastPerceptionVanished(Aurora::NWScript::FunctionContext &ctx);

	void getObjectSeen(Aurora::NWScript::FunctionContext &ctx);
	void getObjectHeard(Aurora::NWScript::FunctionContext &ctx);

	void getNearestCreatureToLocation(Aurora::NWScript::FunctionContext &ctx);
	void beginConversation(Aurora::NWScript::FunctionContext &ctx);

	void createItemOnObject(Aurora::NWScript::FunctionContext &ctx);
	void destroyObject(Aurora::NWScript::FunctionContext &ctx);

	void getPosition(Aurora::NWScript::FunctionContext &ctx);
	void getFacing(Aurora::NWScript::FunctionContext &ctx);
	void getItemPossessor(Aurora::NWScript::FunctionContext &ctx);
	void getItemPossessedBy(Aurora::NWScript::FunctionContext &ctx);
	void getObjectType(Aurora::NWScript::FunctionContext &ctx);
	void getDistanceToObject(Aurora::NWScript::FunctionContext &ctx);
	void getDistanceToObject2D(Aurora::NWScript::FunctionContext &ctx);
	void getDistanceBetween(Aurora::NWScript::FunctionContext &ctx);
	void getDistanceBetween2D(Aurora::NWScript::FunctionContext &ctx);
	void getDistanceBetweenLocations(Aurora::NWScript::FunctionContext &ctx);
	void getDistanceBetweenLocations2D(Aurora::NWScript::FunctionContext &ctx);
	void exploreAreaForPlayer(Aurora::NWScript::FunctionContext &ctx);

	void getFirstItemInInventory(Aurora::NWScript::FunctionContext &ctx);
	void getNextItemInInventory(Aurora::NWScript::FunctionContext &ctx);

	void getFirstObjectInArea(Aurora::NWScript::FunctionContext &ctx);
	void getNextObjectInArea(Aurora::NWScript::FunctionContext &ctx);

	void getIsEnemy  (Aurora::NWScript::FunctionContext &ctx);
	void getIsFriend (Aurora::NWScript::FunctionContext &ctx);
	void getIsNeutral(Aurora::NWScript::FunctionContext &ctx);
	void getFactionEqual(Aurora::NWScript::FunctionContext &ctx);

	void getPlotFlag(Aurora::NWScript::FunctionContext &ctx);
	void setPlotFlag(Aurora::NWScript::FunctionContext &ctx);

	void getEffectType(Aurora::NWScript::FunctionContext &ctx);
	void getLastDamager(Aurora::NWScript::FunctionContext &ctx);

	void getName(Aurora::NWScript::FunctionContext &ctx);

	void setIsDestroyable  (Aurora::NWScript::FunctionContext &ctx);
	void getIsInConversation(Aurora::NWScript::FunctionContext &ctx);

	// Quests / Journals
	void addJournalQuestEntry(Aurora::NWScript::FunctionContext &ctx);
	void removeJournalQuestEntry(Aurora::NWScript::FunctionContext &ctx);
	void getJournalEntry(Aurora::NWScript::FunctionContext &ctx);
	void getJournalQuestExperience(Aurora::NWScript::FunctionContext &ctx);

	// Economy
	void giveGoldToCreature(Aurora::NWScript::FunctionContext &ctx);
	void takeGoldFromCreature(Aurora::NWScript::FunctionContext &ctx);
	void getGold(Aurora::NWScript::FunctionContext &ctx);

	// Situated objects, functions_situated.cpp

	void getLocked(Aurora::NWScript::FunctionContext &ctx);
	void setLocked(Aurora::NWScript::FunctionContext &ctx);

	void getIsOpen(Aurora::NWScript::FunctionContext &ctx);

	void getLastOpenedBy(Aurora::NWScript::FunctionContext &ctx);
	void getLastClosedBy(Aurora::NWScript::FunctionContext &ctx);
	void getLastUsedBy  (Aurora::NWScript::FunctionContext &ctx);

	// Actions, functions_action.cpp

	void getCurrentAction(Aurora::NWScript::FunctionContext &ctx);

	void assignCommand(Aurora::NWScript::FunctionContext &ctx);
	void delayCommand (Aurora::NWScript::FunctionContext &ctx);
	void actionStartConversation(Aurora::NWScript::FunctionContext &ctx);
	void actionOpenDoor(Aurora::NWScript::FunctionContext &ctx);
	void actionCloseDoor(Aurora::NWScript::FunctionContext &ctx);
	void actionMoveToObject  (Aurora::NWScript::FunctionContext &ctx);
	void actionMoveToLocation(Aurora::NWScript::FunctionContext &ctx);
	void actionRandomWalk(Aurora::NWScript::FunctionContext &ctx);
	void actionMoveAwayFromObject(Aurora::NWScript::FunctionContext &ctx);
	void actionForceMoveToObject(Aurora::NWScript::FunctionContext &ctx);
	void actionForceMoveToLocation(Aurora::NWScript::FunctionContext &ctx);
	void actionFollowLeader(Aurora::NWScript::FunctionContext &ctx);
	void actionPutDownItem(Aurora::NWScript::FunctionContext &ctx);
	void actionCastSpellAtObject(Aurora::NWScript::FunctionContext &ctx);
	void clearAllActions(Aurora::NWScript::FunctionContext &ctx);

	void setFacing(Aurora::NWScript::FunctionContext &ctx);
	void actionEquipItem(Aurora::NWScript::FunctionContext &ctx);
	void actionPickUpItem(Aurora::NWScript::FunctionContext &ctx);
	void actionAttack(Aurora::NWScript::FunctionContext &ctx);
	void cutsceneAttack(Aurora::NWScript::FunctionContext &ctx);
	void cutsceneMove(Aurora::NWScript::FunctionContext &ctx);
	void actionUseFeat(Aurora::NWScript::FunctionContext &ctx);
	void actionUseTalentOnObject(Aurora::NWScript::FunctionContext &ctx);
	void actionUseTalentAtLocation(Aurora::NWScript::FunctionContext &ctx);
	void cancelCombat(Aurora::NWScript::FunctionContext &ctx);
	void getLastAttacker(Aurora::NWScript::FunctionContext &ctx);
	void actionSpeakString(Aurora::NWScript::FunctionContext &ctx);
	void speakString(Aurora::NWScript::FunctionContext &ctx);
	void actionSpeakStringByStrRef(Aurora::NWScript::FunctionContext &ctx);
	void actionPlayAnimation(Aurora::NWScript::FunctionContext &ctx);
	void playAnimation(Aurora::NWScript::FunctionContext &ctx);
	void actionJumpToObject(Aurora::NWScript::FunctionContext &ctx);
	void actionJumpToLocation(Aurora::NWScript::FunctionContext &ctx);
	void actionInteractObject(Aurora::NWScript::FunctionContext &ctx);
	void actionPauseConversation(Aurora::NWScript::FunctionContext &ctx);
	void actionResumeConversation(Aurora::NWScript::FunctionContext &ctx);
	void actionWait(Aurora::NWScript::FunctionContext &ctx);
	void getUserActionsPending(Aurora::NWScript::FunctionContext &ctx);
	void noClicksFor(Aurora::NWScript::FunctionContext &ctx);
	void actionGiveItem(Aurora::NWScript::FunctionContext &ctx);
	void actionTakeItem(Aurora::NWScript::FunctionContext &ctx);
	void giveItem(Aurora::NWScript::FunctionContext &ctx);
	void actionDoCommand(Aurora::NWScript::FunctionContext &ctx);
	void actionUseSkill(Aurora::NWScript::FunctionContext &ctx);
	void setFacingPoint(Aurora::NWScript::FunctionContext &ctx);

	// Sound, functions_sound.cpp

	void musicBackgroundPlay(Aurora::NWScript::FunctionContext &ctx);
	void musicBackgroundStop(Aurora::NWScript::FunctionContext &ctx);
	void musicBackgroundChangeDay(Aurora::NWScript::FunctionContext &ctx);
	void musicBackgroundChangeNight(Aurora::NWScript::FunctionContext &ctx);

	void musicBackgroundGetDayTrack(Aurora::NWScript::FunctionContext &ctx);
	void musicBackgroundGetNightTrack(Aurora::NWScript::FunctionContext &ctx);

	void soundObjectPlay(Aurora::NWScript::FunctionContext &ctx);
	void soundObjectStop(Aurora::NWScript::FunctionContext &ctx);
	void playRumblePattern(Aurora::NWScript::FunctionContext &ctx);
	void stopRumblePattern(Aurora::NWScript::FunctionContext &ctx);

	// Movies, functions_movie.cpp

	void playMovie(Aurora::NWScript::FunctionContext &ctx);

	// Creatures, functions_creatures.cpp

	void getGender(Aurora::NWScript::FunctionContext &ctx);
	void getLevelByClass(Aurora::NWScript::FunctionContext &ctx);

	void getLevelByPosition(Aurora::NWScript::FunctionContext &ctx);
	void getClassByPosition(Aurora::NWScript::FunctionContext &ctx);

	void getRacialType(Aurora::NWScript::FunctionContext &ctx);
	void getSubRace(Aurora::NWScript::FunctionContext &ctx);

	void getHasFeat(Aurora::NWScript::FunctionContext &ctx);
	void getHasSkill(Aurora::NWScript::FunctionContext &ctx);
	void getSkillRank(Aurora::NWScript::FunctionContext &ctx);
	void getHasFeatEffect(Aurora::NWScript::FunctionContext &ctx);

	void talentSpell(Aurora::NWScript::FunctionContext &ctx);
	void talentFeat(Aurora::NWScript::FunctionContext &ctx);
	void talentSkill(Aurora::NWScript::FunctionContext &ctx);
	void getCreatureHasTalent(Aurora::NWScript::FunctionContext &ctx);
	void getCreatureTalentRandom(Aurora::NWScript::FunctionContext &ctx);
	void getCreatureTalentBest(Aurora::NWScript::FunctionContext &ctx);
	void getIsTalentValid(Aurora::NWScript::FunctionContext &ctx);
	void getTypeFromTalent(Aurora::NWScript::FunctionContext &ctx);
	void getIdFromTalent(Aurora::NWScript::FunctionContext &ctx);
	void getCategoryFromTalent(Aurora::NWScript::FunctionContext &ctx);
	void getLastCombatFeatUsed(Aurora::NWScript::FunctionContext &ctx);

	void getAbilityScore(Aurora::NWScript::FunctionContext &ctx);
	void getAbilityModifier(Aurora::NWScript::FunctionContext &ctx);

	void getIsDead(Aurora::NWScript::FunctionContext &ctx);
	void getHitDice(Aurora::NWScript::FunctionContext &ctx);
	void getAC(Aurora::NWScript::FunctionContext &ctx);
	void getAttackTarget(Aurora::NWScript::FunctionContext &ctx);
	void getAttemptedAttackTarget(Aurora::NWScript::FunctionContext &ctx);
	void getAttemptedSpellTarget(Aurora::NWScript::FunctionContext &ctx);
	void getIsInCombat(Aurora::NWScript::FunctionContext &ctx);
	void getLastHostileActor(Aurora::NWScript::FunctionContext &ctx);

	void effectHeal(Aurora::NWScript::FunctionContext &ctx);
	void effectDamage(Aurora::NWScript::FunctionContext &ctx);
	void effectVisualEffect(Aurora::NWScript::FunctionContext &ctx);
	void effectACIncrease(Aurora::NWScript::FunctionContext &ctx);
	void effectAttackIncrease(Aurora::NWScript::FunctionContext &ctx);
	void effectSkillIncrease(Aurora::NWScript::FunctionContext &ctx);
	void effectTemporaryHitpoints(Aurora::NWScript::FunctionContext &ctx);
	void effectDeath(Aurora::NWScript::FunctionContext &ctx);
	void effectKnockdown(Aurora::NWScript::FunctionContext &ctx);
	void effectParalyze(Aurora::NWScript::FunctionContext &ctx);
	void effectStunned(Aurora::NWScript::FunctionContext &ctx);
	void effectHaste(Aurora::NWScript::FunctionContext &ctx);
	void effectAbilityIncrease(Aurora::NWScript::FunctionContext &ctx);
	void effectMovementSpeedIncrease(Aurora::NWScript::FunctionContext &ctx);
	void effectResurrection(Aurora::NWScript::FunctionContext &ctx);
	void effectCutSceneHorrified(Aurora::NWScript::FunctionContext &ctx);
	void effectCutSceneParalyze(Aurora::NWScript::FunctionContext &ctx);
	void effectCutSceneStunned(Aurora::NWScript::FunctionContext &ctx);
	void applyEffectToObject(Aurora::NWScript::FunctionContext &ctx);

	void touchAttackMelee(Aurora::NWScript::FunctionContext &ctx);
	void touchAttackRanged(Aurora::NWScript::FunctionContext &ctx);

	// Items
	void getBaseItemType(Aurora::NWScript::FunctionContext &ctx);
	void getItemStackSize(Aurora::NWScript::FunctionContext &ctx);
	void setItemStackSize(Aurora::NWScript::FunctionContext &ctx);

	// Global values, functions_global.cpp

	void getGlobalBoolean(Aurora::NWScript::FunctionContext &ctx);
	void setGlobalBoolean(Aurora::NWScript::FunctionContext &ctx);
	void getGlobalNumber(Aurora::NWScript::FunctionContext &ctx);
	void setGlobalNumber(Aurora::NWScript::FunctionContext &ctx);
	void getGlobalString(Aurora::NWScript::FunctionContext &ctx);
	void setGlobalString(Aurora::NWScript::FunctionContext &ctx);

	// Local variables, functions_local.cpp

	void getLocalBoolean(Aurora::NWScript::FunctionContext &ctx);
	void setLocalBoolean(Aurora::NWScript::FunctionContext &ctx);
	void getLocalNumber(Aurora::NWScript::FunctionContext &ctx);
	void setLocalNumber(Aurora::NWScript::FunctionContext &ctx);

	// Party, functions_party.cpp

	void getPartyMemberByIndex(Aurora::NWScript::FunctionContext &ctx);
	void getPartyMemberCount(Aurora::NWScript::FunctionContext &ctx);
	void getSoloMode(Aurora::NWScript::FunctionContext &ctx);
	void setSoloMode(Aurora::NWScript::FunctionContext &ctx);
	void getCommandable(Aurora::NWScript::FunctionContext &ctx);

	void isObjectPartyMember(Aurora::NWScript::FunctionContext &ctx);
	void isNPCPartyMember(Aurora::NWScript::FunctionContext &ctx);
	void isAvailableCreature(Aurora::NWScript::FunctionContext &ctx);

	void setPartyLeader(Aurora::NWScript::FunctionContext &ctx);
	void setCommandable(Aurora::NWScript::FunctionContext &ctx);

	void showPartySelectionGUI(Aurora::NWScript::FunctionContext &ctx);
	void addAvailableNPCByTemplate(Aurora::NWScript::FunctionContext &ctx);
	void addAvailableNPCByObject(Aurora::NWScript::FunctionContext &ctx);
	void removeAvailableNPC(Aurora::NWScript::FunctionContext &ctx);
	void spawnAvailableNPC(Aurora::NWScript::FunctionContext &ctx);
	void addPartyMember(Aurora::NWScript::FunctionContext &ctx);
	void removePartyMember(Aurora::NWScript::FunctionContext &ctx);

	// Alignment, functions_creatures.cpp

	void getGoodEvilValue(Aurora::NWScript::FunctionContext &ctx);
	void getAlignmentGoodEvil(Aurora::NWScript::FunctionContext &ctx);
	void adjustAlignment(Aurora::NWScript::FunctionContext &ctx);
	void setGoodEvilValue(Aurora::NWScript::FunctionContext &ctx);

	// Saving throws, functions_creatures.cpp

	void fortitudeSave(Aurora::NWScript::FunctionContext &ctx);
	void reflexSave(Aurora::NWScript::FunctionContext &ctx);
	void willSave(Aurora::NWScript::FunctionContext &ctx);

	// Experience, functions_creatures.cpp

	void giveXPToCreature(Aurora::NWScript::FunctionContext &ctx);
	void setXP(Aurora::NWScript::FunctionContext &ctx);
	void getXP(Aurora::NWScript::FunctionContext &ctx);
	void givePlotXP(Aurora::NWScript::FunctionContext &ctx);

	// Level-up / store GUI, functions_module.cpp

	void showLevelUpGUI(Aurora::NWScript::FunctionContext &ctx);
	void openStore(Aurora::NWScript::FunctionContext &ctx);

	// Misc, functions_object.cpp / functions_module.cpp

	void playSound(Aurora::NWScript::FunctionContext &ctx);
	void barkString(Aurora::NWScript::FunctionContext &ctx);
	void actionBarkString(Aurora::NWScript::FunctionContext &ctx);
	void actionUnequipItem(Aurora::NWScript::FunctionContext &ctx);
	void setAreaTransitionBMP(Aurora::NWScript::FunctionContext &ctx);
	void getModuleName(Aurora::NWScript::FunctionContext &ctx);
	void getLoadFromSaveGame(Aurora::NWScript::FunctionContext &ctx);
	void endGame(Aurora::NWScript::FunctionContext &ctx);
	void getTransitionTarget(Aurora::NWScript::FunctionContext &ctx);
	void getLockUnlockDC(Aurora::NWScript::FunctionContext &ctx);

	// Galaxy map, functions_module.cpp

	void showGalaxyMap(Aurora::NWScript::FunctionContext &ctx);
	void setPlanetSelectable(Aurora::NWScript::FunctionContext &ctx);
	void getPlanetSelectable(Aurora::NWScript::FunctionContext &ctx);
	void setPlanetAvailable(Aurora::NWScript::FunctionContext &ctx);
	void getPlanetAvailable(Aurora::NWScript::FunctionContext &ctx);
	void getSelectedPlanet(Aurora::NWScript::FunctionContext &ctx);

	// Events, functions_events.cpp

	void signalEvent(Aurora::NWScript::FunctionContext &ctx);
	void eventUserDefined(Aurora::NWScript::FunctionContext &ctx);
	void getUserDefinedEventNumber(Aurora::NWScript::FunctionContext &ctx);

	// Time, functions_time.cpp

	void getTimeHour(Aurora::NWScript::FunctionContext &ctx);
	void getTimeMinute(Aurora::NWScript::FunctionContext &ctx);
	void getTimeSecond(Aurora::NWScript::FunctionContext &ctx);
	void getTimeMillisecond(Aurora::NWScript::FunctionContext &ctx);

	// Parameters, functions_parameters.cpp

	void getScriptParameter(Aurora::NWScript::FunctionContext &ctx);
	void getScriptStringParameter(Aurora::NWScript::FunctionContext &ctx);

private:
	Game *_game;

	// State for GetFirstItemInInventory / GetNextItemInInventory
	Object *_inventoryIterObject { nullptr };
	std::vector<Common::UString> _inventoryIterTags;
	size_t _inventoryIterIndex { 0 };

	// State for GetFirstObjectInArea / GetNextObjectInArea
	std::vector<Object *> _areaIterObjects;
	size_t _areaIterIndex { 0 };

	// State for GetFirstObjectInShape / GetNextObjectInShape
	std::vector<Object *> _shapeIterObjects;
	size_t _shapeIterIndex { 0 };

	// Custom token store (SetCustomToken / GetCustomToken replacements in dialogue)
	std::map<int, Common::UString> _customTokens;
};

} // End of namespace KotOR

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_SCRIPT_FUNCTIONS_H
