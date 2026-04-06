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
 *  Handles camera movement in KotOR games.
 */

#include <cmath>
#include <cstdint>

#include "src/common/maths.h"
#include "src/common/util.h"

#include "src/graphics/camera.h"

#include "src/engines/aurora/flycamera.h"

#include "src/engines/kotorbase/cameracontroller.h"
#include "src/engines/kotorbase/module.h"
#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/area.h"

namespace Engines {

namespace KotORBase {

static const float kRotationSpeed = static_cast<float>(M_PI) / 2.0f;
static const float kMovementSpeed = 2.0f * static_cast<float>(M_PI);

CameraController::CameraController(Module *module) : _module(module) {
}

bool CameraController::isFlyCamera() const {
	return _flycam;
}

void CameraController::toggleFlyCamera() {
	_flycam = !_flycam;
}

float CameraController::getYaw() const {
	return _yaw;
}

void CameraController::setYaw(float value) {
	_yaw = value;
	_dirty = true;
}

void CameraController::updateTarget() {
	float x, y, z;
	Creature *partyLeader = _module->getPartyLeader();

	partyLeader->getPosition(x, y, z);

	_target = glm::vec3(x, y, z + partyLeader->getCameraHeight());
	_dirty = true;
}

void CameraController::updateCameraStyle() {
	const Area::CameraStyle &style = _module->getCurrentArea()->getCameraStyle();

	_distance = style.distance;
	_pitch = style.pitch;
	_height = style.height;

	GfxMan.setPerspective(style.viewAngle, 0.1f, 10000.0f);

	_dirty = true;
	_actualDistance = _distance;
}

bool CameraController::handleEvent(const Events::Event &e) {
	if (_flycam)
		return FlyCam.handleCameraInput(e);

	switch (e.type) {
		case Events::kEventKeyDown:
		case Events::kEventKeyUp:
			switch (e.key.keysym.scancode) {
				case SDL_SCANCODE_A:
					_counterClockwiseMovementWanted = (e.type == Events::kEventKeyDown);
					return true;

				case SDL_SCANCODE_D:
					_clockwiseMovementWanted = (e.type == Events::kEventKeyDown);
					return true;

				default:
					return false;
			}

		case Events::kEventControllerAxisMotion:
			switch (e.caxis.axis) {
				case Events::kControllerAxisLeftX:
				case Events::kControllerAxisRightX:
					_clockwiseMovementWanted = (e.caxis.value > 10000);
					_counterClockwiseMovementWanted = (e.caxis.value < -10000);
					return true;

				default:
					return false;
			}

		default:
			return false;
	}
}

void CameraController::processRotation(float frameTime) {
	if (_flycam)
		return;

	if (_cinematic) {
		// In cinematic mode, orientation is driven by the angle or the focus target.
		if (_cinematicFocus) {
			float tx, ty, tz;
			_cinematicFocus->getPosition(tx, ty, tz);
			float dx = tx - _target.x;
			float dy = ty - _target.y;
			_yaw = atan2(dy, dx);
		}
		CameraMan.setOrientation(_pitch, 0.0f, Common::rad2deg(_yaw));
		return;
	}

	if (shouldMoveClockwise()) {
		_yaw -= kRotationSpeed * frameTime;
		_yaw = fmodf(_yaw, 2.0f * static_cast<float>(M_PI));
	} else if (shouldMoveCounterClockwise()) {
		_yaw += kRotationSpeed * frameTime;
		_yaw = fmodf(_yaw, 2.0f * static_cast<float>(M_PI));
	}

	CameraMan.setOrientation(_pitch, 0.0f, Common::rad2deg(_yaw));
}

void CameraController::processMovement(float frameTime) {
	if (_flycam) {
		CameraMan.update();
		return;
	}

	if (_cinematic) {
		// If focusing on someone, update our anchor point.
		if (_cinematicFocus) {
			float fx, fy, fz;
			_cinematicFocus->getPosition(fx, fy, fz);
			_target = glm::vec3(fx, fy, fz + 1.2f); // focus slightly above feet
		}

		// Simple implementation: move camera to cinematic position
		glm::vec3 actualPosition = getCameraPosition(_distance);
		CameraMan.setPosition(actualPosition.x, actualPosition.y, actualPosition.z);
		CameraMan.update();
		return;
	}

	glm::vec3 expectedPosition = getCameraPosition(_distance);
	float expectedDistance = glm::distance(_target, expectedPosition);

	if (shouldMoveClockwise() || shouldMoveCounterClockwise() || _dirty) {
		glm::vec3 obstacle;
		if (_module->getCurrentArea()->rayTest(_target, expectedPosition, obstacle)) {
			_obstacleExists = true;
			_obstacleDistance = glm::distance(_target, obstacle);
		} else {
			_obstacleExists = false;
		}
		_dirty = false;
	}

	float delta = kMovementSpeed * frameTime;

	if (_obstacleExists) {
		if (_actualDistance >= _obstacleDistance) {
			_actualDistance = _obstacleDistance;
		} else if (_actualDistance + delta <= _obstacleDistance) {
			_actualDistance += delta;
		}
	} else if (_actualDistance + delta <= expectedDistance) {
		_actualDistance += delta;
	}

	// TODO: 3.5 is only an assumption for the max distance
	_actualDistance = MIN(_actualDistance, 3.5f);

	glm::vec3 actualPosition = getCameraPosition(_actualDistance);
	CameraMan.setPosition(actualPosition.x, actualPosition.y, actualPosition.z);
	CameraMan.update();
}

void CameraController::stopMovement() {
	_clockwiseMovementWanted = false;
	_counterClockwiseMovementWanted = false;
}

void CameraController::syncOrbitingCamera() {
	if (_flycam)
		return;
	if (!_module->getCurrentArea())
		return;

	processRotation(0.0f);
	processMovement(0.0f);
}

void CameraController::setCinematicCamera(uint32_t cameraID, float cameraAngle, const Common::UString &cameraModel) {
	_cinematic = true;
	_cinematicFocus = nullptr; // Clear any dynamic focus; setCinematicFocus must be called after if needed.
	_cinematicID = cameraID;
	_cinematicAngle = cameraAngle;
	_cinematicModel = cameraModel;

	// For now, let's just adjust the yaw based on cameraAngle
	_yaw = Common::deg2rad(cameraAngle);
	_dirty = true;
}

void CameraController::setCinematicFocus(Object *target) {
	_cinematicFocus = target;
	_dirty = true;
}

void CameraController::resetToOrbit() {
	_cinematic = false;
	_cinematicFocus = nullptr;
	updateTarget();
	_dirty = true;
}

glm::vec3 CameraController::getCameraPosition(float distance) const {
	glm::vec3 position;
	position.x = _target.x + distance * sin(_yaw);
	position.y = _target.y - distance * cos(_yaw);
	position.z = _target.z + _height;
	return position;
}

bool CameraController::shouldMoveClockwise() const {
	return _clockwiseMovementWanted && !_counterClockwiseMovementWanted;
}

bool CameraController::shouldMoveCounterClockwise() const {
	return _counterClockwiseMovementWanted && !_clockwiseMovementWanted;
}

} // End of namespace KotORBase

} // End of namespace Engines
