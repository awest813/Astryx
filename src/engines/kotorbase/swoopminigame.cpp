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
 *  Swoop racing / turret minigame state (SWMG) for KotOR games.
 */

#include <algorithm>

#include "src/engines/kotorbase/swoopminigame.h"

namespace Engines {

namespace KotORBase {

namespace {

const Common::UString kEmptyString;

} // End of anonymous namespace

SwoopMinigame &SwoopMinigame::get() {
	static SwoopMinigame instance;
	return instance;
}

SwoopMinigame::SwoopMinigame() {
	reset();
}

void SwoopMinigame::reset() {
	_active = false;
	_offset = 0.0f;
	_lateralVelocity = 0.0f;
	_lateralAccel = 5.0f;
	_speed = 0.0f;
	_maxSpeed = 100.0f;
	_minSpeed = 0.0f;
	_accelPerSec = 10.0f;
	_tunnelPos = 1.0f;
	_tunnelNeg = -1.0f;
	_tunnelInfinite = false;
	_origin[0] = _origin[1] = _origin[2] = 0.0f;
	_invincible = false;
	_invulnTimer = 0.0f;
	_hitPoints = 100.0f;
	_maxHitPoints = 100.0f;
	_followerHitPoints = 100.0f;
	_sphereRadius = 1.0f;
	_numLoops = 0;
	_cameraNear = 0.1f;
	_cameraFar = 1000.0f;
	_soundFrequency = 1.0f;
	_soundFrequencyRandom = false;
	_soundVolume = 1.0f;
	_lastEvent = 0;
	_lastHPChange = 0.0f;
	_lastBulletHitDamage = 0.0f;
	_lastBulletFiredDamage = 0.0f;
	_lastEventModelName.clear();
	_objectName.clear();
	_obstacles.clear();
	_namedKinds.clear();
	for (size_t i = 0; i < _gunBanks.size(); ++i)
		_gunBanks[i] = SWMGGunBank();
}

void SwoopMinigame::setActive(bool active) {
	_active = active;
	if (!active)
		_lateralVelocity = 0.0f;
}

void SwoopMinigame::update(float frameTime) {
	if (frameTime <= 0.0f)
		return;

	if (_invulnTimer > 0.0f)
		_invulnTimer = std::max(0.0f, _invulnTimer - frameTime);

	if (!_active)
		return;

	if (_speed < _maxSpeed)
		_speed = std::min(_maxSpeed, _speed + frameTime * _accelPerSec);
	if (_speed < _minSpeed)
		_speed = _minSpeed;

	_offset += _lateralVelocity * frameTime;
	_lateralVelocity *= std::max(0.0f, 1.0f - frameTime * 2.5f);

	if (!_tunnelInfinite) {
		if (_offset > _tunnelPos) {
			_offset = _tunnelPos;
			_lateralVelocity = 0.0f;
		} else if (_offset < _tunnelNeg) {
			_offset = _tunnelNeg;
			_lateralVelocity = 0.0f;
		}
	}
}

void SwoopMinigame::setLateralAcceleration(float accel) {
	_lateralAccel = accel;
	_lateralVelocity = accel;
}

void SwoopMinigame::setSpeed(float speed) {
	_speed = speed;
}

void SwoopMinigame::setMaxSpeed(float maxSpeed) {
	_maxSpeed = maxSpeed;
}

void SwoopMinigame::setMinSpeed(float minSpeed) {
	_minSpeed = minSpeed;
}

void SwoopMinigame::setAccelerationPerSecond(float accel) {
	_accelPerSec = accel;
}

void SwoopMinigame::setPlayerOffset(float offset) {
	_offset = offset;
	if (!_tunnelInfinite) {
		if (_offset > _tunnelPos)
			_offset = _tunnelPos;
		if (_offset < _tunnelNeg)
			_offset = _tunnelNeg;
	}
}

void SwoopMinigame::setTunnelPos(float bound) {
	_tunnelPos = bound;
	if (!_tunnelInfinite && _offset > _tunnelPos)
		_offset = _tunnelPos;
}

void SwoopMinigame::setTunnelNeg(float bound) {
	_tunnelNeg = bound;
	if (!_tunnelInfinite && _offset < _tunnelNeg)
		_offset = _tunnelNeg;
}

void SwoopMinigame::setTunnelInfinite(bool infinite) {
	_tunnelInfinite = infinite;
}

void SwoopMinigame::setPlayerOrigin(float x, float y, float z) {
	_origin[0] = x;
	_origin[1] = y;
	_origin[2] = z;
}

void SwoopMinigame::getPlayerOrigin(float &x, float &y, float &z) const {
	x = _origin[0];
	y = _origin[1];
	z = _origin[2];
}

void SwoopMinigame::setInvincible(bool invincible) {
	_invincible = invincible;
}

void SwoopMinigame::startInvulnerability(float duration) {
	_invulnTimer = duration;
}

bool SwoopMinigame::isInvulnerable() const {
	return _invincible || _invulnTimer > 0.0f;
}

void SwoopMinigame::setHitPoints(float hp) {
	_hitPoints = hp;
}

void SwoopMinigame::setMaxHitPoints(float hp) {
	_maxHitPoints = hp;
}

void SwoopMinigame::adjustFollowerHitPoints(float delta) {
	_followerHitPoints += delta;
	_lastHPChange = delta;
	_lastEvent = 3; // follower HP change
}

void SwoopMinigame::setFollowerHitPoints(float hp) {
	_lastHPChange = hp - _followerHitPoints;
	_followerHitPoints = hp;
}

void SwoopMinigame::setSphereRadius(float radius) {
	_sphereRadius = radius;
}

void SwoopMinigame::setNumLoops(int loops) {
	_numLoops = loops;
}

void SwoopMinigame::setCameraClip(float nearClip, float farClip) {
	_cameraNear = nearClip;
	_cameraFar = farClip;
}

void SwoopMinigame::setSoundFrequency(float freq) {
	_soundFrequency = freq;
}

void SwoopMinigame::setSoundFrequencyIsRandom(bool random) {
	_soundFrequencyRandom = random;
}

void SwoopMinigame::setSoundVolume(float volume) {
	_soundVolume = volume;
}

void SwoopMinigame::onObstacleHit() {
	_lastEvent = 1;
	_speed = std::max(0.0f, _speed * 0.6f);
	_lastEventModelName = _objectName;
	if (!_objectName.empty()) {
		_obstacles.push_back(_objectName);
		registerNamedObject(_objectName, 4);
	}
}

void SwoopMinigame::onBulletHit(float damage) {
	_lastEvent = 2;
	_lastBulletHitDamage = damage;
	if (!isInvulnerable()) {
		_lastHPChange = -damage;
		_hitPoints = std::max(0.0f, _hitPoints - damage);
	}
}

void SwoopMinigame::onDamage(float amount) {
	_lastEvent = 4;
	if (isInvulnerable())
		return;
	_lastHPChange = -amount;
	_hitPoints = std::max(0.0f, _hitPoints - amount);
}

void SwoopMinigame::onDeath() {
	_lastEvent = 5;
	_hitPoints = 0.0f;
}

const Common::UString &SwoopMinigame::getObstacle(int index) const {
	if (index < 0 || index >= static_cast<int>(_obstacles.size()))
		return kEmptyString;
	return _obstacles[static_cast<size_t>(index)];
}

void SwoopMinigame::setObjectName(const Common::UString &name) {
	_objectName = name;
}

void SwoopMinigame::registerNamedObject(const Common::UString &name, int kind) {
	if (!name.empty())
		_namedKinds[name] = kind;
}

int SwoopMinigame::getObjectKind(const Common::UString &name) const {
	std::map<Common::UString, int>::const_iterator it = _namedKinds.find(name);
	return (it == _namedKinds.end()) ? -1 : it->second;
}

bool SwoopMinigame::hasNamedObject(const Common::UString &name) const {
	return _namedKinds.find(name) != _namedKinds.end();
}

SWMGGunBank &SwoopMinigame::getGunBank(int index) {
	if (index < 0)
		index = 0;
	if (index >= static_cast<int>(_gunBanks.size()))
		index = static_cast<int>(_gunBanks.size()) - 1;
	return _gunBanks[static_cast<size_t>(index)];
}

const SWMGGunBank &SwoopMinigame::getGunBank(int index) const {
	if (index < 0)
		index = 0;
	if (index >= static_cast<int>(_gunBanks.size()))
		index = static_cast<int>(_gunBanks.size()) - 1;
	return _gunBanks[static_cast<size_t>(index)];
}

} // End of namespace KotORBase

} // End of namespace Engines
