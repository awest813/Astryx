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
 *  Swoop racing minigame state (SWMG) for KotOR games.
 */

#include <algorithm>

#include "src/engines/kotorbase/swoopminigame.h"

namespace Engines {

namespace KotORBase {

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
	_tunnelPos = 1.0f;
	_tunnelNeg = -1.0f;
	_lastEvent = 0;
}

void SwoopMinigame::setActive(bool active) {
	_active = active;
	if (!active)
		_lateralVelocity = 0.0f;
}

void SwoopMinigame::update(float frameTime) {
	if (!_active || frameTime <= 0.0f)
		return;

	if (_speed < _maxSpeed)
		_speed = std::min(_maxSpeed, _speed + frameTime * 10.0f);

	_offset += _lateralVelocity * frameTime;
	_lateralVelocity *= std::max(0.0f, 1.0f - frameTime * 2.5f);

	if (_offset > _tunnelPos) {
		_offset = _tunnelPos;
		_lateralVelocity = 0.0f;
	} else if (_offset < _tunnelNeg) {
		_offset = _tunnelNeg;
		_lateralVelocity = 0.0f;
	}
}

void SwoopMinigame::setLateralAcceleration(float accel) {
	_lateralAccel = accel;
}

void SwoopMinigame::setSpeed(float speed) {
	_speed = speed;
}

void SwoopMinigame::setMaxSpeed(float maxSpeed) {
	_maxSpeed = maxSpeed;
	if (_speed > _maxSpeed)
		_speed = _maxSpeed;
}

void SwoopMinigame::setPlayerOffset(float offset) {
	_offset = offset;
	if (_offset > _tunnelPos)
		_offset = _tunnelPos;
	if (_offset < _tunnelNeg)
		_offset = _tunnelNeg;
}

void SwoopMinigame::setTunnelPos(float bound) {
	_tunnelPos = bound;
	if (_offset > _tunnelPos)
		_offset = _tunnelPos;
}

void SwoopMinigame::setTunnelNeg(float bound) {
	_tunnelNeg = bound;
	if (_offset < _tunnelNeg)
		_offset = _tunnelNeg;
}

void SwoopMinigame::onObstacleHit() {
	_lastEvent = 1;
	_speed = std::max(0.0f, _speed * 0.6f);
}

} // End of namespace KotORBase

} // End of namespace Engines
