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
	friend class DialogGUI;

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

	void stubFunction(Aurora::NWScript::FunctionContext &ctx);
	void stubSWMGFunction(Aurora::NWScript::FunctionContext &ctx);

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


	void stringToFloat (Aurora::NWScript::FunctionContext &ctx);


	void stringToInt   (Aurora::NWScript::FunctionContext &ctx);





	void setCustomToken(Aurora::NWScript::FunctionContext &ctx);


	const Common::UString &getCustomToken(int token) const;





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


	void setCutsceneMode(Aurora::NWScript::FunctionContext &ctx);


	void setPlayerInputEnabled(Aurora::NWScript::FunctionContext &ctx);


	void setCameraTarget(Aurora::NWScript::FunctionContext &ctx);


	void cameraTransitionToTarget(Aurora::NWScript::FunctionContext &ctx);


	void cameraMoveAlongPath(Aurora::NWScript::FunctionContext &ctx);


	void cameraHold(Aurora::NWScript::FunctionContext &ctx);


	void cameraLookAtObject(Aurora::NWScript::FunctionContext &ctx);


	void restoreGameplayCamera(Aurora::NWScript::FunctionContext &ctx);


	void playMusicStinger(Aurora::NWScript::FunctionContext &ctx);
	void playMusicStinger(const Common::UString &stinger);
	void playVideo(const Common::UString &resRef);


	void setAIArchetype(Aurora::NWScript::FunctionContext &ctx);


	void signalEncounter(Aurora::NWScript::FunctionContext &ctx);

	void showUpgradeScreen(Aurora::NWScript::FunctionContext &ctx);
	void getEncounterActive(Aurora::NWScript::FunctionContext &ctx);
	void setEncounterActive(Aurora::NWScript::FunctionContext &ctx);
	void getEncounterSpawnsMax(Aurora::NWScript::FunctionContext &ctx);
	void setEncounterSpawnsMax(Aurora::NWScript::FunctionContext &ctx);
	void getEncounterSpawnsCurrent(Aurora::NWScript::FunctionContext &ctx);
	void setEncounterSpawnsCurrent(Aurora::NWScript::FunctionContext &ctx);

	void actionWorkbench(Aurora::NWScript::FunctionContext &ctx);


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

	void getNearestObjectToLocation(Aurora::NWScript::FunctionContext &ctx);

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


	void getIsReactionTypeHostile(Aurora::NWScript::FunctionContext &ctx);


	void getIsReactionTypeFriendly(Aurora::NWScript::FunctionContext &ctx);


	void getIsReactionTypeNeutral(Aurora::NWScript::FunctionContext &ctx);





	void getPlotFlag(Aurora::NWScript::FunctionContext &ctx);


	void setPlotFlag(Aurora::NWScript::FunctionContext &ctx);





	void getEffectType(Aurora::NWScript::FunctionContext &ctx);


	void getLastDamager(Aurora::NWScript::FunctionContext &ctx);





	void getName(Aurora::NWScript::FunctionContext &ctx);





	void setIsDestroyable  (Aurora::NWScript::FunctionContext &ctx);


	void getIsInConversation(Aurora::NWScript::FunctionContext &ctx);





	// Quests / Journals


	void addJournalQuestEntry(Aurora::NWScript::FunctionContext &ctx);

	void setMapPinEnabled(Aurora::NWScript::FunctionContext &ctx);

	void setJournalQuestEntryPicture(Aurora::NWScript::FunctionContext &ctx);

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

	void getIsDoorActionPossible(Aurora::NWScript::FunctionContext &ctx);
	void doDoorAction(Aurora::NWScript::FunctionContext &ctx);
	void getBlockingDoor(Aurora::NWScript::FunctionContext &ctx);

	// Items, functions_item.cpp

	void getIdentified(Aurora::NWScript::FunctionContext &ctx);
	void setIdentified(Aurora::NWScript::FunctionContext &ctx);
	void getGoldPieceValue(Aurora::NWScript::FunctionContext &ctx);
	void getItemHasItemProperty(Aurora::NWScript::FunctionContext &ctx);
	void getItemACValue(Aurora::NWScript::FunctionContext &ctx);





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


	void actionCastSpellAtLocation(Aurora::NWScript::FunctionContext &ctx);


	void getHasSpell(Aurora::NWScript::FunctionContext &ctx);
	void getHasSpellEffect(Aurora::NWScript::FunctionContext &ctx);

	void getFirstFactionMember(Aurora::NWScript::FunctionContext &ctx);
	void getNextFactionMember(Aurora::NWScript::FunctionContext &ctx);
	void faceObjectAwayFromObject(Aurora::NWScript::FunctionContext &ctx);
	void getInventoryDisturbType(Aurora::NWScript::FunctionContext &ctx);
	void getInventoryDisturbItem(Aurora::NWScript::FunctionContext &ctx);
	void getItemActivated(Aurora::NWScript::FunctionContext &ctx);
	void getItemActivator(Aurora::NWScript::FunctionContext &ctx);

	void getSpellId(Aurora::NWScript::FunctionContext &ctx);


	void getLastSpellHarmful(Aurora::NWScript::FunctionContext &ctx);


	void getSpellTargetLocation(Aurora::NWScript::FunctionContext &ctx);

	void getSpellTarget(Aurora::NWScript::FunctionContext &ctx);
	void getSpellSaveDC(Aurora::NWScript::FunctionContext &ctx);
	void getSpellCastItem(Aurora::NWScript::FunctionContext &ctx);
	void getMetaMagicFeat(Aurora::NWScript::FunctionContext &ctx);
	void getEffectSpellId(Aurora::NWScript::FunctionContext &ctx);
	void eventSpellCastAt(Aurora::NWScript::FunctionContext &ctx);

	void getLastKiller(Aurora::NWScript::FunctionContext &ctx);
	void getDamageDealtByType(Aurora::NWScript::FunctionContext &ctx);
	void getTotalDamageDealt(Aurora::NWScript::FunctionContext &ctx);
	void getLastAttackResult(Aurora::NWScript::FunctionContext &ctx);
	void getLastWeaponUsed(Aurora::NWScript::FunctionContext &ctx);

	void surrenderToEnemies(Aurora::NWScript::FunctionContext &ctx);
	void actionSurrenderToEnemies(Aurora::NWScript::FunctionContext &ctx);
	void surrenderByFaction(Aurora::NWScript::FunctionContext &ctx);
	void surrenderRetainBuffs(Aurora::NWScript::FunctionContext &ctx);

	void effectImmunity(Aurora::NWScript::FunctionContext &ctx);
	void getIsImmune(Aurora::NWScript::FunctionContext &ctx);
	void effectDamageImmunityIncrease(Aurora::NWScript::FunctionContext &ctx);
	void effectSpellImmunity(Aurora::NWScript::FunctionContext &ctx);
	void effectSleep(Aurora::NWScript::FunctionContext &ctx);
	void effectRegenerate(Aurora::NWScript::FunctionContext &ctx);
	void effectTemporaryForcePoints(Aurora::NWScript::FunctionContext &ctx);

	void effectAreaOfEffect(Aurora::NWScript::FunctionContext &ctx);
	void getFirstInPersistentObject(Aurora::NWScript::FunctionContext &ctx);
	void getNextInPersistentObject(Aurora::NWScript::FunctionContext &ctx);
	void getAreaOfEffectCreator(Aurora::NWScript::FunctionContext &ctx);
	void applyEffectAtLocation(Aurora::NWScript::FunctionContext &ctx);

	void magicalEffect(Aurora::NWScript::FunctionContext &ctx);
	void supernaturalEffect(Aurora::NWScript::FunctionContext &ctx);
	void extraordinaryEffect(Aurora::NWScript::FunctionContext &ctx);

	void effectDamageResistance(Aurora::NWScript::FunctionContext &ctx);
	void effectConcealment(Aurora::NWScript::FunctionContext &ctx);
	void effectAssuredHit(Aurora::NWScript::FunctionContext &ctx);
	void effectAssuredDeflection(Aurora::NWScript::FunctionContext &ctx);
	void effectEntangle(Aurora::NWScript::FunctionContext &ctx);
	void effectForceJump(Aurora::NWScript::FunctionContext &ctx);
	void effectBeam(Aurora::NWScript::FunctionContext &ctx);
	void effectForceResistanceIncrease(Aurora::NWScript::FunctionContext &ctx);
	void effectBodyFuel(Aurora::NWScript::FunctionContext &ctx);
	void effectDamageIncrease(Aurora::NWScript::FunctionContext &ctx);
	void effectHitPointChangeWhenDying(Aurora::NWScript::FunctionContext &ctx);
	void effectDroidStun(Aurora::NWScript::FunctionContext &ctx);
	void effectForceResisted(Aurora::NWScript::FunctionContext &ctx);
	void effectForceFizzle(Aurora::NWScript::FunctionContext &ctx);
	void effectDispelMagicAll(Aurora::NWScript::FunctionContext &ctx);
	void effectBlasterDeflectionIncrease(Aurora::NWScript::FunctionContext &ctx);
	void effectBlasterDeflectionDecrease(Aurora::NWScript::FunctionContext &ctx);

	void speakOneLinerConversation(Aurora::NWScript::FunctionContext &ctx);
	void addToParty(Aurora::NWScript::FunctionContext &ctx);
	void removeFromParty(Aurora::NWScript::FunctionContext &ctx);
	void getIsDay(Aurora::NWScript::FunctionContext &ctx);
	void getIsNight(Aurora::NWScript::FunctionContext &ctx);
	void revealMap(Aurora::NWScript::FunctionContext &ctx);
	void setAILevel(Aurora::NWScript::FunctionContext &ctx);
	void getLastConversation(Aurora::NWScript::FunctionContext &ctx);

	void getIsListening(Aurora::NWScript::FunctionContext &ctx);
	void setListening(Aurora::NWScript::FunctionContext &ctx);
	void setListenPattern(Aurora::NWScript::FunctionContext &ctx);
	void testStringAgainstPattern(Aurora::NWScript::FunctionContext &ctx);
	void getMatchedSubstring(Aurora::NWScript::FunctionContext &ctx);
	void getMatchedSubstringsCount(Aurora::NWScript::FunctionContext &ctx);
	void setAssociateListenPatterns(Aurora::NWScript::FunctionContext &ctx);

	void getFactionWeakestMember(Aurora::NWScript::FunctionContext &ctx);
	void getFactionStrongestMember(Aurora::NWScript::FunctionContext &ctx);
	void getFactionMostDamagedMember(Aurora::NWScript::FunctionContext &ctx);
	void getFactionLeastDamagedMember(Aurora::NWScript::FunctionContext &ctx);
	void getFactionGold(Aurora::NWScript::FunctionContext &ctx);
	void getFactionAverageReputation(Aurora::NWScript::FunctionContext &ctx);
	void getFactionAverageGoodEvilAlignment(Aurora::NWScript::FunctionContext &ctx);
	void getFactionAverageLevel(Aurora::NWScript::FunctionContext &ctx);
	void getFactionAverageXP(Aurora::NWScript::FunctionContext &ctx);
	void getFactionMostFrequentClass(Aurora::NWScript::FunctionContext &ctx);
	void getFactionWorstAC(Aurora::NWScript::FunctionContext &ctx);
	void getFactionBestAC(Aurora::NWScript::FunctionContext &ctx);
	void changeFactionByFaction(Aurora::NWScript::FunctionContext &ctx);

	void getModuleFileName(Aurora::NWScript::FunctionContext &ctx);
	void getReflexAdjustedDamage(Aurora::NWScript::FunctionContext &ctx);
	void getIsPlayableRacialType(Aurora::NWScript::FunctionContext &ctx);
	void getTargetLocation(Aurora::NWScript::FunctionContext &ctx);
	void getIsLinkImmune(Aurora::NWScript::FunctionContext &ctx);


	void getStealthMode(Aurora::NWScript::FunctionContext &ctx);


	void setStealthMode(Aurora::NWScript::FunctionContext &ctx);
	void setLightsaberPowered(Aurora::NWScript::FunctionContext &ctx);


	void getAppearanceType(Aurora::NWScript::FunctionContext &ctx);


	void setAppearanceType(Aurora::NWScript::FunctionContext &ctx);


	void clearAllActions(Aurora::NWScript::FunctionContext &ctx);

	void clearAllEffects(Aurora::NWScript::FunctionContext &ctx);





	void setFacing(Aurora::NWScript::FunctionContext &ctx);


	void actionEquipItem(Aurora::NWScript::FunctionContext &ctx);


	void actionEquipMostDamagingMelee(Aurora::NWScript::FunctionContext &ctx);


	void actionEquipMostDamagingRanged(Aurora::NWScript::FunctionContext &ctx);


	void actionRest(Aurora::NWScript::FunctionContext &ctx);


	void getIsResting(Aurora::NWScript::FunctionContext &ctx);


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


	void actionWait(Aurora::NWScript::FunctionContext &ctx);


	void actionDoCommand(Aurora::NWScript::FunctionContext &ctx);




	void actionInteractObject(Aurora::NWScript::FunctionContext &ctx);


	void actionPauseConversation(Aurora::NWScript::FunctionContext &ctx);


	void actionResumeConversation(Aurora::NWScript::FunctionContext &ctx);




	void getUserActionsPending(Aurora::NWScript::FunctionContext &ctx);


	void noClicksFor(Aurora::NWScript::FunctionContext &ctx);


	void actionGiveItem(Aurora::NWScript::FunctionContext &ctx);


	void actionTakeItem(Aurora::NWScript::FunctionContext &ctx);


	void giveItem(Aurora::NWScript::FunctionContext &ctx);




	void actionUseSkill(Aurora::NWScript::FunctionContext &ctx);


	void actionCircleObject(Aurora::NWScript::FunctionContext &ctx);


	void setFacingPoint(Aurora::NWScript::FunctionContext &ctx);
	void angleToVector(Aurora::NWScript::FunctionContext &ctx);
	void vectorToAngle(Aurora::NWScript::FunctionContext &ctx);

	void actionLockObject(Aurora::NWScript::FunctionContext &ctx);

	void actionUnlockObject(Aurora::NWScript::FunctionContext &ctx);

	void actionCastFakeSpellAtObject(Aurora::NWScript::FunctionContext &ctx);

	void actionCastFakeSpellAtLocation(Aurora::NWScript::FunctionContext &ctx);





	// Sound, functions_sound.cpp





	void musicBackgroundPlay(Aurora::NWScript::FunctionContext &ctx);


	void musicBackgroundStop(Aurora::NWScript::FunctionContext &ctx);


	void musicBackgroundChangeDay(Aurora::NWScript::FunctionContext &ctx);


	void musicBackgroundChangeNight(Aurora::NWScript::FunctionContext &ctx);





	void musicBackgroundGetDayTrack(Aurora::NWScript::FunctionContext &ctx);


	void musicBackgroundGetNightTrack(Aurora::NWScript::FunctionContext &ctx);





	void musicBattlePlay(Aurora::NWScript::FunctionContext &ctx);


	void musicBattleStop(Aurora::NWScript::FunctionContext &ctx);


	void musicBattleChange(Aurora::NWScript::FunctionContext &ctx);







	void soundObjectPlay(Aurora::NWScript::FunctionContext &ctx);
	void soundObjectStop(Aurora::NWScript::FunctionContext &ctx);


	void playRumblePattern(Aurora::NWScript::FunctionContext &ctx);


	void stopRumblePattern(Aurora::NWScript::FunctionContext &ctx);





	// Movies, functions_movie.cpp





	void playMovie(Aurora::NWScript::FunctionContext &ctx);

	void isMoviePlaying(Aurora::NWScript::FunctionContext &ctx);

	void queueMovie(Aurora::NWScript::FunctionContext &ctx);

	void playMovieQueue(Aurora::NWScript::FunctionContext &ctx);
	void enableVideoEffect(Aurora::NWScript::FunctionContext &ctx);
	void disableVideoEffect(Aurora::NWScript::FunctionContext &ctx);
	void setDialogPlaceableCamera(Aurora::NWScript::FunctionContext &ctx);





	// Creatures, functions_creatures.cpp





	void getGender(Aurora::NWScript::FunctionContext &ctx);


	void getLevelByClass(Aurora::NWScript::FunctionContext &ctx);


	void changeToJedi(Aurora::NWScript::FunctionContext &ctx);





	void getGoodEvilValue(Aurora::NWScript::FunctionContext &ctx);


	void getAlignmentGoodEvil(Aurora::NWScript::FunctionContext &ctx);


	void adjustAlignment(Aurora::NWScript::FunctionContext &ctx);





	void getLevelByPosition(Aurora::NWScript::FunctionContext &ctx);


	void getClassByPosition(Aurora::NWScript::FunctionContext &ctx);





	void getRacialType(Aurora::NWScript::FunctionContext &ctx);


	void getSubRace(Aurora::NWScript::FunctionContext &ctx);





	void getHasFeat(Aurora::NWScript::FunctionContext &ctx);


	void getHasSkill(Aurora::NWScript::FunctionContext &ctx);


	void getSkillRank(Aurora::NWScript::FunctionContext &ctx);


	void getIsSkillSuccessful(Aurora::NWScript::FunctionContext &ctx);


	void getIsAbilitySuccessful(Aurora::NWScript::FunctionContext &ctx);


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

	void getGoingToBeAttackedBy(Aurora::NWScript::FunctionContext &ctx);

	void getAttemptedSpellTarget(Aurora::NWScript::FunctionContext &ctx);


	void getIsInCombat(Aurora::NWScript::FunctionContext &ctx);

	void getLastAttackMode(Aurora::NWScript::FunctionContext &ctx);

	void getLastHostileActor(Aurora::NWScript::FunctionContext &ctx);

	void getLastHostileTarget(Aurora::NWScript::FunctionContext &ctx);

	void getLastAttackAction(Aurora::NWScript::FunctionContext &ctx);

	void getWasForcePowerSuccessful(Aurora::NWScript::FunctionContext &ctx);





	void effectHeal(Aurora::NWScript::FunctionContext &ctx);


	void effectDamage(Aurora::NWScript::FunctionContext &ctx);


	void effectVisualEffect(Aurora::NWScript::FunctionContext &ctx);


	void effectACIncrease(Aurora::NWScript::FunctionContext &ctx);
	void effectSavingThrowIncrease(Aurora::NWScript::FunctionContext &ctx);
	void effectDamageReduction(Aurora::NWScript::FunctionContext &ctx);
	void effectInvisibility(Aurora::NWScript::FunctionContext &ctx);
	void effectPoison(Aurora::NWScript::FunctionContext &ctx);
	void effectSeeInvisible(Aurora::NWScript::FunctionContext &ctx);
	void effectAbilityDecrease(Aurora::NWScript::FunctionContext &ctx);
	void effectAttackDecrease(Aurora::NWScript::FunctionContext &ctx);
	void effectDamageDecrease(Aurora::NWScript::FunctionContext &ctx);
	void effectDamageImmunityDecrease(Aurora::NWScript::FunctionContext &ctx);
	void effectACDecrease(Aurora::NWScript::FunctionContext &ctx);
	void effectMovementSpeedDecrease(Aurora::NWScript::FunctionContext &ctx);
	void effectSavingThrowDecrease(Aurora::NWScript::FunctionContext &ctx);
	void effectSkillDecrease(Aurora::NWScript::FunctionContext &ctx);
	void effectForceResistanceDecrease(Aurora::NWScript::FunctionContext &ctx);
	void getLastForcePowerUsed(Aurora::NWScript::FunctionContext &ctx);

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

	void effectModifyAttacks(Aurora::NWScript::FunctionContext &ctx);

	void effectLightsaberThrow(Aurora::NWScript::FunctionContext &ctx);

	void effectWhirlWind(Aurora::NWScript::FunctionContext &ctx);


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

	void getGlobalLocation(Aurora::NWScript::FunctionContext &ctx);

	void setGlobalLocation(Aurora::NWScript::FunctionContext &ctx);





	// Local variables, functions_local.cpp





	void getLocalInt(Aurora::NWScript::FunctionContext &ctx);


	void setLocalInt(Aurora::NWScript::FunctionContext &ctx);


	void getLocalFloat(Aurora::NWScript::FunctionContext &ctx);


	void setLocalFloat(Aurora::NWScript::FunctionContext &ctx);


	void getLocalString(Aurora::NWScript::FunctionContext &ctx);


	void setLocalString(Aurora::NWScript::FunctionContext &ctx);





	void getLocalBoolean(Aurora::NWScript::FunctionContext &ctx);


	void setLocalBoolean(Aurora::NWScript::FunctionContext &ctx);


	void getLocalNumber(Aurora::NWScript::FunctionContext &ctx);


	void setLocalNumber(Aurora::NWScript::FunctionContext &ctx);





	// Minigames, functions_minigame.cpp


	void playPazaak(Aurora::NWScript::FunctionContext &ctx);


	void getLastPazaakResult(Aurora::NWScript::FunctionContext &ctx);


	void swmgSetLateralAccelerationPerSecond(Aurora::NWScript::FunctionContext &ctx);


	void swmgGetLateralAccelerationPerSecond(Aurora::NWScript::FunctionContext &ctx);


	void swmgSetPlayerSpeed(Aurora::NWScript::FunctionContext &ctx);


	void swmgGetPlayerSpeed(Aurora::NWScript::FunctionContext &ctx);


	void swmgSetPlayerMaxSpeed(Aurora::NWScript::FunctionContext &ctx);


	void swmgGetPlayerMaxSpeed(Aurora::NWScript::FunctionContext &ctx);


	void swmgOnObstacleHit(Aurora::NWScript::FunctionContext &ctx);


	void swmgGetLastEvent(Aurora::NWScript::FunctionContext &ctx);


	void swmgSetSpeedBlurEffect(Aurora::NWScript::FunctionContext &ctx);

	void swmgGetPlayerTunnelPos(Aurora::NWScript::FunctionContext &ctx);
	void swmgSetPlayerTunnelPos(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetPlayerTunnelNeg(Aurora::NWScript::FunctionContext &ctx);
	void swmgSetPlayerTunnelNeg(Aurora::NWScript::FunctionContext &ctx);

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











	void setGoodEvilValue(Aurora::NWScript::FunctionContext &ctx);





	// Saving throws, functions_creatures.cpp





	void fortitudeSave(Aurora::NWScript::FunctionContext &ctx);


	void reflexSave(Aurora::NWScript::FunctionContext &ctx);


	void willSave(Aurora::NWScript::FunctionContext &ctx);

	void getFortitudeSavingThrow(Aurora::NWScript::FunctionContext &ctx);

	void getWillSavingThrow(Aurora::NWScript::FunctionContext &ctx);

	void getReflexSavingThrow(Aurora::NWScript::FunctionContext &ctx);





	// Experience, functions_creatures.cpp





	void giveXPToCreature(Aurora::NWScript::FunctionContext &ctx);


	void setXP(Aurora::NWScript::FunctionContext &ctx);


	void getXP(Aurora::NWScript::FunctionContext &ctx);


	void givePlotXP(Aurora::NWScript::FunctionContext &ctx);


	void awardStealthXP(Aurora::NWScript::FunctionContext &ctx);


	void getStealthXPEnabled(Aurora::NWScript::FunctionContext &ctx);


	void setStealthXPEnabled(Aurora::NWScript::FunctionContext &ctx);


	void getStealthXPDecrement(Aurora::NWScript::FunctionContext &ctx);


	void setStealthXPDecrement(Aurora::NWScript::FunctionContext &ctx);





	// Level-up / store GUI, functions_module.cpp





	void showLevelUpGUI(Aurora::NWScript::FunctionContext &ctx);


	void openStore(Aurora::NWScript::FunctionContext &ctx);


	void popUpGUIPanel(Aurora::NWScript::FunctionContext &ctx);


	void popUpDeathGUIPanel(Aurora::NWScript::FunctionContext &ctx);





	// Misc, functions_object.cpp / functions_module.cpp





	void playSound(Aurora::NWScript::FunctionContext &ctx);
	void ambientSoundPlay(Aurora::NWScript::FunctionContext &ctx);
	void ambientSoundStop(Aurora::NWScript::FunctionContext &ctx);
	void ambientSoundChangeDay(Aurora::NWScript::FunctionContext &ctx);
	void ambientSoundChangeNight(Aurora::NWScript::FunctionContext &ctx);
	void ambientSoundSetDayVolume(Aurora::NWScript::FunctionContext &ctx);
	void ambientSoundSetNightVolume(Aurora::NWScript::FunctionContext &ctx);

	void displayFeedBackText(Aurora::NWScript::FunctionContext &ctx);

	void barkString(Aurora::NWScript::FunctionContext &ctx);


	void actionBarkString(Aurora::NWScript::FunctionContext &ctx);


	void actionUnequipItem(Aurora::NWScript::FunctionContext &ctx);


	void setAreaTransitionBMP(Aurora::NWScript::FunctionContext &ctx);


	void getModuleName(Aurora::NWScript::FunctionContext &ctx);


	void getLoadFromSaveGame(Aurora::NWScript::FunctionContext &ctx);


	void endGame(Aurora::NWScript::FunctionContext &ctx);


	void getTransitionTarget(Aurora::NWScript::FunctionContext &ctx);


	void getLockUnlockDC(Aurora::NWScript::FunctionContext &ctx);


	void getLockKeyTag(Aurora::NWScript::FunctionContext &ctx);


	void getLockRequiredSkill(Aurora::NWScript::FunctionContext &ctx);

	void doSinglePlayerAutoSave(Aurora::NWScript::FunctionContext &ctx);

	void setTutorialWindowsEnabled(Aurora::NWScript::FunctionContext &ctx);

	void showTutorialWindow(Aurora::NWScript::FunctionContext &ctx);

	void startCreditSequence(Aurora::NWScript::FunctionContext &ctx);

	void isCreditSequenceInProgress(Aurora::NWScript::FunctionContext &ctx);

	void saveNPCState(Aurora::NWScript::FunctionContext &ctx);





	void floatingTextStringOnCreature(Aurora::NWScript::FunctionContext &ctx);


	void floatingTextStrRefOnCreature(Aurora::NWScript::FunctionContext &ctx);





	// Galaxy map, functions_module.cpp





	void showGalaxyMap(Aurora::NWScript::FunctionContext &ctx);


	void setPlanetSelectable(Aurora::NWScript::FunctionContext &ctx);


	void getPlanetSelectable(Aurora::NWScript::FunctionContext &ctx);


	void setPlanetAvailable(Aurora::NWScript::FunctionContext &ctx);


	void getPlanetAvailable(Aurora::NWScript::FunctionContext &ctx);


	void getSelectedPlanet(Aurora::NWScript::FunctionContext &ctx);





	void addJournalWorldEntry(Aurora::NWScript::FunctionContext &ctx);


	void addJournalWorldEntryStrref(Aurora::NWScript::FunctionContext &ctx);


	void deleteJournalWorldEntry(Aurora::NWScript::FunctionContext &ctx);


	void deleteJournalWorldEntryStrref(Aurora::NWScript::FunctionContext &ctx);


	void deleteJournalWorldAllEntries(Aurora::NWScript::FunctionContext &ctx);





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


	void getWeaponRanged(Aurora::NWScript::FunctionContext &ctx);









	void getInfluence(Aurora::NWScript::FunctionContext &ctx);


	void setInfluence(Aurora::NWScript::FunctionContext &ctx);









	void getSkillPointBonus(Aurora::NWScript::FunctionContext &ctx);


	void setSkillPointBonus(Aurora::NWScript::FunctionContext &ctx);





	void setPartyAIStyle(Aurora::NWScript::FunctionContext &ctx);





	void effectForcePushed(Aurora::NWScript::FunctionContext &ctx);


	void effectForcePushTargeted(Aurora::NWScript::FunctionContext &ctx);


	void effectForceDrain(Aurora::NWScript::FunctionContext &ctx);


	void effectForceShield(Aurora::NWScript::FunctionContext &ctx);


	void effectHealForcePoints(Aurora::NWScript::FunctionContext &ctx);


	void effectDamageForcePoints(Aurora::NWScript::FunctionContext &ctx);







	void getTrapDisarmable(Aurora::NWScript::FunctionContext &ctx);

	void getTrapDetectable(Aurora::NWScript::FunctionContext &ctx);

	void getTrapDetectedBy(Aurora::NWScript::FunctionContext &ctx);

	void getTrapFlagged(Aurora::NWScript::FunctionContext &ctx);

	void getTrapBaseType(Aurora::NWScript::FunctionContext &ctx);

	void getTrapOneShot(Aurora::NWScript::FunctionContext &ctx);

	void getTrapCreator(Aurora::NWScript::FunctionContext &ctx);

	void getTrapKeyTag(Aurora::NWScript::FunctionContext &ctx);

	void getTrapDisarmDC(Aurora::NWScript::FunctionContext &ctx);

	void getTrapDetectDC(Aurora::NWScript::FunctionContext &ctx);

	void setTrapDetectedBy(Aurora::NWScript::FunctionContext &ctx);
	void getIsTrapped(Aurora::NWScript::FunctionContext &ctx);
	void setTrapDisabled(Aurora::NWScript::FunctionContext &ctx);

	void getLockKeyRequired(Aurora::NWScript::FunctionContext &ctx);

	void getLockLockable(Aurora::NWScript::FunctionContext &ctx);

	void getLockLockDC(Aurora::NWScript::FunctionContext &ctx);

	void getPCLevellingUp(Aurora::NWScript::FunctionContext &ctx);

	void setPlaceableIllumination(Aurora::NWScript::FunctionContext &ctx);

	void getPlaceableIllumination(Aurora::NWScript::FunctionContext &ctx);

	void setEffectIcon(Aurora::NWScript::FunctionContext &ctx);

	void getFactionLeader(Aurora::NWScript::FunctionContext &ctx);

	void setNPCAIStyle(Aurora::NWScript::FunctionContext &ctx);

	void setNPCSelectability(Aurora::NWScript::FunctionContext &ctx);

	void getNPCSelectability(Aurora::NWScript::FunctionContext &ctx);

	void getIsDebilitated(Aurora::NWScript::FunctionContext &ctx);

	void resetDialogState(Aurora::NWScript::FunctionContext &ctx);

	void holdWorldFadeInForDialog(Aurora::NWScript::FunctionContext &ctx);

	void getFirstAttacker(Aurora::NWScript::FunctionContext &ctx);

	void getNextAttacker(Aurora::NWScript::FunctionContext &ctx);

	void playRoomAnimation(Aurora::NWScript::FunctionContext &ctx);

	void effectPsychicStatic(Aurora::NWScript::FunctionContext &ctx);

	void playVisualAreaEffect(Aurora::NWScript::FunctionContext &ctx);

	void soundObjectFadeAndStop(Aurora::NWScript::FunctionContext &ctx);

	void aurPostString(Aurora::NWScript::FunctionContext &ctx);

	void swmgGetLastBulletHitPart(Aurora::NWScript::FunctionContext &ctx);

	void swmgIsGunBankTargetting(Aurora::NWScript::FunctionContext &ctx);

	void swmgGetPlayerOffset(Aurora::NWScript::FunctionContext &ctx);

	void swmgSetPlayerOffset(Aurora::NWScript::FunctionContext &ctx);

	void swmgGetEnemyCount(Aurora::NWScript::FunctionContext &ctx);

	void swmgGetEnemy(Aurora::NWScript::FunctionContext &ctx);

	// Phase 3 SWMG de-stubbing
	void swmgGetLastEventModelName(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetObjectByName(Aurora::NWScript::FunctionContext &ctx);
	void swmgPlayAnimation(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetLastBulletHitDamage(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetLastBulletHitTarget(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetLastBulletHitShooter(Aurora::NWScript::FunctionContext &ctx);
	void swmgAdjustFollowerHitPoints(Aurora::NWScript::FunctionContext &ctx);
	void swmgOnBulletHit(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetLastFollowerHit(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetLastObstacleHit(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetLastBulletFiredDamage(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetLastBulletFiredTarget(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetObjectName(Aurora::NWScript::FunctionContext &ctx);
	void swmgOnDeath(Aurora::NWScript::FunctionContext &ctx);
	void swmgIsFollower(Aurora::NWScript::FunctionContext &ctx);
	void swmgIsPlayer(Aurora::NWScript::FunctionContext &ctx);
	void swmgIsEnemy(Aurora::NWScript::FunctionContext &ctx);
	void swmgIsTrigger(Aurora::NWScript::FunctionContext &ctx);
	void swmgIsObstacle(Aurora::NWScript::FunctionContext &ctx);
	void swmgSetFollowerHitPoints(Aurora::NWScript::FunctionContext &ctx);
	void swmgOnDamage(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetLastHPChange(Aurora::NWScript::FunctionContext &ctx);
	void swmgRemoveAnimation(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetCameraNearClip(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetCameraFarClip(Aurora::NWScript::FunctionContext &ctx);
	void swmgSetCameraClip(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetPlayer(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetObstacleCount(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetObstacle(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetHitPoints(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetMaxHitPoints(Aurora::NWScript::FunctionContext &ctx);
	void swmgSetMaxHitPoints(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetSphereRadius(Aurora::NWScript::FunctionContext &ctx);
	void swmgSetSphereRadius(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetNumLoops(Aurora::NWScript::FunctionContext &ctx);
	void swmgSetNumLoops(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetPosition(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetGunBankCount(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetGunBankBulletModel(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetGunBankGunModel(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetGunBankDamage(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetGunBankTimeBetweenShots(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetGunBankLifespan(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetGunBankSpeed(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetGunBankTarget(Aurora::NWScript::FunctionContext &ctx);
	void swmgSetGunBankBulletModel(Aurora::NWScript::FunctionContext &ctx);
	void swmgSetGunBankGunModel(Aurora::NWScript::FunctionContext &ctx);
	void swmgSetGunBankDamage(Aurora::NWScript::FunctionContext &ctx);
	void swmgSetGunBankTimeBetweenShots(Aurora::NWScript::FunctionContext &ctx);
	void swmgSetGunBankLifespan(Aurora::NWScript::FunctionContext &ctx);
	void swmgSetGunBankSpeed(Aurora::NWScript::FunctionContext &ctx);
	void swmgSetGunBankTarget(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetPlayerInvincibility(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetPlayerMinSpeed(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetPlayerAccelerationPerSecond(Aurora::NWScript::FunctionContext &ctx);
	void swmgSetPlayerInvincibility(Aurora::NWScript::FunctionContext &ctx);
	void swmgSetPlayerMinSpeed(Aurora::NWScript::FunctionContext &ctx);
	void swmgSetPlayerAccelerationPerSecond(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetPlayerOrigin(Aurora::NWScript::FunctionContext &ctx);
	void swmgSetPlayerOrigin(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetGunBankHorizontalSpread(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetGunBankVerticalSpread(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetGunBankSensingRadius(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetGunBankInaccuracy(Aurora::NWScript::FunctionContext &ctx);
	void swmgSetGunBankHorizontalSpread(Aurora::NWScript::FunctionContext &ctx);
	void swmgSetGunBankVerticalSpread(Aurora::NWScript::FunctionContext &ctx);
	void swmgSetGunBankSensingRadius(Aurora::NWScript::FunctionContext &ctx);
	void swmgSetGunBankInaccuracy(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetIsInvulnerable(Aurora::NWScript::FunctionContext &ctx);
	void swmgStartInvulnerability(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetSoundFrequency(Aurora::NWScript::FunctionContext &ctx);
	void swmgSetSoundFrequency(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetSoundFrequencyIsRandom(Aurora::NWScript::FunctionContext &ctx);
	void swmgSetSoundFrequencyIsRandom(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetSoundVolume(Aurora::NWScript::FunctionContext &ctx);
	void swmgSetSoundVolume(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetPlayerTunnelInfinite(Aurora::NWScript::FunctionContext &ctx);
	void swmgSetPlayerTunnelInfinite(Aurora::NWScript::FunctionContext &ctx);

	// KotOR II SWMG extras
	void swmgGetSwoopUpgrade(Aurora::NWScript::FunctionContext &ctx);
	void swmgGetTrackPosition(Aurora::NWScript::FunctionContext &ctx);
	void swmgSetFollowerPosition(Aurora::NWScript::FunctionContext &ctx);
	void swmgDestroyMiniGameObject(Aurora::NWScript::FunctionContext &ctx);
	void swmgSetJumpSpeed(Aurora::NWScript::FunctionContext &ctx);
	void swmgPlayerApplyForce(Aurora::NWScript::FunctionContext &ctx);

	// Phase 3 remaining generic natives
	void soundObjectSetFixedVariance(Aurora::NWScript::FunctionContext &ctx);
	void actionForceFollowObject(Aurora::NWScript::FunctionContext &ctx);
	void soundObjectGetFixedVariance(Aurora::NWScript::FunctionContext &ctx);
	void randomName(Aurora::NWScript::FunctionContext &ctx);
	void setItemNonEquippable(Aurora::NWScript::FunctionContext &ctx);
	void getButtonMashCheck(Aurora::NWScript::FunctionContext &ctx);
	void setButtonMashCheck(Aurora::NWScript::FunctionContext &ctx);
	void getLastPlayerDied(Aurora::NWScript::FunctionContext &ctx);
	void getModuleItemLost(Aurora::NWScript::FunctionContext &ctx);
	void getModuleItemLostBy(Aurora::NWScript::FunctionContext &ctx);
	void eventConversation(Aurora::NWScript::FunctionContext &ctx);
	void setEncounterDifficulty(Aurora::NWScript::FunctionContext &ctx);
	void getEncounterDifficulty(Aurora::NWScript::FunctionContext &ctx);
	void actionExamine(Aurora::NWScript::FunctionContext &ctx);
	void getLastCommand(Aurora::NWScript::FunctionContext &ctx);
	void getLastAssociateCommand(Aurora::NWScript::FunctionContext &ctx);
	void getLastDisarmed(Aurora::NWScript::FunctionContext &ctx);
	void getLastDisturbed(Aurora::NWScript::FunctionContext &ctx);
	void getLastLocked(Aurora::NWScript::FunctionContext &ctx);
	void getLastUnlocked(Aurora::NWScript::FunctionContext &ctx);
	void versusAlignmentEffect(Aurora::NWScript::FunctionContext &ctx);
	void versusRacialTypeEffect(Aurora::NWScript::FunctionContext &ctx);
	void versusTrapEffect(Aurora::NWScript::FunctionContext &ctx);
	void addMultiClass(Aurora::NWScript::FunctionContext &ctx);
	void actionEquipMostEffectiveArmor(Aurora::NWScript::FunctionContext &ctx);
	void getIsDawn(Aurora::NWScript::FunctionContext &ctx);
	void getIsDusk(Aurora::NWScript::FunctionContext &ctx);
	void getIsEncounterCreature(Aurora::NWScript::FunctionContext &ctx);
	void getLastPlayerDying(Aurora::NWScript::FunctionContext &ctx);
	void getStartingLocation(Aurora::NWScript::FunctionContext &ctx);
	void soundObjectSetVolume(Aurora::NWScript::FunctionContext &ctx);
	void soundObjectSetPosition(Aurora::NWScript::FunctionContext &ctx);
	void getLastRespawnButtonPresser(Aurora::NWScript::FunctionContext &ctx);
	void getIsWeaponEffective(Aurora::NWScript::FunctionContext &ctx);
	void eventActivateItem(Aurora::NWScript::FunctionContext &ctx);
	void musicBackgroundSetDelay(Aurora::NWScript::FunctionContext &ctx);
	void getItemActivatedTargetLocation(Aurora::NWScript::FunctionContext &ctx);
	void getItemActivatedTarget(Aurora::NWScript::FunctionContext &ctx);
	void effectDisguise(Aurora::NWScript::FunctionContext &ctx);
	void getMaxStealthXP(Aurora::NWScript::FunctionContext &ctx);
	void effectTrueSeeing(Aurora::NWScript::FunctionContext &ctx);
	void effectTimeStop(Aurora::NWScript::FunctionContext &ctx);
	void setMaxStealthXP(Aurora::NWScript::FunctionContext &ctx);
	void effectHorrified(Aurora::NWScript::FunctionContext &ctx);
	void effectSpellLevelAbsorption(Aurora::NWScript::FunctionContext &ctx);
	void effectDispelMagicBest(Aurora::NWScript::FunctionContext &ctx);
	void getCurrentStealthXP(Aurora::NWScript::FunctionContext &ctx);
	void getNumStackedItems(Aurora::NWScript::FunctionContext &ctx);
	void effectMissChance(Aurora::NWScript::FunctionContext &ctx);
	void setCurrentStealthXP(Aurora::NWScript::FunctionContext &ctx);
	void getCreatureSize(Aurora::NWScript::FunctionContext &ctx);
	void getLastTrapDetected(Aurora::NWScript::FunctionContext &ctx);
	void effectDamageShield(Aurora::NWScript::FunctionContext &ctx);
	void getNearestTrapToObject(Aurora::NWScript::FunctionContext &ctx);
	void getAttemptedMovementTarget(Aurora::NWScript::FunctionContext &ctx);
	void getBlockingCreature(Aurora::NWScript::FunctionContext &ctx);
	void getChallengeRating(Aurora::NWScript::FunctionContext &ctx);
	void getFoundEnemyCreature(Aurora::NWScript::FunctionContext &ctx);
	void getMovementRate(Aurora::NWScript::FunctionContext &ctx);
	void duplicateHeadAppearance(Aurora::NWScript::FunctionContext &ctx);
	void getIsPlaceableObjectActionPossible(Aurora::NWScript::FunctionContext &ctx);
	void doPlaceableObjectAction(Aurora::NWScript::FunctionContext &ctx);
	void exportAllCharacters(Aurora::NWScript::FunctionContext &ctx);
	void getCreatureMovmentType(Aurora::NWScript::FunctionContext &ctx);
	void musicBackgroundGetBattleTrack(Aurora::NWScript::FunctionContext &ctx);
	void getHasInventory(Aurora::NWScript::FunctionContext &ctx);
	void getStrRefSoundDuration(Aurora::NWScript::FunctionContext &ctx);
	void soundObjectGetPitchVariance(Aurora::NWScript::FunctionContext &ctx);
	void soundObjectSetPitchVariance(Aurora::NWScript::FunctionContext &ctx);
	void soundObjectGetVolume(Aurora::NWScript::FunctionContext &ctx);
	void setFormation(Aurora::NWScript::FunctionContext &ctx);
	void setForcePowerUnsuccessful(Aurora::NWScript::FunctionContext &ctx);
	void setAreaFogColor(Aurora::NWScript::FunctionContext &ctx);
	void changeItemCost(Aurora::NWScript::FunctionContext &ctx);
	void getIsLiveContentAvailable(Aurora::NWScript::FunctionContext &ctx);
	void getIsPoisoned(Aurora::NWScript::FunctionContext &ctx);
	void cancelPostDialogCharacterSwitch(Aurora::NWScript::FunctionContext &ctx);
	void suppressStatusSummaryEntry(Aurora::NWScript::FunctionContext &ctx);
	void getCheatCode(Aurora::NWScript::FunctionContext &ctx);
	void setMusicVolume(Aurora::NWScript::FunctionContext &ctx);
	void createItemOnFloor(Aurora::NWScript::FunctionContext &ctx);
	void setAvailableNPCId(Aurora::NWScript::FunctionContext &ctx);
	void yavinHackCloseDoor(Aurora::NWScript::FunctionContext &ctx);




	void getLastItemEquipped(Aurora::NWScript::FunctionContext &ctx);
	void getSubScreenID(Aurora::NWScript::FunctionContext &ctx);
	void getCasterLevel(Aurora::NWScript::FunctionContext &ctx);
	void getFirstEffect(Aurora::NWScript::FunctionContext &ctx);
	void getNextEffect(Aurora::NWScript::FunctionContext &ctx);
	void removeEffect(Aurora::NWScript::FunctionContext &ctx);
	void getIsEffectValid(Aurora::NWScript::FunctionContext &ctx);
	void getEffectDurationType(Aurora::NWScript::FunctionContext &ctx);
	void getEffectSubType(Aurora::NWScript::FunctionContext &ctx);
	void getEffectCreator(Aurora::NWScript::FunctionContext &ctx);
	void effectConfused(Aurora::NWScript::FunctionContext &ctx);
	void effectFrightened(Aurora::NWScript::FunctionContext &ctx);
	void effectChoke(Aurora::NWScript::FunctionContext &ctx);
	void resistForce(Aurora::NWScript::FunctionContext &ctx);
	void getLastSpellCaster(Aurora::NWScript::FunctionContext &ctx);
	void getLastSpell(Aurora::NWScript::FunctionContext &ctx);

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

	// State for GetFirstFactionMember / GetNextFactionMember
	Aurora::NWScript::Object *_factionIterRef { nullptr };
	std::vector<Object *> _factionIterObjects;
	size_t _factionIterIndex { 0 };

	// State for GetFirstInPersistentObject / GetNextInPersistentObject
	std::vector<Object *> _persistentIterObjects;
	size_t _persistentIterIndex { 0 };

	// Matched substrings from TestStringAgainstPattern
	std::vector<Common::UString> _matchedSubstrings;

	// Context state for inventory-disturb and item-activation events
	int _lastInventoryDisturbType { 0 };
	Aurora::NWScript::Object *_lastInventoryDisturbItem { nullptr };
	Aurora::NWScript::Object *_lastItemActivated { nullptr };
	Aurora::NWScript::Object *_lastItemActivator { nullptr };

public:
	void grantFeat(Aurora::NWScript::FunctionContext &ctx);

	void grantSpell(Aurora::NWScript::FunctionContext &ctx);
private:



	// Custom token store (SetCustomToken / GetCustomToken replacements in dialogue)

	std::map<int, Common::UString> _customTokens;

};



} // End of namespace KotORBase



} // End of namespace Engines



#endif // ENGINES_KOTORBASE_SCRIPT_FUNCTIONS_H

