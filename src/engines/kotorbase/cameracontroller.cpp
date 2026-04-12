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

void CameraController::setPitch(float value) {
	_pitch = value;
	_dirty = true;
}

void CameraController::setDistance(float value) {
	_distance = value;
	_actualDistance = value;
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
		// Priority 1: Transition Blending
		Object *lookAt = _cameraTarget ? _cameraTarget : _cinematicFocus;
		if (lookAt && _transitionDuration > 0.0f && _transitionTime < _transitionDuration) {
			_transitionTime += frameTime;
			float t = _transitionTime / _transitionDuration;
			if (t >= 1.0f) {
				t = 1.0f;
				_transitionDuration = 0.0f; // Done
			}

			float tx, ty, tz;
			lookAt->getPosition(tx, ty, tz);
			float dx = tx - CameraMan.getX();
			float dy = ty - CameraMan.getY();
			float destYaw = atan2(dy, dx);
			
			// Simple lerp (normalized for wrap-around)
			float diff = destYaw - _sourceYaw;
			while (diff < -M_PI) diff += 2.0f * M_PI;
			while (diff > M_PI) diff -= 2.0f * M_PI;
			
			_yaw = _sourceYaw + diff * t;
		} else if (lookAt) {
			// Instant Look-at
			float tx, ty, tz;
			lookAt->getPosition(tx, ty, tz);
			float dx = tx - CameraMan.getX();
			float dy = ty - CameraMan.getY();
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
		if (_holding) {
			_holdTime -= frameTime;
			if (_holdTime <= 0.0f)
				_holding = false;
			return;
		}

		if (_pathEnd) {
			_pathTime += frameTime;
			float t = _pathTime / _pathDuration;
			if (t >= 1.0f) {
				t = 1.0f;
				// Arrival logic: stay at end point
				float ex, ey, ez;
				_pathEnd->getPosition(ex, ey, ez);
				_target = glm::vec3(ex, ey, ez);
				_pathEnd = nullptr;
			} else {
				float x1, y1, z1, x2, y2, z2;
				_pathStart->getPosition(x1, y1, z1);
				_pathEnd->getPosition(x2, y2, z2);
				
				_target.x = x1 + (x2 - x1) * t;
				_target.y = y1 + (y2 - y1) * t;
				_target.z = z1 + (z2 - z1) * t;
			}
			_dirty = true;
		} else if (_cinematicFocus) {
			float fx, fy, fz;
			_cinematicFocus->getPosition(fx, fy, fz);
			_target = glm::vec3(fx, fy, fz + 1.2f);
			_dirty = true;
		}

		CameraMan.setPosition(_target.x, _target.y, _target.z);
		CameraMan.setDistance(_actualDistance);
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

	if (_shakeTime > 0.0f) {
		_shakeTime -= frameTime;
		// Pseudo-random jitter
		actualPosition.x += ((rand() % 100) / 50.0f - 1.0f) * _shakeIntensity;
		actualPosition.y += ((rand() % 100) / 50.0f - 1.0f) * _shakeIntensity;
		actualPosition.z += ((rand() % 100) / 50.0f - 1.0f) * _shakeIntensity;
	}

	CameraMan.setPosition(actualPosition.x, actualPosition.y, actualPosition.z);
	CameraMan.update();
}

void CameraController::shake(float duration, float intensity) {
	_shakeTime = duration;
	_shakeIntensity = intensity;
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
	_cinematicFocus = nullptr;
	_cinematicID = cameraID;
	_cinematicAngle = cameraAngle;
	_cinematicModel = cameraModel;

	Area *area = _module->getCurrentArea();
	if (area) {
		const Area::Camera *staticCam = area->getCamera(cameraID);
		if (staticCam) {
			_target = glm::vec3(staticCam->position[0], staticCam->position[1], staticCam->position[2]);
			
			// Extract yaw/pitch if possible. 
			_yaw = Common::deg2rad(cameraAngle); 
			_pitch = staticCam->pitch;
			_distance = 0.0f;
			_dirty = true;
			return;
		}
	}

	_yaw = Common::deg2rad(cameraAngle);
	_dirty = true;
}

void CameraController::setCameraMode(CameraMode mode, Object *target) {
	_cinematic = true;
	_cinematicFocus = target;
	_dirty = true;

	switch (mode) {
	case kCameraModeCloseup:
		_pitch = -0.5f;
		_distance = 1.0f;
		break;
	case kCameraModeMedium:
		_pitch = -0.3f;
		_distance = 4.0f;
		break;
	case kCameraModeWide:
		_pitch = -0.2f;
		_distance = 8.0f;
		break;
	case kCameraModeIsometric:
		_pitch = -1.0f;
		_distance = 15.0f;
		break;
	case kCameraModeReveal:
		_pitch = -0.15f;
		_distance = 25.0f;
		break;
	}
}

void CameraController::setCinematicFocus(Object *target) {
	_cinematicFocus = target;
	_dirty = true;
}

void CameraController::setCameraTarget(Object *target) {
	_cameraTarget = target;
	_dirty = true;
}

void CameraController::cameraTransitionToTarget(float duration) {
	_sourceYaw = _yaw;
	_sourcePitch = _pitch;
	_transitionTime = 0.0f;
	_transitionDuration = duration > 0.0f ? duration : 0.001f;
}

void CameraController::cameraMoveAlongPath(Object *start, Object *end, float duration) {
	_pathStart = start;
	_pathEnd = end;
	_pathTime = 0.0f;
	_pathDuration = duration > 0.0f ? duration : 1.0f;
}

void CameraController::cameraHold(float duration) {
	_holding = true;
	_holdTime = duration;
}

void CameraController::restoreGameplayCamera(float blendTime) {
	// For now, snap back. Blending would require a transition state.
	resetToOrbit();
}

void CameraController::resetToOrbit() {
	_cinematic = false;
	_cinematicFocus = nullptr;
	_cameraTarget = nullptr;
	_pathStart = nullptr;
	_pathEnd = nullptr;
	_transitionDuration = 0.0f;
	_holding = false;
	
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
