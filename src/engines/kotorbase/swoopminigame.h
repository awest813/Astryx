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

#ifndef ENGINES_KOTORBASE_SWOOPMINIGAME_H
#define ENGINES_KOTORBASE_SWOOPMINIGAME_H

namespace Engines {

namespace KotORBase {

/** Lightweight swoop/tunnel racer simulation backing NWScript SWMG_* calls. */
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

	void setPlayerOffset(float offset);
	float getPlayerOffset() const { return _offset; }

	void setTunnelPos(float bound);
	float getTunnelPos() const { return _tunnelPos; }

	void setTunnelNeg(float bound);
	float getTunnelNeg() const { return _tunnelNeg; }

	void onObstacleHit();
	int getLastEvent() const { return _lastEvent; }
	void clearLastEvent() { _lastEvent = 0; }

private:
	SwoopMinigame();

	bool  _active { false };
	float _offset { 0.0f };
	float _lateralVelocity { 0.0f };
	float _lateralAccel { 5.0f };
	float _speed { 0.0f };
	float _maxSpeed { 100.0f };
	float _tunnelPos { 1.0f };
	float _tunnelNeg { -1.0f };
	int   _lastEvent { 0 };
};

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_SWOOPMINIGAME_H
