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
 */

#include <algorithm>
#include <cmath>

#include "external/glm/geometric.hpp"

#include "src/common/debug.h"
#include "src/common/system.h"

#include "src/engines/kotorbase/swoopminigame.h"
#include "src/engines/kotorbase/objectcontainer.h"
#include "src/engines/kotorbase/types.h"

namespace Engines {

namespace KotORBase {

namespace {

/** KotOR.js turns the turret by 1 radian per second. */
const float kTurretTurnRate = 57.2957795f;

/** Followers without a Sphere_Radius still collide, with this radius. */
const float kDefaultSphereRadius = 1.0f;

/** Death animations, as KotOR.js plays them. */
const char *const kAnimationDamage = "damage";
const char *const kAnimationDie    = "die";

float followerRadius(const MinigameFollower &follower) {
	return (follower.sphereRadius > 0.0f) ? follower.sphereRadius : kDefaultSphereRadius;
}

bool hasBounds(const MinigameObstacle &obstacle) {
	return (obstacle.boundsMax.x >= obstacle.boundsMin.x) &&
	       (obstacle.boundsMax.y >= obstacle.boundsMin.y) &&
	       (obstacle.boundsMax.z >= obstacle.boundsMin.z);
}

Common::UString followerName(const MinigameFollowerDesc &desc) {
	return desc.models.empty() ? Common::UString() : desc.models.front().model;
}

} // End of anonymous namespace

// .--- MinigameObject

MinigameObject::MinigameObject(MinigameObjectKind kind, size_t index, const Common::UString &name) :
	Object(kObjectTypePlaceable), _kind(kind), _index(index) {

	setTag(name);
	_name = name;
}

// '---

// .--- Geometry helpers

bool segmentHitsSphere(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &center, float radius) {
	const glm::vec3 ab = b - a;
	const float lengthSquared = glm::dot(ab, ab);

	float t = 0.0f;
	if (lengthSquared > 0.0f)
		t = std::max(0.0f, std::min(1.0f, glm::dot(center - a, ab) / lengthSquared));

	const glm::vec3 closest = a + ab * t;
	const glm::vec3 d = center - closest;

	return glm::dot(d, d) <= radius * radius;
}

bool segmentHitsBox(const glm::vec3 &a, const glm::vec3 &b,
                    const glm::vec3 &min, const glm::vec3 &max, float margin) {

	// Slab test against the grown box.
	float tMin = 0.0f, tMax = 1.0f;

	for (int axis = 0; axis < 3; ++axis) {
		const float lo = min[axis] - margin;
		const float hi = max[axis] + margin;
		const float d  = b[axis] - a[axis];

		if (std::fabs(d) < 1e-6f) {
			if ((a[axis] < lo) || (a[axis] > hi))
				return false;

			continue;
		}

		float t1 = (lo - a[axis]) / d;
		float t2 = (hi - a[axis]) / d;
		if (t1 > t2)
			std::swap(t1, t2);

		tMin = std::max(tMin, t1);
		tMax = std::min(tMax, t2);
		if (tMin > tMax)
			return false;
	}

	return true;
}

bool rayHitsSphere(const glm::vec3 &origin, const glm::vec3 &direction,
                   const glm::vec3 &center, float radius, float range) {

	const float length = glm::length(direction);
	if (length <= 0.0f)
		return false;

	const glm::vec3 dir = direction / length;
	const float along = glm::dot(center - origin, dir);
	if (along < -radius)
		return false;
	if ((range > 0.0f) && (along - radius > range))
		return false;

	const glm::vec3 closest = origin + dir * std::max(0.0f, along);
	const glm::vec3 d = center - closest;

	return glm::dot(d, d) <= radius * radius;
}

float clampToTunnel(float value, float neg, float pos, bool infinite) {
	if (infinite || (pos <= neg))
		return value;

	return std::max(neg, std::min(pos, value));
}

// '---

SwoopMinigame &SwoopMinigame::get() {
	static SwoopMinigame instance;
	return instance;
}

SwoopMinigame::SwoopMinigame() {
	reset();
}

void SwoopMinigame::reset() {
	_active = false;
	_desc = MinigameDesc();

	_player = MinigameFollower();
	_enemies.clear();
	_obstacles.clear();
	_bullets.clear();
	_nextBulletID = 1;

	_input = MinigameInput();

	_lateralAccel = 0.0f;
	_speed        = 0.0f;
	_maxSpeed     = 0.0f;
	_minSpeed     = 0.0f;
	_accelPerSec  = 0.0f;

	_offset         = glm::vec3(0.0f);
	_tunnelNeg      = glm::vec3(0.0f);
	_tunnelPos      = glm::vec3(0.0f);
	_tunnelInfinite = glm::vec3(0.0f);
	_origin         = glm::vec3(0.0f);

	_nearClip = 0.0f;
	_farClip  = 0.0f;
	_speedBlur = false;
	_speedBlurRatio = 0.0f;

	_lastEvent.clear();
	_lastEventModelName.clear();
	_lastHPChange = 0;
	_lastBulletHitDamage = 0;
	_lastBulletHitTarget = 0;
	_lastBulletHitShooter = nullptr;
	_lastBulletFiredDamage = 0;
	_lastBulletFiredTarget = 0;
	_lastFollowerHit = nullptr;
	_lastObstacleHit = nullptr;

	// Create the player's script object, so SWMG_GetPlayer() has something to return.
	_player.object = std::make_unique<MinigameObject>(kMinigameObjectPlayer, 0, "");
}

void SwoopMinigame::initFollower(MinigameFollower &follower, const MinigameFollowerDesc &desc,
                                 MinigameObjectKind kind, size_t index) {

	follower.desc = desc;
	follower.object = std::make_unique<MinigameObject>(kind, index, followerName(desc));

	follower.hitPoints    = desc.hitPoints;
	follower.maxHitPoints = desc.maxHitPoints;
	follower.hadHitPoints = desc.hitPoints > 0;
	follower.alive        = true;
	follower.sphereRadius = desc.sphereRadius;
	follower.numLoops     = desc.numLoops;

	follower.gunBanks.clear();
	for (const MinigameGunBankDesc &bank : desc.gunBanks) {
		MinigameGunBank state;
		state.desc = bank;
		follower.gunBanks.push_back(state);
	}
}

void SwoopMinigame::start(const MinigameDesc &desc, const std::vector<Common::UString> &obstacleNames) {
	reset();

	_desc = desc;
	_active = true;

	initFollower(_player, desc.player, kMinigameObjectPlayer, 0);

	for (size_t i = 0; i < desc.enemies.size(); ++i) {
		_enemies.push_back(std::make_unique<MinigameFollower>());
		initFollower(*_enemies.back(), desc.enemies[i], kMinigameObjectEnemy, i);
	}

	for (size_t i = 0; i < obstacleNames.size(); ++i) {
		std::unique_ptr<MinigameObstacle> obstacle = std::make_unique<MinigameObstacle>();

		obstacle->desc.name = obstacleNames[i];
		for (const MinigameObstacleDesc &obstacleDesc : desc.obstacles) {
			if (obstacleDesc.name.equalsIgnoreCase(obstacleNames[i])) {
				obstacle->desc = obstacleDesc;
				break;
			}
		}

		// No bounds until the runtime knows where the obstacle is.
		obstacle->boundsMin = glm::vec3( 1.0f);
		obstacle->boundsMax = glm::vec3(-1.0f);

		obstacle->object = std::make_unique<MinigameObject>(kMinigameObjectObstacle, i, obstacleNames[i]);
		_obstacles.push_back(std::move(obstacle));
	}

	_lateralAccel = desc.lateralAccel;
	_minSpeed     = desc.player.minSpeed;
	_maxSpeed     = desc.player.maxSpeed;
	_accelPerSec  = desc.player.accelSecs;

	_tunnelNeg      = desc.player.tunnelNeg;
	_tunnelPos      = desc.player.tunnelPos;
	_tunnelInfinite = desc.player.tunnelInfinite;

	_nearClip = desc.nearClip;
	_farClip  = desc.farClip;
}

// .--- Objects

MinigameObject *SwoopMinigame::getPlayer() {
	return _player.object.get();
}

MinigameObject *SwoopMinigame::getEnemy(size_t index) {
	return (index < _enemies.size()) ? _enemies[index]->object.get() : nullptr;
}

MinigameObject *SwoopMinigame::getObstacle(size_t index) {
	return (index < _obstacles.size()) ? _obstacles[index]->object.get() : nullptr;
}

MinigameFollower *SwoopMinigame::getEnemyState(size_t index) {
	return (index < _enemies.size()) ? _enemies[index].get() : nullptr;
}

MinigameObstacle *SwoopMinigame::getObstacleState(size_t index) {
	return (index < _obstacles.size()) ? _obstacles[index].get() : nullptr;
}

MinigameObject *SwoopMinigame::getObjectByName(const Common::UString &name) {
	if (name.empty())
		return nullptr;

	for (std::unique_ptr<MinigameObstacle> &obstacle : _obstacles)
		if (obstacle->object->getTag().equalsIgnoreCase(name))
			return obstacle->object.get();

	for (std::unique_ptr<MinigameFollower> &enemy : _enemies)
		if (enemy->object->getTag().equalsIgnoreCase(name))
			return enemy->object.get();

	if (_player.object && _player.object->getTag().equalsIgnoreCase(name))
		return _player.object.get();

	return nullptr;
}

MinigameObject *SwoopMinigame::toMinigameObject(Aurora::NWScript::Object *object) {
	MinigameObject *minigameObject = dynamic_cast<MinigameObject *>(object);
	if (!minigameObject)
		return nullptr;

	// Only objects of the current minigame.
	switch (minigameObject->getMinigameKind()) {
		case kMinigameObjectPlayer:
			return (minigameObject == _player.object.get()) ? minigameObject : nullptr;

		case kMinigameObjectEnemy:
			return (getEnemy(minigameObject->getMinigameIndex()) == minigameObject) ? minigameObject : nullptr;

		case kMinigameObjectObstacle:
			return (getObstacle(minigameObject->getMinigameIndex()) == minigameObject) ? minigameObject : nullptr;
	}

	return nullptr;
}

MinigameFollower *SwoopMinigame::getFollower(Aurora::NWScript::Object *object) {
	MinigameObject *minigameObject = toMinigameObject(object);
	if (!minigameObject)
		return nullptr;

	if (minigameObject->getMinigameKind() == kMinigameObjectPlayer)
		return &_player;
	if (minigameObject->getMinigameKind() == kMinigameObjectEnemy)
		return getEnemyState(minigameObject->getMinigameIndex());

	return nullptr;
}

MinigameObstacle *SwoopMinigame::getObstacleOf(Aurora::NWScript::Object *object) {
	MinigameObject *minigameObject = toMinigameObject(object);
	if (!minigameObject || (minigameObject->getMinigameKind() != kMinigameObjectObstacle))
		return nullptr;

	return getObstacleState(minigameObject->getMinigameIndex());
}

Common::UString SwoopMinigame::getObjectName(Aurora::NWScript::Object *object) {
	MinigameObject *minigameObject = toMinigameObject(object);
	if (minigameObject)
		return minigameObject->getTag();

	Object *kotorObject = ObjectContainer::toObject(object);
	return kotorObject ? kotorObject->getTag() : Common::UString();
}

bool SwoopMinigame::isTrigger(Aurora::NWScript::Object *object) {
	MinigameObject *minigameObject = toMinigameObject(object);
	if (!minigameObject || (minigameObject->getMinigameKind() != kMinigameObjectEnemy))
		return false;

	MinigameFollower *enemy = getEnemyState(minigameObject->getMinigameIndex());
	return enemy && enemy->desc.trigger;
}

const MinigameScripts *SwoopMinigame::getScripts(MinigameObject &object) {
	switch (object.getMinigameKind()) {
		case kMinigameObjectPlayer:
			return &_player.desc.scripts;

		case kMinigameObjectEnemy: {
			MinigameFollower *enemy = getEnemyState(object.getMinigameIndex());
			return enemy ? &enemy->desc.scripts : nullptr;
		}

		case kMinigameObjectObstacle: {
			MinigameObstacle *obstacle = getObstacleState(object.getMinigameIndex());
			return obstacle ? &obstacle->desc.scripts : nullptr;
		}
	}

	return nullptr;
}

// '---

// .--- Per-frame simulation

void SwoopMinigame::step(float dt) {
	if (!_active || (dt <= 0.0f))
		return;

	auto tickFollower = [dt](MinigameFollower &follower) {
		follower.invulnerability = std::max(0.0f, follower.invulnerability - dt);

		for (MinigameGunBank &bank : follower.gunBanks)
			bank.fireTimer = std::max(0.0f, bank.fireTimer - dt);
	};

	tickFollower(_player);
	for (std::unique_ptr<MinigameFollower> &enemy : _enemies)
		tickFollower(*enemy);

	for (std::unique_ptr<MinigameObstacle> &obstacle : _obstacles)
		obstacle->invulnerability = std::max(0.0f, obstacle->invulnerability - dt);

	movePlayer(dt);
	moveBullets(dt);

	checkDeath(_player);
	for (std::unique_ptr<MinigameFollower> &enemy : _enemies)
		checkDeath(*enemy);
}

void SwoopMinigame::movePlayer(float dt) {
	if (!_player.alive)
		return;

	glm::vec3 offset = _offset;

	if (isSwoop()) {
		// ModuleMGPlayer.update(): speed starts at the minimum and grows by
		// the acceleration up to the maximum; steering moves sideways at the
		// lateral acceleration.
		if ((_minSpeed != 0.0f) || (_speed != 0.0f)) {
			if (_speed < _minSpeed)
				_speed = _minSpeed;

			_speed += _accelPerSec * dt;

			if ((_maxSpeed != 0.0f) && (_speed >= _maxSpeed))
				_speed = _maxSpeed;
		}

		float lateral = 0.0f;
		if (_input.left && !_input.right)
			lateral = -_lateralAccel;
		else if (_input.right && !_input.left)
			lateral = _lateralAccel;

		offset.x += lateral * dt;
		offset.y += _speed * dt;

	} else if (_desc.type == kMinigameTypeTurret) {
		// Up/down pitch around X, left/right turn around Z.
		const float turn = kTurretTurnRate * dt;

		float vertical = 0.0f;
		if (_input.up && !_input.down)
			vertical = turn;
		else if (_input.down && !_input.up)
			vertical = -turn;

		if (_reverseVertical)
			vertical = -vertical;

		offset.x += vertical;

		if (_input.left && !_input.right)
			offset.z += turn;
		else if (_input.right && !_input.left)
			offset.z -= turn;
	}

	setPlayerOffset(offset);
}

void SwoopMinigame::moveBullets(float dt) {
	for (std::list<MinigameBullet>::iterator b = _bullets.begin(); b != _bullets.end(); ) {
		if (b->spent) {
			b = _bullets.erase(b);
			continue;
		}

		b->age += dt;
		if (b->age >= b->lifespan) {
			b = _bullets.erase(b);
			continue;
		}

		b->previous  = b->position;
		b->position += b->direction * (b->speed * dt);
		++b;
	}
}

void SwoopMinigame::setFollowerPose(MinigameObject &object, const glm::vec3 &position, const glm::vec3 &forward) {
	MinigameFollower *follower = getFollower(&object);
	if (!follower)
		return;

	follower->lastPosition = follower->hasPosition ? follower->position : position;
	follower->position     = position;
	follower->hasPosition  = true;

	if (glm::length(forward) > 0.0f)
		follower->forward = glm::normalize(forward);

	object.setPosition(position.x, position.y, position.z);
}

void SwoopMinigame::setObstacleBounds(size_t index, const glm::vec3 &min, const glm::vec3 &max) {
	MinigameObstacle *obstacle = getObstacleState(index);
	if (!obstacle)
		return;

	obstacle->boundsMin = min;
	obstacle->boundsMax = max;

	const glm::vec3 center = (min + max) * 0.5f;
	obstacle->object->setPosition(center.x, center.y, center.z);
}

glm::vec3 SwoopMinigame::collisionPoint(const glm::vec3 &position) const {
	// A swoop race runs on the track plane: what matters is where along the
	// track and how far to the side, not at which height the models sit.
	if (isSwoop())
		return glm::vec3(position.x, position.y, 0.0f);

	return position;
}

std::vector<MinigameHit> SwoopMinigame::detectHits() {
	std::vector<MinigameHit> hits;
	if (!_active)
		return hits;

	if (_player.alive && _player.hasPosition) {
		const glm::vec3 from = collisionPoint(_player.lastPosition);
		const glm::vec3 to   = collisionPoint(_player.position);
		const float radius   = followerRadius(_player);

		for (std::unique_ptr<MinigameObstacle> &obstacle : _obstacles) {
			if (!hasBounds(*obstacle))
				continue;

			glm::vec3 min = obstacle->boundsMin, max = obstacle->boundsMax;
			if (isSwoop()) {
				min.z = -1.0f;
				max.z =  1.0f;
			}

			const bool touching = segmentHitsBox(from, to, min, max, radius);
			const MinigameObject *other = obstacle->object.get();

			if (!touching) {
				_player.contacts.erase(other);
			} else if (_player.contacts.insert(other).second) {
				MinigameHit hit;
				hit.type    = MinigameHit::kFollowerHitObstacle;
				hit.subject = _player.object.get();
				hit.other   = obstacle->object.get();
				hits.push_back(hit);
			}
		}

		// KotOR.js only lets the player bump into enemies (and triggers) in a race.
		if (isSwoop()) {
			for (std::unique_ptr<MinigameFollower> &enemy : _enemies) {
				if (!enemy->alive || !enemy->hasPosition)
					continue;

				const bool touching = segmentHitsSphere(from, to, collisionPoint(enemy->position),
				                                        radius + followerRadius(*enemy));
				const MinigameObject *other = enemy->object.get();

				if (!touching) {
					_player.contacts.erase(other);
				} else if (_player.contacts.insert(other).second) {
					MinigameHit hit;
					hit.type    = MinigameHit::kFollowerHitFollower;
					hit.subject = _player.object.get();
					hit.other   = enemy->object.get();
					hits.push_back(hit);
				}
			}
		}
	}

	// Player bullets hit enemies, enemy bullets hit the player.
	for (MinigameBullet &bullet : _bullets) {
		if (bullet.spent || !bullet.shooter)
			continue;

		const glm::vec3 &from = bullet.previous;
		const glm::vec3 &to   = bullet.position;

		auto test = [&](MinigameFollower &target) {
			if (!target.alive || !target.hasPosition || (target.object.get() == bullet.shooter))
				return false;

			if (!segmentHitsSphere(from, to, target.position, followerRadius(target)))
				return false;

			bullet.spent = true;

			MinigameHit hit;
			hit.type    = MinigameHit::kBulletHitFollower;
			hit.subject = target.object.get();
			hit.bullet  = bullet;
			hits.push_back(hit);
			return true;
		};

		if (bullet.shooter->getMinigameKind() == kMinigameObjectPlayer) {
			for (std::unique_ptr<MinigameFollower> &enemy : _enemies)
				if (!enemy->desc.trigger && test(*enemy))
					break;
		} else {
			test(_player);
		}
	}

	return hits;
}

void SwoopMinigame::dispatchHits(const std::vector<MinigameHit> &hits) {
	for (const MinigameHit &hit : hits) {
		if (!hit.subject || !_active)
			continue;

		switch (hit.type) {
			case MinigameHit::kFollowerHitObstacle:
				if (!hit.other)
					break;

				debugC(Common::kDebugEngineLogic, 1, "SWMG: \"%s\" hit obstacle \"%s\"",
				       hit.subject->getTag().c_str(), hit.other->getTag().c_str());

				_lastObstacleHit = hit.other;
				_lastFollowerHit = hit.subject;
				if (!runEvent(*hit.subject, kMinigameScriptOnHitObstacle))
					defaultObstacleHit(*hit.subject);

				_lastObstacleHit = hit.other;
				_lastFollowerHit = hit.subject;
				runEvent(*hit.other, kMinigameScriptOnHitFollower);
				break;

			case MinigameHit::kFollowerHitFollower:
				if (!hit.other)
					break;

				debugC(Common::kDebugEngineLogic, 1, "SWMG: \"%s\" hit follower \"%s\"",
				       hit.subject->getTag().c_str(), hit.other->getTag().c_str());

				_lastFollowerHit = hit.other;
				runEvent(*hit.subject, kMinigameScriptOnHitFollower);

				_lastFollowerHit = hit.subject;
				runEvent(*hit.other, kMinigameScriptOnHitFollower);
				break;

			case MinigameHit::kBulletHitFollower:
				_lastBulletHitDamage  = hit.bullet.damage;
				_lastBulletHitTarget  = hit.bullet.targetType;
				_lastBulletHitShooter = hit.bullet.shooter;

				if (!runEvent(*hit.subject, kMinigameScriptOnHitBullet))
					defaultBulletHit(*hit.subject);
				break;
		}
	}
}

std::vector<MinigameFireRequest> SwoopMinigame::getEnemyFireRequests() {
	std::vector<MinigameFireRequest> requests;
	if (!_active || !_player.alive || !_player.hasPosition)
		return requests;

	for (std::unique_ptr<MinigameFollower> &enemy : _enemies) {
		if (!enemy->alive || !enemy->hasPosition || enemy->desc.trigger)
			continue;

		for (size_t i = 0; i < enemy->gunBanks.size(); ++i) {
			MinigameGunBank &bank = enemy->gunBanks[i];

			const float range = bank.desc.bullet.speed * bank.desc.bullet.lifespan;
			bank.targeting = rayHitsSphere(enemy->position, enemy->forward, _player.position,
			                               followerRadius(_player), range);

			if (bank.targeting && (bank.fireTimer <= 0.0f)) {
				MinigameFireRequest request;
				request.shooter = enemy->object.get();
				request.bank    = i;
				requests.push_back(request);
			}
		}
	}

	return requests;
}

bool SwoopMinigame::fire(MinigameObject &shooter, size_t bankIndex, const glm::vec3 &origin,
                         const glm::vec3 &direction) {

	MinigameFollower *follower = getFollower(&shooter);
	if (!follower || !follower->alive || (bankIndex >= follower->gunBanks.size()))
		return false;

	MinigameGunBank &bank = follower->gunBanks[bankIndex];
	if (bank.fireTimer > 0.0f)
		return false;

	bank.fireTimer = bank.desc.bullet.rateOfFire;

	_lastBulletFiredDamage = bank.desc.bullet.damage;
	_lastBulletFiredTarget = bank.desc.bullet.targetType;

	if (_host && !bank.desc.fireSound.empty())
		_host->playSound(bank.desc.fireSound, &shooter);

	if ((bank.desc.bullet.speed <= 0.0f) || (bank.desc.bullet.lifespan <= 0.0f) ||
	    (glm::length(direction) <= 0.0f))
		return true;

	MinigameBullet bullet;
	bullet.id         = _nextBulletID++;
	bullet.shooter    = &shooter;
	bullet.bank       = bankIndex;
	bullet.position   = origin;
	bullet.previous   = origin;
	bullet.direction  = glm::normalize(direction);
	bullet.speed      = bank.desc.bullet.speed;
	bullet.lifespan   = bank.desc.bullet.lifespan;
	bullet.damage     = bank.desc.bullet.damage;
	bullet.targetType = bank.desc.bullet.targetType;

	_bullets.push_back(bullet);
	return true;
}

// '---

// .--- Script events

bool SwoopMinigame::runEvent(MinigameObject &object, MinigameScript script) {
	const MinigameScripts *scripts = getScripts(object);
	if (!scripts || (script < 0) || (script >= kMinigameScriptMAX))
		return false;

	const Common::UString &resRef = (*scripts)[script];
	if (resRef.empty())
		return false;

	if (_host)
		_host->runScript(resRef, object);

	return true;
}

void SwoopMinigame::runCreateEvents() {
	for (size_t i = 0; i < _enemies.size(); ++i)
		runEvent(*_enemies[i]->object, kMinigameScriptOnCreate);

	for (size_t i = 0; i < _obstacles.size(); ++i)
		runEvent(*_obstacles[i]->object, kMinigameScriptOnCreate);

	runEvent(*_player.object, kMinigameScriptOnCreate);
}

void SwoopMinigame::runHeartbeats() {
	if (!_active)
		return;

	runEvent(*_player.object, kMinigameScriptOnHeartbeat);

	for (size_t i = 0; i < _enemies.size() && _active; ++i)
		if (_enemies[i]->alive)
			runEvent(*_enemies[i]->object, kMinigameScriptOnHeartbeat);

	for (size_t i = 0; i < _obstacles.size() && _active; ++i)
		runEvent(*_obstacles[i]->object, kMinigameScriptOnHeartbeat);
}

void SwoopMinigame::onTrackLoop(MinigameObject &follower) {
	runEvent(follower, kMinigameScriptOnTrackLoop);
}

void SwoopMinigame::onAccelerate() {
	runEvent(*_player.object, kMinigameScriptOnAccelerate);
}

void SwoopMinigame::onBrake() {
	runEvent(*_player.object, kMinigameScriptOnBrake);
}

void SwoopMinigame::onFire(MinigameObject &shooter) {
	runEvent(shooter, kMinigameScriptOnFire);
}

void SwoopMinigame::playAnimation(MinigameObject &object, const Common::UString &animation, bool loop) {
	if (_host && !animation.empty())
		_host->playAnimation(object, animation, loop);
}

void SwoopMinigame::removeAnimation(MinigameObject &object, const Common::UString &animation) {
	if (_host && !animation.empty())
		_host->removeAnimation(object, animation);
}

// '---

// .--- Default behaviour

void SwoopMinigame::defaultBulletHit(MinigameObject &self) {
	damage(self, _lastBulletHitDamage);
}

void SwoopMinigame::defaultObstacleHit(MinigameObject &self) {
	MinigameFollower *follower = getFollower(&self);
	if (follower && (follower->desc.bumpDamage > 0))
		damage(self, follower->desc.bumpDamage);
}

void SwoopMinigame::defaultDamage(MinigameObject &self) {
	if (MinigameFollower *follower = getFollower(&self))
		checkDeath(*follower);
}

void SwoopMinigame::defaultDeath(MinigameObject &self) {
	MinigameFollower *follower = getFollower(&self);
	if (follower) {
		follower->alive = false;
		follower->hitPoints = std::min(follower->hitPoints, 0);
	}

	if (_host) {
		_host->playAnimation(self, kAnimationDie, false);
		_host->onDeath(self);

		if (follower && !follower->desc.deathSound.empty())
			_host->playSound(follower->desc.deathSound, &self);
	}
}

void SwoopMinigame::damage(MinigameObject &self, int amount) {
	MinigameFollower *follower = getFollower(&self);
	if (!follower || !follower->alive || (amount <= 0))
		return;

	if (follower->invulnerability > 0.0f)
		return;

	follower->hitPoints -= amount;
	_lastHPChange = -amount;

	if (_host)
		_host->playAnimation(self, kAnimationDamage, false);

	if (!runEvent(self, kMinigameScriptOnDamage))
		defaultDamage(self);

	checkDeath(*follower);
}

void SwoopMinigame::checkDeath(MinigameFollower &follower) {
	if (follower.hitPoints > 0)
		follower.hadHitPoints = true;

	if (!follower.alive || !follower.hadHitPoints || (follower.hitPoints > 0))
		return;

	follower.alive = false;

	debugC(Common::kDebugEngineLogic, 1, "SWMG: \"%s\" died", follower.object->getTag().c_str());

	if (!runEvent(*follower.object, kMinigameScriptOnDeath))
		defaultDeath(*follower.object);
}

// '---

// .--- Player movement

void SwoopMinigame::setPlayerOffset(const glm::vec3 &offset) {
	for (int axis = 0; axis < 3; ++axis)
		_offset[axis] = clampToTunnel(offset[axis], _tunnelNeg[axis], _tunnelPos[axis],
		                              _tunnelInfinite[axis] != 0.0f);
}

void SwoopMinigame::setTunnelPos(const glm::vec3 &bound) {
	_tunnelPos = bound;
	setPlayerOffset(_offset);
}

void SwoopMinigame::setTunnelNeg(const glm::vec3 &bound) {
	_tunnelNeg = bound;
	setPlayerOffset(_offset);
}

void SwoopMinigame::setTunnelInfinite(const glm::vec3 &infinite) {
	_tunnelInfinite = infinite;
	setPlayerOffset(_offset);
}

float SwoopMinigame::getTurretTurnRate() {
	return kTurretTurnRate;
}

void SwoopMinigame::setPlayerInvincibility(float seconds) {
	_player.invulnerability = std::max(0.0f, seconds);
}

float SwoopMinigame::getPlayerInvincibility() const {
	return _player.invulnerability;
}

// '---

// .--- Per-object state

int SwoopMinigame::getHitPoints(Aurora::NWScript::Object *object) {
	if (MinigameFollower *follower = getFollower(object))
		return follower->hitPoints;
	if (MinigameObstacle *obstacle = getObstacleOf(object))
		return obstacle->hitPoints;

	return 0;
}

int SwoopMinigame::getMaxHitPoints(Aurora::NWScript::Object *object) {
	if (MinigameFollower *follower = getFollower(object))
		return follower->maxHitPoints;

	return 0;
}

void SwoopMinigame::setMaxHitPoints(Aurora::NWScript::Object *object, int hp) {
	if (MinigameFollower *follower = getFollower(object))
		follower->maxHitPoints = hp;
}

int SwoopMinigame::adjustHitPoints(Aurora::NWScript::Object *object, int hp, bool absolute) {
	if (MinigameFollower *follower = getFollower(object)) {
		const int before = follower->hitPoints;

		follower->hitPoints = absolute ? hp : (follower->hitPoints + hp);
		if ((follower->maxHitPoints > 0) && (follower->hitPoints > follower->maxHitPoints))
			follower->hitPoints = follower->maxHitPoints;

		_lastHPChange = follower->hitPoints - before;
		if (follower->hitPoints > 0)
			follower->hadHitPoints = true;

		return follower->hitPoints;
	}

	if (MinigameObstacle *obstacle = getObstacleOf(object)) {
		const int before = obstacle->hitPoints;

		obstacle->hitPoints = absolute ? hp : (obstacle->hitPoints + hp);
		_lastHPChange = obstacle->hitPoints - before;

		return obstacle->hitPoints;
	}

	return 0;
}

float SwoopMinigame::getSphereRadius(Aurora::NWScript::Object *object) {
	MinigameFollower *follower = getFollower(object);
	return follower ? follower->sphereRadius : 0.0f;
}

void SwoopMinigame::setSphereRadius(Aurora::NWScript::Object *object, float radius) {
	if (MinigameFollower *follower = getFollower(object))
		follower->sphereRadius = radius;
}

int SwoopMinigame::getNumLoops(Aurora::NWScript::Object *object) {
	MinigameFollower *follower = getFollower(object);
	return follower ? follower->numLoops : 0;
}

void SwoopMinigame::setNumLoops(Aurora::NWScript::Object *object, int loops) {
	if (MinigameFollower *follower = getFollower(object))
		follower->numLoops = loops;
}

glm::vec3 SwoopMinigame::getPosition(Aurora::NWScript::Object *object) {
	if (MinigameFollower *follower = getFollower(object))
		return follower->position;

	if (MinigameObstacle *obstacle = getObstacleOf(object))
		if (hasBounds(*obstacle))
			return (obstacle->boundsMin + obstacle->boundsMax) * 0.5f;

	return glm::vec3(0.0f);
}

bool SwoopMinigame::isInvulnerable(Aurora::NWScript::Object *object) {
	if (MinigameFollower *follower = getFollower(object))
		return follower->invulnerability > 0.0f;
	if (MinigameObstacle *obstacle = getObstacleOf(object))
		return obstacle->invulnerability > 0.0f;

	return false;
}

void SwoopMinigame::startInvulnerability(Aurora::NWScript::Object *object) {
	if (MinigameFollower *follower = getFollower(object))
		follower->invulnerability = std::max(follower->invulnerability, follower->desc.invincePeriod);
}

MinigameGunBank *SwoopMinigame::getGunBank(Aurora::NWScript::Object *object, int bank) {
	MinigameFollower *follower = getFollower(object);
	if (!follower || (bank < 0) || (static_cast<size_t>(bank) >= follower->gunBanks.size()))
		return nullptr;

	return &follower->gunBanks[bank];
}

int SwoopMinigame::getGunBankCount(Aurora::NWScript::Object *object) {
	MinigameFollower *follower = getFollower(object);
	return follower ? static_cast<int>(follower->gunBanks.size()) : 0;
}

MinigameSoundSettings &SwoopMinigame::getSound(Aurora::NWScript::Object *object, int sound) {
	MinigameFollower *follower = getFollower(object);
	if (!follower) {
		_noSound = MinigameSoundSettings();
		return _noSound;
	}

	return follower->sounds[sound];
}

// '---

void SwoopMinigame::setCameraClip(float nearClip, float farClip) {
	_nearClip = nearClip;
	_farClip  = farClip;

	if (_host)
		_host->onCameraClipChanged();
}

void SwoopMinigame::setLastAnimEvent(const Common::UString &event, const Common::UString &modelName) {
	_lastEvent = event;
	_lastEventModelName = modelName;
}

} // End of namespace KotORBase

} // End of namespace Engines
