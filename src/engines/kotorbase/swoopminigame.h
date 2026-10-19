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
 *  Swoop racing / turret minigame simulation (SWMG) for KotOR games.
 *
 *  This is the graphics-free half of the minigames: the player, enemy and
 *  obstacle state the SWMG_* script functions read and write, the player's
 *  movement, gun banks and bullets, the collision tests and which scripts
 *  they fire. Models, the camera and input live in MinigameRuntime, which
 *  feeds world positions in and runs the scripts through a MinigameHost.
 *
 *  Ported from KotOR.js (ModuleMiniGame, ModuleMGPlayer, ModuleMGEnemy,
 *  ModuleMGObstacle, ModuleMGGunBank, ModuleMGGunBullet).
 *
 *  World conventions (as in KotOR.js): a swoop player moves along +Y at its
 *  speed and sideways along X; a turret player rotates around X (up/down)
 *  and Z (left/right). Bullets fly along their shooter's forward (+Y) axis.
 */

#ifndef ENGINES_KOTORBASE_SWOOPMINIGAME_H
#define ENGINES_KOTORBASE_SWOOPMINIGAME_H

#include <list>
#include <map>
#include <memory>
#include <set>
#include <vector>

#include "external/glm/vec3.hpp"

#include "src/common/ustring.h"

#include "src/engines/kotorbase/object.h"
#include "src/engines/kotorbase/minigamedesc.h"

namespace Engines {

namespace KotORBase {

enum MinigameObjectKind {
	kMinigameObjectPlayer,
	kMinigameObjectEnemy,
	kMinigameObjectObstacle
};

/** The object scripts see for a minigame player, enemy or obstacle. */
class MinigameObject : public Object {
public:
	MinigameObject(MinigameObjectKind kind, size_t index, const Common::UString &name);

	MinigameObjectKind getMinigameKind() const { return _kind; }
	size_t getMinigameIndex() const { return _index; }

private:
	MinigameObjectKind _kind;
	size_t _index;
};

/** The player's steering input this frame. */
struct MinigameInput {
	bool left  { false };
	bool right { false };
	bool up    { false };
	bool down  { false };
};

/** What the simulation needs from the game: scripts, animations, effects. */
class MinigameHost {
public:
	virtual ~MinigameHost() { }

	/** Run a script with the given minigame object as OBJECT_SELF. */
	virtual void runScript(const Common::UString &script, MinigameObject &self) = 0;

	virtual void playAnimation(MinigameObject &UNUSED(object), const Common::UString &UNUSED(animation),
	                           bool UNUSED(loop)) { }
	virtual void removeAnimation(MinigameObject &UNUSED(object), const Common::UString &UNUSED(animation)) { }

	/** A player or enemy died (and its OnDeath script didn't take over). */
	virtual void onDeath(MinigameObject &UNUSED(object)) { }

	virtual void playSound(const Common::UString &UNUSED(sound), MinigameObject *UNUSED(source)) { }

	/** The camera near/far clip planes changed. */
	virtual void onCameraClipChanged() { }
};

/** Per-sound settings scripts tweak (SWMG_SetSound*). */
struct MinigameSoundSettings {
	int  frequency { 0 };
	bool random { false };
	int  volume { 127 };
};

struct MinigameGunBank {
	MinigameGunBankDesc desc;
	float fireTimer { 0.0f }; ///< Seconds until the bank can fire again.
	bool targeting { false }; ///< Is the bank's aim on a target (enemies)?
};

/** The player or an enemy. */
struct MinigameFollower {
	MinigameFollowerDesc desc;
	std::unique_ptr<MinigameObject> object;

	int hitPoints { 0 };
	int maxHitPoints { 0 };
	bool hadHitPoints { false }; ///< Hit points have been above 0, so 0 means dead.
	bool alive { true };
	float invulnerability { 0.0f }; ///< Seconds of invulnerability left.
	float sphereRadius { 0.0f };
	int numLoops { 0 };

	glm::vec3 position { 0.0f };     ///< World position (fed by the runtime).
	glm::vec3 lastPosition { 0.0f }; ///< World position the frame before.
	glm::vec3 forward { 0.0f, 1.0f, 0.0f };
	bool hasPosition { false };

	std::vector<MinigameGunBank> gunBanks;
	std::set<const MinigameObject *> contacts; ///< Objects currently touched.
	std::map<int, MinigameSoundSettings> sounds;
};

struct MinigameObstacle {
	MinigameObstacleDesc desc;
	std::unique_ptr<MinigameObject> object;

	glm::vec3 boundsMin { 0.0f }; ///< World bounding box.
	glm::vec3 boundsMax { 0.0f };
	int hitPoints { 0 };
	float invulnerability { 0.0f };
};

struct MinigameBullet {
	uint32_t id { 0 };
	MinigameObject *shooter { nullptr };
	size_t bank { 0 };

	glm::vec3 position { 0.0f };
	glm::vec3 previous { 0.0f };              ///< Position before the last step.
	glm::vec3 direction { 0.0f, 1.0f, 0.0f }; ///< Normalized.
	float speed { 0.0f };
	float age { 0.0f };
	float lifespan { 0.0f };
	int damage { 0 };
	uint32_t targetType { 0 };
	bool spent { false };
};

/** A collision found by SwoopMinigame::detectHits(). */
struct MinigameHit {
	enum Type {
		kFollowerHitObstacle, ///< subject (player) ran into other (obstacle).
		kFollowerHitFollower, ///< subject (player) ran into other (enemy).
		kBulletHitFollower    ///< bullet hit subject (player or enemy).
	};

	Type type;
	MinigameObject *subject { nullptr };
	MinigameObject *other { nullptr };
	MinigameBullet bullet;
};

/** An enemy gun bank that has the player in its sights and may fire. */
struct MinigameFireRequest {
	MinigameObject *shooter { nullptr };
	size_t bank { 0 };
};

// .--- Geometry helpers

/** Does the segment from a to b pass within radius of center? */
bool segmentHitsSphere(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &center, float radius);

/** Does the segment from a to b touch the box [min, max] grown by margin on all sides? */
bool segmentHitsBox(const glm::vec3 &a, const glm::vec3 &b,
                    const glm::vec3 &min, const glm::vec3 &max, float margin);

/** Does the ray from origin along direction pass within radius of center, no farther than range (0: unlimited)? */
bool rayHitsSphere(const glm::vec3 &origin, const glm::vec3 &direction,
                   const glm::vec3 &center, float radius, float range);

/** Clamp a value into the tunnel [neg, pos], unless infinite or the tunnel is empty (pos <= neg). */
float clampToTunnel(float value, float neg, float pos, bool infinite);

// '---

/** The running minigame, backing the SWMG_* script functions. */
class SwoopMinigame {
public:
	static SwoopMinigame &get();

	/** Forget the current minigame. */
	void reset();

	/** Set up a minigame from an area's description; one obstacle per entry in obstacleNames.
	 *
	 *  Obstacles are the LYT's; each gets the scripts of the ARE obstacle with the same name.
	 */
	void start(const MinigameDesc &desc, const std::vector<Common::UString> &obstacleNames);

	bool isActive() const { return _active; }
	MinigameType getType() const { return _desc.type; }
	const MinigameDesc &getDesc() const { return _desc; }

	void setHost(MinigameHost *host) { _host = host; }

	// .--- Objects
	MinigameObject *getPlayer();
	size_t getEnemyCount() const { return _enemies.size(); }
	MinigameObject *getEnemy(size_t index);
	size_t getObstacleCount() const { return _obstacles.size(); }
	MinigameObject *getObstacle(size_t index);
	MinigameObject *getObjectByName(const Common::UString &name);

	MinigameFollower &getPlayerState() { return _player; }
	MinigameFollower *getEnemyState(size_t index);
	MinigameObstacle *getObstacleState(size_t index);

	/** The player or enemy behind a script object, if any. */
	MinigameFollower *getFollower(Aurora::NWScript::Object *object);
	/** The obstacle behind a script object, if any. */
	MinigameObstacle *getObstacleOf(Aurora::NWScript::Object *object);
	/** The minigame object behind a script object, if any. */
	MinigameObject *toMinigameObject(Aurora::NWScript::Object *object);

	Common::UString getObjectName(Aurora::NWScript::Object *object);
	bool isTrigger(Aurora::NWScript::Object *object);
	// '---

	// .--- The runtime's per-frame calls
	void setInput(const MinigameInput &input) { _input = input; }

	/** Move the player, age timers and bullets, and notice deaths. */
	void step(float dt);

	/** Where a player or enemy is in the world, and where it faces. */
	void setFollowerPose(MinigameObject &object, const glm::vec3 &position, const glm::vec3 &forward);
	/** An obstacle's world bounding box. */
	void setObstacleBounds(size_t index, const glm::vec3 &min, const glm::vec3 &max);

	/** Find new contacts: player against obstacles and enemies, bullets against targets. */
	std::vector<MinigameHit> detectHits();
	/** Set the last-event state and run the scripts (or default behaviour) for hits. */
	void dispatchHits(const std::vector<MinigameHit> &hits);

	/** Enemy gun banks aiming at the player and ready to fire. */
	std::vector<MinigameFireRequest> getEnemyFireRequests();

	/** Fire a gun bank, if it's ready. Returns whether a bullet was spawned. */
	bool fire(MinigameObject &shooter, size_t bank, const glm::vec3 &origin, const glm::vec3 &direction);

	const std::list<MinigameBullet> &getBullets() const { return _bullets; }
	// '---

	// .--- Script events
	/** Run an object's script for an event. Returns false if it has none. */
	bool runEvent(MinigameObject &object, MinigameScript script);

	/** OnCreate of the enemies, the obstacles, then the player. */
	void runCreateEvents();
	/** OnHeartbeat of the player, the enemies and the obstacles. */
	void runHeartbeats();

	void onTrackLoop(MinigameObject &follower);
	void onAccelerate();
	void onBrake();
	void onFire(MinigameObject &shooter);

	void playAnimation(MinigameObject &object, const Common::UString &animation, bool loop);
	void removeAnimation(MinigameObject &object, const Common::UString &animation);
	// '---

	// .--- Default behaviour (SWMG_On*)
	void defaultBulletHit(MinigameObject &self);
	void defaultObstacleHit(MinigameObject &self);
	void defaultDamage(MinigameObject &self);
	void defaultDeath(MinigameObject &self);

	/** Take hit points off a player or enemy, running OnDamage and OnDeath. */
	void damage(MinigameObject &self, int amount);
	// '---

	// .--- Player movement
	void setLateralAcceleration(float accel) { _lateralAccel = accel; }
	float getLateralAcceleration() const { return _lateralAccel; }

	void setSpeed(float speed) { _speed = speed; }
	float getSpeed() const { return _speed; }
	void setMaxSpeed(float maxSpeed) { _maxSpeed = maxSpeed; }
	float getMaxSpeed() const { return _maxSpeed; }
	void setMinSpeed(float minSpeed) { _minSpeed = minSpeed; }
	float getMinSpeed() const { return _minSpeed; }
	void setAccelerationPerSecond(float accel) { _accelPerSec = accel; }
	float getAccelerationPerSecond() const { return _accelPerSec; }

	/** Swoop: translation from the track start. Turret: rotation in degrees. */
	void setPlayerOffset(const glm::vec3 &offset);
	const glm::vec3 &getPlayerOffset() const { return _offset; }

	void setTunnelPos(const glm::vec3 &bound);
	const glm::vec3 &getTunnelPos() const { return _tunnelPos; }
	void setTunnelNeg(const glm::vec3 &bound);
	const glm::vec3 &getTunnelNeg() const { return _tunnelNeg; }
	void setTunnelInfinite(const glm::vec3 &infinite);
	const glm::vec3 &getTunnelInfinite() const { return _tunnelInfinite; }

	void setPlayerOrigin(const glm::vec3 &origin) { _origin = origin; }
	const glm::vec3 &getPlayerOrigin() const { return _origin; }

	/** Turret rotation speed, degrees per second. */
	static float getTurretTurnRate();
	void setReverseVerticalInput(bool reverse) { _reverseVertical = reverse; }

	/** Player invincibility: seconds left of it. */
	void setPlayerInvincibility(float seconds);
	float getPlayerInvincibility() const;
	// '---

	// .--- Per-object state
	int getHitPoints(Aurora::NWScript::Object *object);
	int getMaxHitPoints(Aurora::NWScript::Object *object);
	void setMaxHitPoints(Aurora::NWScript::Object *object, int hp);
	/** Add to (or, if absolute, set) hit points. Returns the new hit points. */
	int adjustHitPoints(Aurora::NWScript::Object *object, int hp, bool absolute);

	float getSphereRadius(Aurora::NWScript::Object *object);
	void setSphereRadius(Aurora::NWScript::Object *object, float radius);
	int getNumLoops(Aurora::NWScript::Object *object);
	void setNumLoops(Aurora::NWScript::Object *object, int loops);
	glm::vec3 getPosition(Aurora::NWScript::Object *object);

	bool isInvulnerable(Aurora::NWScript::Object *object);
	void startInvulnerability(Aurora::NWScript::Object *object);

	MinigameGunBank *getGunBank(Aurora::NWScript::Object *object, int bank);
	int getGunBankCount(Aurora::NWScript::Object *object);

	MinigameSoundSettings &getSound(Aurora::NWScript::Object *object, int sound);
	// '---

	// .--- Camera and effects
	void setCameraClip(float nearClip, float farClip);
	float getCameraNearClip() const { return _nearClip; }
	float getCameraFarClip() const { return _farClip; }

	void setSpeedBlur(bool enabled, float ratio) { _speedBlur = enabled; _speedBlurRatio = ratio; }
	bool getSpeedBlur() const { return _speedBlur; }
	// '---

	// .--- Last-event state, valid while a minigame script runs
	const Common::UString &getLastEvent() const { return _lastEvent; }
	const Common::UString &getLastEventModelName() const { return _lastEventModelName; }
	void setLastAnimEvent(const Common::UString &event, const Common::UString &modelName);

	int getLastHPChange() const { return _lastHPChange; }
	int getLastBulletHitDamage() const { return _lastBulletHitDamage; }
	uint32_t getLastBulletHitTarget() const { return _lastBulletHitTarget; }
	MinigameObject *getLastBulletHitShooter() const { return _lastBulletHitShooter; }
	int getLastBulletFiredDamage() const { return _lastBulletFiredDamage; }
	uint32_t getLastBulletFiredTarget() const { return _lastBulletFiredTarget; }
	MinigameObject *getLastFollowerHit() const { return _lastFollowerHit; }
	MinigameObject *getLastObstacleHit() const { return _lastObstacleHit; }
	// '---

private:
	SwoopMinigame();

	bool _active { false };
	MinigameDesc _desc;
	MinigameHost *_host { nullptr };

	MinigameFollower _player;
	std::vector<std::unique_ptr<MinigameFollower>> _enemies;
	std::vector<std::unique_ptr<MinigameObstacle>> _obstacles;
	std::list<MinigameBullet> _bullets;
	uint32_t _nextBulletID { 1 };

	MinigameInput _input;
	bool _reverseVertical { false };

	float _lateralAccel { 0.0f };
	float _speed { 0.0f };
	float _maxSpeed { 0.0f };
	float _minSpeed { 0.0f };
	float _accelPerSec { 0.0f };

	glm::vec3 _offset { 0.0f };
	glm::vec3 _tunnelNeg { 0.0f };
	glm::vec3 _tunnelPos { 0.0f };
	glm::vec3 _tunnelInfinite { 0.0f };
	glm::vec3 _origin { 0.0f };

	float _nearClip { 0.0f };
	float _farClip { 0.0f };
	bool _speedBlur { false };
	float _speedBlurRatio { 0.0f };

	Common::UString _lastEvent;
	Common::UString _lastEventModelName;
	int _lastHPChange { 0 };
	int _lastBulletHitDamage { 0 };
	uint32_t _lastBulletHitTarget { 0 };
	MinigameObject *_lastBulletHitShooter { nullptr };
	int _lastBulletFiredDamage { 0 };
	uint32_t _lastBulletFiredTarget { 0 };
	MinigameObject *_lastFollowerHit { nullptr };
	MinigameObject *_lastObstacleHit { nullptr };

	MinigameSoundSettings _noSound;

	void initFollower(MinigameFollower &follower, const MinigameFollowerDesc &desc,
	                  MinigameObjectKind kind, size_t index);

	const MinigameScripts *getScripts(MinigameObject &object);
	void checkDeath(MinigameFollower &follower);

	void movePlayer(float dt);
	void moveBullets(float dt);
	bool isSwoop() const { return _desc.type == kMinigameTypeSwoop; }

	/** The position used for collisions: flattened onto the track plane in a swoop race. */
	glm::vec3 collisionPoint(const glm::vec3 &position) const;
};

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_SWOOPMINIGAME_H
