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

#ifndef ENGINES_KOTORBASE_SWOOPMINIGAME_H
#define ENGINES_KOTORBASE_SWOOPMINIGAME_H

#include <array>
#include <map>
#include <string>
#include <vector>

#include "src/common/ustring.h"

namespace Engines {

namespace KotORBase {

struct SWMGGunBank {
	Common::UString bulletModel;
	Common::UString gunModel;
	float damage { 10.0f };
	float timeBetweenShots { 0.5f };
	float lifespan { 2.0f };
	float speed { 50.0f };
	float horizontalSpread { 0.0f };
	float verticalSpread { 0.0f };
	float sensingRadius { 10.0f };
	float inaccuracy { 0.0f };
	Common::UString target;
};

/** Lightweight swoop/tunnel/turret simulation backing NWScript SWMG_* calls. */
class SwoopMinigame {
public:
	static SwoopMinigame &get();

	void reset();
	void setActive(bool active);
	bool isActive() const { return _active; }

	void update(float frameTime);

	void setLateralAcceleration(float accel);
	float getLateralAcceleration() const { return _lateralAccel; }

	void setSpeed(float speed);
	float getSpeed() const { return _speed; }

	void setMaxSpeed(float maxSpeed);
	float getMaxSpeed() const { return _maxSpeed; }

	void setMinSpeed(float minSpeed);
	float getMinSpeed() const { return _minSpeed; }

	void setAccelerationPerSecond(float accel);
	float getAccelerationPerSecond() const { return _accelPerSec; }

	void setPlayerOffset(float offset);
	float getPlayerOffset() const { return _offset; }

	void setTunnelPos(float bound);
	float getTunnelPos() const { return _tunnelPos; }

	void setTunnelNeg(float bound);
	float getTunnelNeg() const { return _tunnelNeg; }

	void setTunnelInfinite(bool infinite);
	bool getTunnelInfinite() const { return _tunnelInfinite; }

	void setPlayerOrigin(float x, float y, float z);
	void getPlayerOrigin(float &x, float &y, float &z) const;

	void setInvincible(bool invincible);
	bool isInvincible() const { return _invincible; }
	void startInvulnerability(float duration);
	bool isInvulnerable() const;

	void setHitPoints(float hp);
	float getHitPoints() const { return _hitPoints; }
	void setMaxHitPoints(float hp);
	float getMaxHitPoints() const { return _maxHitPoints; }
	void adjustFollowerHitPoints(float delta);
	float getFollowerHitPoints() const { return _followerHitPoints; }
	void setFollowerHitPoints(float hp);

	void setSphereRadius(float radius);
	float getSphereRadius() const { return _sphereRadius; }

	void setNumLoops(int loops);
	int getNumLoops() const { return _numLoops; }

	void setCameraClip(float nearClip, float farClip);
	float getCameraNearClip() const { return _cameraNear; }
	float getCameraFarClip() const { return _cameraFar; }

	void setSoundFrequency(float freq);
	float getSoundFrequency() const { return _soundFrequency; }
	void setSoundFrequencyIsRandom(bool random);
	bool getSoundFrequencyIsRandom() const { return _soundFrequencyRandom; }
	void setSoundVolume(float volume);
	float getSoundVolume() const { return _soundVolume; }

	void onObstacleHit();
	void onBulletHit(float damage);
	void onDamage(float amount);
	void onDeath();

	int getLastEvent() const { return _lastEvent; }
	void clearLastEvent() { _lastEvent = 0; }
	float getLastHPChange() const { return _lastHPChange; }
	float getLastBulletHitDamage() const { return _lastBulletHitDamage; }
	float getLastBulletFiredDamage() const { return _lastBulletFiredDamage; }
	const Common::UString &getLastEventModelName() const { return _lastEventModelName; }

	int getObstacleCount() const { return static_cast<int>(_obstacles.size()); }
	const Common::UString &getObstacle(int index) const;
	void setObjectName(const Common::UString &name);
	const Common::UString &getObjectName() const { return _objectName; }
	void registerNamedObject(const Common::UString &name, int kind); // 0 player 1 follower 2 enemy 3 trigger 4 obstacle
	int getObjectKind(const Common::UString &name) const;
	bool hasNamedObject(const Common::UString &name) const;

	int getGunBankCount() const { return static_cast<int>(_gunBanks.size()); }
	SWMGGunBank &getGunBank(int index);
	const SWMGGunBank &getGunBank(int index) const;

private:
	SwoopMinigame();

	bool  _active { false };
	float _offset { 0.0f };
	float _lateralVelocity { 0.0f };
	float _lateralAccel { 5.0f };
	float _speed { 0.0f };
	float _maxSpeed { 100.0f };
	float _minSpeed { 0.0f };
	float _accelPerSec { 10.0f };
	float _tunnelPos { 1.0f };
	float _tunnelNeg { -1.0f };
	bool  _tunnelInfinite { false };
	float _origin[3] { 0.0f, 0.0f, 0.0f };
	bool  _invincible { false };
	float _invulnTimer { 0.0f };
	float _hitPoints { 100.0f };
	float _maxHitPoints { 100.0f };
	float _followerHitPoints { 100.0f };
	float _sphereRadius { 1.0f };
	int   _numLoops { 0 };
	float _cameraNear { 0.1f };
	float _cameraFar { 1000.0f };
	float _soundFrequency { 1.0f };
	bool  _soundFrequencyRandom { false };
	float _soundVolume { 1.0f };
	int   _lastEvent { 0 };
	float _lastHPChange { 0.0f };
	float _lastBulletHitDamage { 0.0f };
	float _lastBulletFiredDamage { 0.0f };
	Common::UString _lastEventModelName;
	Common::UString _objectName;
	std::vector<Common::UString> _obstacles;
	std::map<Common::UString, int> _namedKinds;
	std::array<SWMGGunBank, 4> _gunBanks;
};

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_SWOOPMINIGAME_H
