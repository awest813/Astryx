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
 *  The context needed to run a module in KotOR games.
 */

#ifndef ENGINES_KOTORBASE_MODULE_H
#define ENGINES_KOTORBASE_MODULE_H

#include <list>
#include <map>
#include <set>
#include <utility>

#include <memory>
#include "src/common/ustring.h"
#include "src/common/changeid.h"
#include "src/common/configman.h"

#include "src/aurora/ifofile.h"

#include "src/aurora/nwscript/objectref.h"

#include "src/graphics/aurora/fadequad.h"

#include "src/events/types.h"

#include "src/engines/kotorbase/object.h"
#include "src/engines/kotorbase/objectcontainer.h"
#include "src/engines/kotorbase/savedgame.h"
#include "src/engines/kotorbase/partyleader.h"
#include "src/engines/kotorbase/partycontroller.h"
#include "src/engines/kotorbase/cameracontroller.h"
#include "src/engines/kotorbase/creatureinfo.h"
#include "src/engines/kotorbase/round.h"

#include "src/engines/kotorbase/gui/ingame.h"
#include "src/engines/kotorbase/gui/dialog.h"
#include "src/engines/kotorbase/gui/partyselection.h"

namespace Engines {

class Console;

namespace KotORBase {

class Area;
class Creature;
class LoadScreen;
class Game;

class Module : public Object, public ObjectContainer {
public:
	Module(Game &game, ::Engines::Console &console);
	virtual ~Module();

	/** Clear the whole context. */
	void clear();

	// Module management

	/** Is a module currently loaded and ready to run? */
	bool isLoaded() const;
	/** Is a module currently running? */
	bool isRunning() const;

	/** Load a module. */
	void load(const Common::UString &module,
	          const Common::UString &entryLocation = "",
	          ObjectType entryLocationType = kObjectTypeAll);

	/** Load texture pack used by the module. */
	void loadTexturePack();
	/** Use this character generation information to create the player character. */
	void usePC(const CharacterGenerationInfo &info);
	/** Exit the currently running module. */
	void exit();

	/** Change the module to a new one immediately. */
	void replaceModule(const Common::UString &module);

	// Information about the current module

	/** Return the IFO of the currently loaded module. */
	const Aurora::IFOFile &getIFO() const;
	/** Return the module's name. */
	const Common::UString &getName() const;

	// Elements of the current module

	/** Return the area the PC is currently in. */
	Area *getCurrentArea();
	/** Return the player character. */
	Creature *getPC();
	/** Return the game context. */
	Game &getGame() const;
	/** Return a map between surface type and whether it is walkable. */
	const std::vector<bool> &getWalkableSurfaces() const;
	/** Return the fade quad. */
	Graphics::Aurora::FadeQuad &getFadeQuad();

	void removeObject(Object &object);

	// Interact with the current module

	/** Show the ingame main menu. */
	void showMenu();

	/** Show the galaxy map menu. */
	virtual void showGalaxyMap();

	/** Show the workbench menu. */
	virtual void showWorkbench();

	/** Show the journal menu. */
	virtual void showJournal();

	// Scripting

	int getRunScriptVar() const;
	int getUserDefinedEventNumber() const;

	void setRunScriptVar(int runScriptVar);

	void delayScript(const Common::UString &script,
	                 const Aurora::NWScript::ScriptState &state,
	                 Aurora::NWScript::Object *owner, Aurora::NWScript::Object *triggerer,
	                 uint32_t delay);

	void signalUserDefinedEvent(Object *owner, int number);
	
	/** Transition the camera to focus on a target object/waypoint. */
	virtual void cameraTransitionToTarget(const Common::UString &target, float duration);

	/** Signal an encounter choreography. */
	virtual void signalEncounter(const Common::UString &id);

	/** Play an intro/cinematic movie. */
	virtual void playMovie(const Common::UString &resRef);

	// Persistence
	std::shared_ptr<Aurora::GFF3File> getAreaObjectSave(const Common::UString &key);
	void setAreaObjectSave(const Common::UString &key, std::shared_ptr<Aurora::GFF3File> state);

	// Journal

	// Journal
	void addJournalQuestEntry(const Common::UString &quest, uint32_t state);
	const std::map<Common::UString, uint32_t> &getJournal() const { return _journal; }

	// Party transitions

	/** Move the current party to a specified location within the current area. */
	void moveParty(float x, float y, float z);
	/** Move the current party to a specified location and facing within the current area. */
	void moveParty(float x, float y, float z, float angle);
	/** Move the current party to a specified object within a specified module. */
	void moveParty(const Common::UString &module, const Common::UString &object, ObjectType type = kObjectTypeAll);
	/** Notify the module that the party leader was moved. */
	void movedPartyLeader();

	/** Shake the camera. */
	void shakeCamera(float duration, float intensity);

	/** Play a movie. */
	void playMovie(const Common::UString &resRef);

	/** Transition the camera to a target object by tag. */
	void cameraTransitionToTarget(const Common::UString &tag, float duration);

	// Party management

	/** Get the party leader. */
	Creature *getPartyLeader() const;
	/** Get the party member at a given index (0 is always the party leader). */
	Creature *getPartyMemberByIndex(int index) const;
	/** Get the count of party members including the leader. */
	int getPartyMemberCount() const;

	/** Is a specified creature a party member? */
	bool isObjectPartyMember(Creature *creature) const;
	/** Is a NPC slot in the active party? */
	bool isNPCPartyMember(int npc) const;
	/** Is a NPC in the list of available party members? */
	bool isAvailableCreature(int npc) const;
	/** Is the player in solo mode? */
	bool isSoloMode() const;

	/** Set which party member should be the controlled character. */
	void setPartyLeader(int npc);
	/** Set which party member should be the controlled character. */
	void setPartyLeaderByIndex(int index);
	/** Enable or disable solo mode. */
	void setSoloMode(bool enabled);

	/** Show the party selection GUI. */
	void showPartySelectionGUI(int forceNPC1, int forceNPC2);
	/** Add a NPC to the list of available party members using a template. */
	void addAvailableNPCByTemplate(int npc, const Common::UString &templ);
	/** Add an existing creature object to the available party roster. */
	void addAvailableNPCByObject(int npc, Creature *creature);
	/** Remove a NPC from the available party roster. */
	void removeAvailableNPC(int npc);
	/** Spawn an available NPC into the current area and add them to the active party. */
	void spawnAvailableNPC(int npc, const Common::UString &waypointTag);
	/** Add a creature directly to the active party. */
	void addPartyMember(int npc, Creature *creature);
	/** Remove a creature from the active party by NPC slot. */
	void removePartyMember(int npc);
	/** Notify the module that the party leader has changed. */
	void notifyPartyLeaderChanged();

	// GUI management

	/** Set the return string for the 'Return to Ebon Hawk' button. */
	void setReturnStrref(uint32_t id);
	/** Set the string for the return dialog */
	void setReturnQueryStrref(uint32_t id);
	/** Enable or disable the 'Return to Ebon Hawk' button. */
	void setReturnEnabled(bool enabled);

	// Global values

	/** Get a global boolean. */
	bool getGlobalBoolean(const Common::UString &id) const;
	/** Set a global number. */
	int getGlobalNumber(const Common::UString &id) const;

	/** Set a global boolean. */
	void setGlobalBoolean(const Common::UString &id, bool value);
	/** Get a global number. */
	void setGlobalNumber(const Common::UString &id, int value);

	/** Get a global string. */
	Common::UString getGlobalString(const Common::UString &id) const;
	/** Set a global string. */
	void setGlobalString(const Common::UString &id, const Common::UString &value);

	// Galaxy map

	/** Show the galaxy map GUI. */
	void showGalaxyMap();
	/** Set whether a planet is selectable by the player. */
	void setPlanetSelectable(int planet, bool selectable);
	/** Is a planet selectable by the player? */
	bool getPlanetSelectable(int planet) const;
	/** Set whether a planet is available (travel target). */
	void setPlanetAvailable(int planet, bool available);
	/** Is a planet available as a travel target? */
	bool getPlanetAvailable(int planet) const;
	/** Get the currently selected planet. */
	int getSelectedPlanet() const;

	// Reputation

	/** Get the reputation of oTarget's faction towards oSource's faction (0–100). */
	int getReputation(int sourceFaction, int targetFaction) const;
	/** Adjust the reputation of targetFaction towards sourceFaction by delta, clamped to [0, 100]. */
	void adjustReputation(int targetFaction, int sourceFaction, int delta);

	// Static utility methods

	static Common::UString getName(const Common::UString &module, const Common::UString &moduleDirOptionName);

	// Module main loop (called by the Game class)

	/** Enter the loaded module, starting it. */
	void enter();
	/** Leave the running module, quitting it. */
	void leave();
	/** Click an object with the mouse. */
	void clickObject(Object *object);
	/** Enter an object with the mouse. */
	void enterObject(Object *object);
	/** Leave an object with the mouse. */
	void leaveObject(Object *object);
	/** Add a single event for consideration into the event queue. */
	void addEvent(const Events::Event &event);
	/** Process the current event queue. */
	void processEventQueue();
	/** Update timestamp of the previous rendered frame. */
	void updateFrameTimestamp();

	// Saved games

	void loadSavedGame(SavedGame *save);
	/** Returns true if the current module was entered via a save-game load. */
	bool isLoadedFromSaveGame() const;

	// Conversation

	bool isConversationActive() const;

	void startConversation(const Common::UString &name, Aurora::NWScript::Object *owner = 0);

	// Animation

	void playAnimationOnActiveObject(const Common::UString &baseAnim,
	                                 const Common::UString &headAnim);

	// GUI creation

	virtual KotORBase::LoadScreen *createLoadScreen(const Common::UString &name) const = 0;

	// Object creation

	virtual KotORBase::Creature *createCreature(const Aurora::GFF3Struct &creature) const = 0;

	// Miscellaneous creation

	virtual KotORBase::CharacterGenerationInfo *createCharGenInfo(const CharacterGenerationInfo &info) const = 0;

	// Camera
	float getCameraYaw() const;
	void setCameraYaw(float yaw);
	
	void setMapExplored(const Common::UString &resRef, const std::vector<bool> &data);
	const std::vector<bool> *getMapExplored(const Common::UString &resRef) const;

	void setCinematicCamera(uint32_t cameraID, float cameraAngle, const Common::UString &cameraModel);
	void setCameraMode(CameraMode mode, Object *target = nullptr);
	void setCinematicFocus(Object *target);
	void setCameraTarget(Object *target);
	void cameraTransitionToTarget(float blendTime);
	void cameraMoveAlongPath(Object *start, Object *end, float duration);
	void cameraHold(float duration);
	void restoreGameplayCamera(float blendTime);
	void resetToOrbit();

	void setCutsceneMode(bool enabled);
	void setPlayerInputEnabled(bool enabled);
	void playMusicStinger(const Common::UString &stinger);

	// Delayed object interactions

	void delayConversation(const Common::UString &name, Aurora::NWScript::Object *owner = nullptr);
	void delayContainer(Placeable *placeable);


	void addItemToActiveObject(const Common::UString &item, int count);
	void toggleFlyCamera();
	void toggleWalkmesh();
	void toggleTriggers();
	int getNextCombatRound() const;

	/** Create a creature from a blueprint template name (for script use). */
	Creature *createCreatureByTemplate(const Common::UString &resRef) const;

	/** Return the last item acquired via script events (may be null). */
	Object *getLastAcquiredItem() const;
	/** Record the last item acquired (called from createItemOnObject etc.). */
	void setLastAcquiredItem(Object *item);

protected:
	std::unique_ptr<IngameGUI> _ingame; ///< The ingame GUI.
	std::unique_ptr<DialogGUI> _dialog; ///< Conversation/cutscene GUI.
	std::unique_ptr<PartySelectionGUI> _partySelection; ///< The party selection GUI.

	// Object creation

	virtual KotORBase::Creature *createCreature() const = 0;
	virtual KotORBase::Creature *createCreature(const Common::UString &resRef) const = 0;

private:
	enum ActionType {
		kActionNone   = 0,
		kActionScript = 1
	};

	struct Action {
		ActionType type;

		Common::UString script;

		Aurora::NWScript::ScriptState state;
		Aurora::NWScript::ObjectReference owner;
		Aurora::NWScript::ObjectReference triggerer;

		uint32_t timestamp;

		bool operator<(const Action &s) const;
	};

	typedef std::list<Events::Event> EventQueue;
	typedef std::multiset<Action> ActionQueue;

	// Global values

	std::map<Common::UString, bool> _globalBooleans;
	std::map<Common::UString, int> _globalNumbers;
	std::map<Common::UString, Common::UString> _globalStrings;

	// Journal: key = quest tag, value = entry ID
	std::map<Common::UString, uint32_t> _journal;

	// Area Object Persistence: key = "areaTag:objectTag", value = state
	std::map<Common::UString, std::shared_ptr<Aurora::GFF3File>> _areaObjectSaves;

	// Inter-faction reputation store: key = (sourceFaction, targetFaction), value in [0, 100]
	std::map<std::pair<int,int>, int> _reputations;

	// Galaxy map planet state

	std::map<int, bool> _planetSelectable; ///< Which planets are selectable on the galaxy map.
	std::map<int, bool> _planetAvailable;  ///< Which planets are available (unlocked).
	int _selectedPlanet;                   ///< The planet currently selected on the galaxy map.

	// Delayed object interactions

	struct DelayedConversation {
		Common::UString name;
		Aurora::NWScript::Object *owner;

		DelayedConversation(const Common::UString &name, Aurora::NWScript::Object *owner = nullptr);
	};

	std::unique_ptr<DelayedConversation> _delayedConversation;
	Placeable *_delayedContainer { nullptr };

	// Surface types

	/** A map between surface type and whether it is walkable. */
	std::vector<bool> _walkableSurfaces;


	::Engines::Console *_console;

	bool _hasModule; ///< Do we have a module?
	bool _running;   ///< Are we currently running a module?

	std::list<Common::ChangeID> _resources; ///< Resources added by the current module.

	Aurora::IFOFile _ifo; ///< The current module's IFO.

	std::unique_ptr<CharacterGenerationInfo> _chargenInfo; ///< Character generation information.
	CreatureInfo _pcInfo; ///< Abstract player character information.
	Creature *_pc; ///< The player character.

	int _currentTexturePack; ///< The current texture pack.
	Common::ChangeID _textures; ///< Resources added by the current texture pack.

	bool _exit;              ///< Should we exit the module?
	bool _loadedFromSaveGame { false }; ///< Was this module entered via a save-game load?

	Common::UString _module;    ///< The current module's name.
	Common::UString _newModule; ///< The module we should change to.

	Common::UString _entryLocation; ///< The tag of the object in the start location for this module.
	ObjectType      _entryLocationType; ///< The type(s) of the object in the start location for this module.

	std::unique_ptr<Area> _area; ///< The current module's area.

	std::unique_ptr<Graphics::Aurora::FadeQuad> _fade;

	EventQueue  _eventQueue;
	ActionQueue _delayedActions;

	PartyLeaderController _partyLeaderController;
	PartyController _partyController;
	CameraController _cameraController;
	RoundController _roundController;

	uint32_t _prevTimestamp;
	float _frameTime;
	bool _inDialog;
	int _runScriptVar;
	bool _soloMode;
	bool _playerInputEnabled { true };
	bool _cutsceneMode { false };
	int _userDefinedEventNumber { 0 };
	float _playTime { 0.0f };
	std::map<Common::UString, std::vector<bool>> _exploredMaps;
	bool _inBattleMusic { false };  ///< Is the battle music track currently playing?
	Object *_lastAcquiredItem { nullptr }; ///< Last item acquired via script events.

	// Unloading

	/** Unload the whole shebang.
	 *
	 *  @param completeUnload Also unload the PC and texture packs.
	 *                        true:  completely quit the module
	 *                        false: the PC can be transferred to a new module.
	 */
	void unload(bool completeUnload = true);

	void unloadResources();
	void unloadIFO();
	void unloadArea();
	void unloadTexturePack();

	// Loading

	void load();
	void loadState(const Aurora::GFF3File &gff);
	void saveState(Aurora::GFF3File &gff) const;

	void loadResources();
	void loadIFO();
	void loadArea();
	void loadPC();
	void loadParty();
	/** Load the surface types. */
	void loadSurfaceTypes();

	/** Load the actual module. */
	void loadModule(const Common::UString &module, const Common::UString &entryLocation,
	                ObjectType entryLocationType);

	/** Schedule a change to a new module. */
	void changeModule(const Common::UString &module, const Common::UString &entryLocation = "",
	                  ObjectType entryLocationType = kObjectTypeAll);

	void saveGame(const Common::UString &slot, const Common::UString &name);

	void update(uint32_t frameTime);

	/** Actually replace the currently running module. */
	void replaceModule();

	// Party

	void updateCurrentPartyGUI();

	// Minimap

	void initMinimap();
	void updateMinimap();


	bool getObjectLocation(const Common::UString &object, ObjectType location,
	                       float &entryX, float &entryY, float &entryZ, float &entryAngle);

	bool getEntryObjectLocation(float &entryX, float &entryY, float &entryZ, float &entryAngle);
	void getEntryIFOLocation(float &entryX, float &entryY, float &entryZ, float &entryAngle);

	void enterArea();
	void leaveArea();

	void handleEvents();
	void handleActions();

	void updateSoundListener();
	void updateSelection();

	void handleDelayedInteractions();
	void openContainer(Placeable *placeable);

	void notifyCombatRoundBegan(int round);
	void notifyCombatRoundEnded(int round);

	/**
	 * Tick NPC combat AI: hostile NPCs within perception range will auto-queue
	 * an attack action against the nearest party member.  Party members that are
	 * not the current leader will auto-attack any NPC that is attacking the party.
	 */
	void updateCombatAI();

	/** Switch ambient/battle music depending on whether any creature is in combat. */
	void updateBattleMusic();

	/** Award XP to the party for creatures that died this frame. */
	void updateXPOnKill();

	friend class RoundController;
};

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_MODULE_H
