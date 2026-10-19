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
 *  The context holding an area in KotOR games.
 */

#ifndef ENGINES_KOTORBASE_AREA_H
#define ENGINES_KOTORBASE_AREA_H

#include <vector>
#include <list>
#include <map>
#include <set>

#include "external/glm/vec2.hpp"

#include "src/common/ustring.h"
#include "src/common/mutex.h"
#include <memory>

#include "src/aurora/types.h"

namespace Aurora {
class GFF3WriterStruct;
}
#include "src/aurora/lytfile.h"
#include "src/aurora/visfile.h"

#include "src/sound/types.h"

#include "src/events/types.h"
#include "src/events/notifyable.h"

#include "src/engines/kotorbase/object.h"
#include "src/engines/kotorbase/trigger.h"
#include "src/engines/kotorbase/heartbeatscheduler.h"

namespace Engines {

class LocalPathfinding;
class ObjectWalkmesh;

namespace KotORBase {

class Module;
class Room;
class Situated;
class Creature;
class Item;
class Placeable;
class Encounter;
class Pathfinding;
struct CreatureSearchCriteria;

/** An area in Star Wars: Knights of the Old Republic, holding all objects and
 *  rooms within, as well as general area properties like the current background
 *  music and ambient sounds.
 *
 *  Note: the coordinate system of the in-world graphics is oriented as viewed
 *  from the top down. The negative Z axis goes down into the ground, while the
 *  positive Y axis points due north and the positive X axis points due east.
 */
class Area : public Object, public Events::Notifyable {
public:
	Area(Module &module, const Common::UString &resRef);
	~Area();

	// General properties

	/** Return the area's resref (resource ID). */
	const Common::UString &getResRef();

	// Minimap

	/** Get the north axis id. */
	int getNorthAxis();
	/** Get the first map point. */
	void getMapPoint1(float &x, float &y);
	/** Get the second map point. */
	void getMapPoint2(float &x, float &y);
	/** Get the first world point. */
	void getWorldPoint1(float &x, float &y);
	/** Get the second world point. */
	void getWorldPoint2(float &x, float &y);

	// Visibility

	void show();
	void hide();

	// Music/Sound

	uint32_t getMusicDayTrack   () const; ///< Return the music track ID playing by day.
	uint32_t getMusicNightTrack () const; ///< Return the music track ID playing by night.
	uint32_t getMusicBattleTrack() const; ///< Return the music track ID playing in battle.

	void setMusicDayTrack   (uint32_t track); ///< Set the music track ID playing by day.
	void setMusicNightTrack (uint32_t track); ///< Set the music track ID playing by night.
	void setMusicBattleTrack(uint32_t track); ///< Set the music track ID playing in battle.

	void setAmbientSoundDayTrack  (uint32_t track); ///< Set the ambient sound track playing by day.
	void setAmbientSoundNightTrack(uint32_t track); ///< Set the ambient sound track playing by night.
	void setAmbientSoundDayVolume(int volume);
	void setAmbientSoundNightVolume(int volume);

	void stopSound();        ///< Stop all sounds.
	void stopAmbientMusic(); ///< Stop the ambient music.
	void stopAmbientSound(); ///< Stop the ambient sound.

	/** Play the specified music (or the area's default) as ambient music. */
	void playAmbientMusic(Common::UString music = "");
	/** Play the area's battle music track (used during combat). */
	void playBattleMusic();
	/** Play the specified sound (or the area's default) as ambient sound. */
	void playAmbientSound(Common::UString sound = "");

	// Events

	/** Add a single event for consideration into the area event queue. */
	void addEvent(const Events::Event &event);
	/** Process the current event queue. */
	void processEventQueue();

	/** Forcibly remove the focus from the currently highlighted object. */
	void removeFocus();

	// Walkmesh

	float evaluateElevation(float x, float y);
	bool getGroundPointAtScreen(int screenX, int screenY, float &outX, float &outY, float &outZ);
	bool walkable(const glm::vec3 &orig, const glm::vec3 &dest) const;
	/** A walkmesh route from @p from to @p to, as waypoints (the last one is @p to).
	 *  False when either end is off the walkmesh or no route connects them. */
	bool findPath(const glm::vec2 &from, const glm::vec2 &to, std::vector<glm::vec2> &waypoints);
	void toggleWalkmesh();
	bool rayTest(const glm::vec3 &orig, const glm::vec3 &dest, glm::vec3 &intersect) const;

	// Triggers

	void toggleTriggers();
	/** A creature moved: check the triggers for it at the next safe point (see
	 *  processTriggerChecks()). @p allowTransitions is false when it arrived by
	 *  teleport, so that an area transition it lands in doesn't move it on. */
	void queueTriggerCheck(const Object &creature, bool allowTransitions = true);
	/** Run the trigger OnExit/OnEnter (and mine) events of the creatures that
	 *  moved. Call outside of any iteration over objects. */
	void processTriggerChecks();
	/** The triggers in the area, mines included. */
	const std::vector<Trigger *> &getTriggers() const;
	/** The creature tries to spot the mines around it (Awareness). */
	void detectTraps(Creature &creature, bool rollDice);
	/** The mine (trigger ID) the creature spotted last, or 0. */
	uint32_t getLastTrapDetected(uint32_t creatureID) const;

	// Encounters

	/** Let the encounters see where the party is, and spawn. Once per frame. */
	void updateEncounters();
	/** The encounter that spawned this creature, if any. */
	Encounter *findEncounterOwning(const Object &creature) const;

	// Object management

	Object *getObjectByTag(const Common::UString &tag);
	Creature *getNearestCreature(const Object *target, int nth, const CreatureSearchCriteria &criteria) const;
	const std::vector<Creature *> &getCreatures() const;

	void addCreature(Creature *creature);
	Item *spawnDroppedItem(const Common::UString &tag, float x, float y, float z);
	/** Create a placeable from its blueprint (CreateObject). */
	Placeable *spawnPlaceable(const Common::UString &templateResRef, float x, float y, float z, float facing);
	/** Lay a mine of traps.2da row @p trapType at @p position, set by @p setter: a
	 *  trap trigger of the setter's faction (it goes off for their enemies), that
	 *  the party knows about. It's kept when the party leaves and comes back. */
	Trigger *placeMine(int trapType, const glm::vec3 &position, Creature &setter);
	void addToObjectMap(Object *object);
	void removeObject(Object *object);
	/** Destroy an object at the end of the frame (DestroyObject), after an optional delay. */
	void destroyObjectDeferred(Object *object, float delay = 0.0f);
	/** Run the OnHeartbeat of the creatures, placeables, doors and triggers that are due. */
	void runHeartbeats();
	/** Run queued OnSpawn scripts now (before the area's OnEnter, as in the original). */
	void runPendingSpawnEvents();
	/** Remove objects whose deferred destruction is due. Call outside object iteration. */
	void processPendingDestruction();
	/** The game was paused for @p ms: push deferred destructions back as long. */
	void delayPendingDestruction(uint32_t ms);

	// Persistence
	void loadPersistence();
	void savePersistence();
	/** Give every persistent object its save-state key (GIT order). */
	void assignPersistenceKeys();
	/** The object leaves the area for good (destroyed, joined the party, ...). */
	void releaseFromPersistence(Object &object);
	Common::UString getSpawnedCreaturesKey() const;
	Common::UString getPlacedMinesKey() const;
	/** Add a mine laid at runtime, from its instance and trap state. */
	Trigger *addMine(const Aurora::GFF3Struct &instance);
	/** Save-state key of an object; tagCounts tracks repeated tags. */
	Common::UString getPersistenceKey(const Object &object, std::map<Common::UString, int> &tagCounts) const;
	void writePersistence(Aurora::GFF3WriterStruct &gff) const;

	const std::vector<bool> &getMapExplored() const { return _mapExplored; }
	void setMapExplored(const std::vector<bool> &data) { _mapExplored = data; }

	static size_t getDefaultMapExploredTileCount();
	void exploreMapFully();
	/** Reveal minimap tiles near a world position. Returns true if any tile changed. */
	bool revealMapNear(float worldX, float worldY, int radiusTiles = 1);

	// Camera style

	struct CameraStyle {
		float distance { 0.0f };
		float pitch { 0.0f };
		float height { 0.0f };
		float viewAngle { 0.0f };
	};

	const Area::CameraStyle &getCameraStyle() const;




	// Spatial Querying

	const std::vector<Common::UString> &getRoomsVisibleFrom(const Common::UString &room) const;
	std::set<Common::UString> getRoomsVisibleByPartyLeader() const;

	void showAllRooms();
	void notifyObjectMoved(Object &o);
	void notifyPartyLeaderMoved();

	// Minigame

	bool isMinigame();
	const Aurora::GFF3Struct& getMinigame();
	/** The area layout, with the minigame tracks and obstacles. */
	const Aurora::LYTFile &getLayout() const { return _lyt; }


	Object *getActiveObject();

	void processCreaturesActions(float dt);
	void handleCreaturesDeath();
	/** Run the OnMeleeAttacked/OnDamaged/OnDeath scripts of doors and containers
	 *  being bashed, and break open those that fell to 0 hit points. */
	void handleSituatedEvents();

	Creature *findNearestEnemy(Creature *origin);

	/** True if no closed door blocks the XY segment between the two points. */
	bool hasLineOfSight(float x0, float y0, float x1, float y1) const;
	bool hasLineOfSight(const Object *from, const Object *to) const;
	/** Geometry helper: does segment (x0,y0)-(x1,y1) intersect circle (cx,cy,r)? */
	static bool segmentIntersectsCircle(float x0, float y0, float x1, float y1,
	                                    float cx, float cy, float radius);

	struct Camera {
		uint32_t id;
		float position[3];
		float orientation[4];
		float fieldOfView;
		float pitch;  ///< Degrees, applied after the orientation (0 looks straight down).
		float height; ///< Added to the position's z.
	};

	const Camera *getCamera(uint32_t id) const;

protected:
	void notifyCameraMoved();

private:
	typedef std::list<std::unique_ptr<Room>> RoomList;

	typedef std::list<std::unique_ptr<Object>> ObjectList;
	typedef std::map<uint32_t, Object *> ObjectMap;

	std::unique_ptr<Aurora::GFF3File> _are;

	Module *_module; ///< The module this area is in.

	Common::UString _resRef;      ///< The area's resref (resource ID).
	Common::UString _displayName; ///< The area's localized display name.

	Common::UString _ambientDay;   ///< Ambient sound that plays by day.
	Common::UString _ambientNight; ///< Ambient sound that plays by night.

	uint32_t _musicDayTrack;    ///< Music track ID that plays by day.
	uint32_t _musicNightTrack;  ///< Music track ID that plays by night.
	uint32_t _musicBattleTrack; ///< Music track ID that plays in battle.

	Common::UString _musicDay;    ///< Music that plays by day.
	Common::UString _musicNight;  ///< Music that plays by night.
	Common::UString _musicBattle; ///< Music that plays in battle.

	std::vector<Common::UString> _musicBattleStinger; ///< Battle music stingers.

	float _ambientDayVol;   ///< Day ambient sound volume.
	float _ambientNightVol; ///< Night ambient sound volume.

	bool _visible; ///< Is the area currently visible?

	float _worldPt1X, _worldPt1Y, _worldPt2X, _worldPt2Y;
	float _mapPt1X, _mapPt1Y, _mapPt2X, _mapPt2Y;
	int _northAxis;

	Sound::ChannelHandle _ambientSound; ///< Sound handle of the currently playing sound.
	Sound::ChannelHandle _ambientMusic; ///< Sound handle of the currently playing music.

	Aurora::LYTFile _lyt; ///< The area's layout description.
	Aurora::VISFile _vis; ///< The area's inter-room visibility description.

	RoomList _rooms; ///< All rooms in the area.

	ObjectList _objects;   ///< List of all objects in the area.
	ObjectMap  _objectMap; ///< Map of all non-static objects in the area.

	std::vector<Creature *> _creatures;

	Object *_activeObject; ///< The currently active (highlighted) object.

	/** Objects queued by DestroyObject: (object ID, timestamp when due). */
	std::vector<std::pair<uint32_t, uint32_t>> _pendingDestruction;

	/** Static pathfinding walkmesh of each door/placeable (owned by the local pathfinding). */
	std::map<const Object *, const Engines::ObjectWalkmesh *> _staticWalkmeshes;

	bool _highlightAll; ///< Are we currently highlighting all objects?
	std::vector<bool> _mapExplored;

	std::list<Events::Event> _eventQueue; ///< The event queue.

	std::recursive_mutex _mutex; ///< Mutex securing access to the area.

	// Triggers

	std::vector<Trigger *> _triggers;
	bool _triggersVisible;

	/** Creatures (ID) whose triggers need checking -> may they use area transitions. */
	std::map<uint32_t, bool> _pendingTriggerChecks;
	bool _processingTriggers { false };
	/** Creature ID -> the mine (trigger ID) it spotted last. */
	std::map<uint32_t, uint32_t> _lastTrapDetected;
	/** Mines laid at runtime (trigger IDs), which the GIT doesn't know about. */
	std::vector<uint32_t> _placedMines;

	HeartbeatScheduler _heartbeats;

	std::vector<Encounter *> _encounters;


	std::vector<Camera> _cameras;
	CameraStyle _cameraStyle;
	bool _walkmeshInvisible;
	std::list<Situated *> _situatedObjects;

	Pathfinding *_pathfinding;
	Engines::LocalPathfinding *_localPathfinding;

	// Loading helpers

	void clear();
	void load();

	void loadLYT();
	void loadVIS();

	void loadARE(const Aurora::GFF3Struct &are);
	void loadGIT(const Aurora::GFF3Struct &git);

	void loadCameraStyle(uint32_t id);

	void loadRooms();

	void loadProperties(const Aurora::GFF3Struct &props);

	void loadObject(std::unique_ptr<Object> &&object);

	void loadWaypoints (const Aurora::GFF3List &list);
	void loadPlaceables(const Aurora::GFF3List &list);
	void loadDoors     (const Aurora::GFF3List &list);
	void loadCreatures (const Aurora::GFF3List &list);
	void loadSounds    (const Aurora::GFF3List &list);
	void loadTriggers  (const Aurora::GFF3List &list);
	void loadEncounters(const Aurora::GFF3List &list);
	void loadCameras   (const Aurora::GFF3List &list);

	// Highlight / active helpers

	Object *getObjectAt(int x, int y);

	void setActive(Object *object);

	void checkActive(int x = -1, int y = -1);
	void highlightAll(bool enabled);
	void click(int x, int y);


	void updateRoomsVisiblity();
	void updatePerception(Creature &subject);

	// Trigger helpers

	Trigger *findTrigger(uint32_t id) const;
	Creature *findCreature(uint32_t id) const;
	/** Put the creatures standing in triggers at load time inside them, without events. */
	void seedTriggerOccupants();
	/** Check the triggers for one creature. */
	void checkTriggers(uint32_t creatureID, bool allowTransitions);
	/** A creature entered a mine. */
	void enterTrap(Trigger &trigger, Creature &creature);
	/** The object leaves the area: it is no longer inside any trigger. */
	void forgetTriggerOccupant(uint32_t id);
	void runHeartbeat(Object &object);


	friend class Console;
	friend class ActionExecutor;
};

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_AREA_H
