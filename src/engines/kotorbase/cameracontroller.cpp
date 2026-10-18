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
#include <vector>
#include <cstdint>

#include "src/common/maths.h"
#include "src/common/util.h"
#include "src/common/random.h"

#include "external/glm/geometric.hpp"

#include "src/graphics/camera.h"
#include "src/graphics/graphics.h"
#include "src/graphics/aurora/model.h"
#include "src/graphics/aurora/modelnode.h"

#include "src/engines/aurora/model.h"

#include "src/engines/aurora/flycamera.h"

#include "src/engines/kotorbase/cameracontroller.h"
#include "src/engines/kotorbase/cinematicshot.h"
#include "src/engines/kotorbase/module.h"
#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/objectcontainer.h"
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
	if (!partyLeader)
		return;

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

	if (_restoringGameplay) {
		_restoreTime += frameTime;
		float t = _restoreTime / _restoreDuration;
		if (t >= 1.0f) {
			t = 1.0f;
			_restoringGameplay = false;
			resetToOrbit(true);
			return;
		}

		const float smoothT = t * t * (3.0f - 2.0f * t);

		float diff = _restoreDestYaw - _restoreStartYaw;
		while (diff < -static_cast<float>(M_PI))
			diff += 2.0f * static_cast<float>(M_PI);
		while (diff > static_cast<float>(M_PI))
			diff -= 2.0f * static_cast<float>(M_PI);

		_yaw = _restoreStartYaw + diff * smoothT;
		_pitch = _restoreStartPitch + (_restoreDestPitch - _restoreStartPitch) * smoothT;

		CameraMan.setOrientation(_pitch, 0.0f, Common::rad2deg(_yaw));
		return;
	}

	// Cinematic shots are placed (position and orientation) in processMovement().
	if (_cinematic)
		return;

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

	if (_restoringGameplay) {
		float t = _restoreTime / _restoreDuration;
		if (t > 1.0f)
			t = 1.0f;

		const float smoothT = t * t * (3.0f - 2.0f * t);

		_target.x = _restoreStartTarget.x + (_restoreDestTarget.x - _restoreStartTarget.x) * smoothT;
		_target.y = _restoreStartTarget.y + (_restoreDestTarget.y - _restoreStartTarget.y) * smoothT;
		_target.z = _restoreStartTarget.z + (_restoreDestTarget.z - _restoreStartTarget.z) * smoothT;

		const float dist = _restoreStartDistance + (_restoreDestDistance - _restoreStartDistance) * smoothT;
		_distance = dist;
		_actualDistance = dist;

		const glm::vec3 camPos = getCameraPosition(dist);
		CameraMan.setPosition(camPos.x, camPos.y, camPos.z);
		CameraMan.update();
		return;
	}

	if (_cinematic) {
		updateCinematic(frameTime);
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

	float maxDistance = 3.5f;
	if (Area *area = _module->getCurrentArea()) {
		const Area::CameraStyle &style = area->getCameraStyle();
		if (style.distance > 0.0f)
			maxDistance = style.distance;
	}
	_actualDistance = MIN(_actualDistance, maxDistance);

	glm::vec3 actualPosition = getCameraPosition(_actualDistance);

	if (_shakeTime > 0.0f) {
		_shakeTime -= frameTime;
		// Pseudo-random jitter in [-1, 1] * intensity, using the engine RNG.
		actualPosition.x += RNG.getNext(-1.0f, 1.0f) * _shakeIntensity;
		actualPosition.y += RNG.getNext(-1.0f, 1.0f) * _shakeIntensity;
		actualPosition.z += RNG.getNext(-1.0f, 1.0f) * _shakeIntensity;
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

void CameraController::beginCinematic() {
	if (_cinematic)
		return;

	// Remember the gameplay camera to return to.
	_orbitYaw = _yaw;
	_cinematic = true;
	_restoringGameplay = false;
}

void CameraController::startShot(ShotType type, float blendTime) {
	beginCinematic();

	if (type != kShotAnimated)
		releaseAnimatedCamera();

	// A new shot supersedes a running restore to the gameplay camera.
	_restoringGameplay = false;

	// Paths only drive scripted focus shots.
	if (type != kShotFocus) {
		_pathStart = nullptr;
		_pathEnd = nullptr;
	}

	// Coming from the gameplay camera: blend from what is on screen.
	if (!_haveView) {
		const float *position = CameraMan.getPosition();
		const float pitch = Common::deg2rad(_pitch);

		_viewPosition  = glm::vec3(position[0], position[1], position[2]);
		_viewDirection = glm::vec3(-std::sin(_yaw) * std::sin(pitch),
		                            std::cos(_yaw) * std::sin(pitch),
		                           -std::cos(pitch));
		_haveView = true;
	}

	// A field of view belongs to the shot that set it.
	if (_shotFieldOfView != 0.0f)
		setShotFieldOfView(0.0f);

	_shotType = type;
	_holding = false;

	_blendTime = 0.0f;
	_blendDuration = (_haveView && blendTime > 0.0f) ? blendTime : 0.0f;
	_blendFromPosition  = _viewPosition;
	_blendFromDirection = _viewDirection;

	_dirty = true;
}

void CameraController::setDialogShot(int cameraAngle, Object *speaker, Object *listener) {
	startShot(kShotDialog);

	_dialogAngle    = cameraAngle;
	_dialogSpeaker  = speaker;
	_dialogListener = listener;
}

bool CameraController::setStaticShot(uint32_t cameraID) {
	Area *area = _module->getCurrentArea();
	const Area::Camera *camera = area ? area->getCamera(cameraID) : nullptr;
	if (!camera)
		return false;

	startShot(kShotStatic);

	const CinematicShot shot = computeStaticShot(camera->position, camera->orientation,
	                                             camera->height, camera->pitch);
	_staticPosition  = shot.position;
	_staticDirection = shot.direction;

	setShotFieldOfView(camera->fieldOfView);
	return true;
}

bool CameraController::setAnimatedShot(const Common::UString &cameraModel, uint32_t cameraAnimation) {
	if (cameraModel.empty() || cameraAnimation == 0)
		return false;

	// Camera animations are named after their DLG number, offset by 1200.
	const Common::UString animation =
		Common::String::format("CUT%03dW", static_cast<int>(cameraAnimation) - 1200);

	try {
		if (!_animatedCamera || !_animatedCameraName.equalsIgnoreCase(cameraModel)) {
			releaseAnimatedCamera();

			_animatedCamera.reset(loadModelObject(cameraModel));
			_animatedCameraName = cameraModel;
		}

		if (!_animatedCamera || !_animatedCamera->hasAnimation(animation) ||
		    !_animatedCamera->getNode("camerahook")) {
			releaseAnimatedCamera();
			return false;
		}

		startShot(kShotAnimated);

		_animatedCamera->setPosition(0.0f, 0.0f, 0.0f);
		_animatedCamera->show();
		_animatedCamera->playAnimation(animation, true);
	} catch (...) {
		warning("CameraController::setAnimatedShot(): Failed to load camera \"%s\"", cameraModel.c_str());
		releaseAnimatedCamera();
		return false;
	}

	return true;
}

void CameraController::releaseAnimatedCamera() {
	if (_animatedCamera)
		_animatedCamera->hide();

	_animatedCamera.reset();
	_animatedCameraName.clear();
}

void CameraController::setShotFieldOfView(float fov) {
	_shotFieldOfView = fov;

	if (fov > 1.0f && fov < 179.0f) {
		GfxMan.setPerspective(fov, 0.1f, 10000.0f);
	} else if (Area *area = _module->getCurrentArea()) {
		const Area::CameraStyle &style = area->getCameraStyle();
		GfxMan.setPerspective(style.viewAngle, 0.1f, 10000.0f);
	}
}

void CameraController::setCinematicCamera(uint32_t cameraID, float UNUSED(cameraAngle), const Common::UString &UNUSED(cameraModel)) {
	setStaticShot(cameraID);
}

void CameraController::setCameraMode(CameraMode mode, Object *target) {
	// Keep a running transition (CameraTransitionToTarget) blending.
	if (!_cinematic || _shotType != kShotFocus)
		startShot(kShotFocus);
	_cinematicFocus = target;

	// Distance (m) and elevation (degrees) of the shot around the focus.
	switch (mode) {
	case kCameraModeCloseup:
		_focusDistance = 1.2f;
		_focusElevation = 5.0f;
		break;
	case kCameraModeMedium:
		_focusDistance = 3.5f;
		_focusElevation = 10.0f;
		break;
	case kCameraModeWide:
		_focusDistance = 7.0f;
		_focusElevation = 15.0f;
		break;
	case kCameraModeIsometric:
		_focusDistance = 15.0f;
		_focusElevation = 45.0f;
		break;
	case kCameraModeReveal:
		_focusDistance = 25.0f;
		_focusElevation = 20.0f;
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
	// Ease from the current view to the focus shot instead of cutting.
	startShot(kShotFocus, duration > 0.0f ? duration : 0.001f);
}

glm::vec3 CameraController::getEyes(const Object &object) const {
	float x, y, z;
	object.getPosition(x, y, z);

	const Creature *creature = ObjectContainer::toCreature(const_cast<Object *>(&object));
	return glm::vec3(x, y, z + (creature ? creature->getCameraHeight() : 1.0f));
}

/** World transform of a model node, composed from the node chain each call.
 *  (ModelNode's cached absolute transform is only refreshed when the
 *  model's bound is rebuilt, not on every animation frame.) */
static glm::mat4 composeNodeTransform(const Graphics::Aurora::ModelNode &node) {
	std::vector<const Graphics::Aurora::ModelNode *> chain;
	for (const Graphics::Aurora::ModelNode *n = &node; n; n = n->getParent())
		chain.push_back(n);

	glm::mat4 transform(1.0f);
	for (auto n = chain.rbegin(); n != chain.rend(); ++n) {
		float x, y, z, a;

		(*n)->getPosition(x, y, z);
		transform = glm::translate(transform, glm::vec3(x, y, z));

		(*n)->getOrientation(x, y, z, a);
		const glm::vec3 axis(x, y, z);
		if (glm::length(axis) > 0.0001f && a != 0.0f)
			transform = glm::rotate(transform, Common::deg2rad(a), glm::normalize(axis));

		(*n)->getRotation(x, y, z);
		transform = glm::rotate(transform, Common::deg2rad(x), glm::vec3(1.0f, 0.0f, 0.0f));
		transform = glm::rotate(transform, Common::deg2rad(y), glm::vec3(0.0f, 1.0f, 0.0f));
		transform = glm::rotate(transform, Common::deg2rad(z), glm::vec3(0.0f, 0.0f, 1.0f));
	}

	return transform;
}

bool CameraController::computeShot(CinematicShot &shot) const {
	switch (_shotType) {
		case kShotAnimated:
			if (_animatedCamera) {
				const Graphics::Aurora::ModelNode *hook = _animatedCamera->getNode("camerahook");
				if (hook) {
					const glm::mat4 transform = composeNodeTransform(*hook);
					shot.position  = glm::vec3(transform[3]);
					shot.direction = lookDirectionFromTransform(transform);
					return true;
				}
			}
			break;

		case kShotStatic:
			shot.position  = _staticPosition;
			shot.direction = _staticDirection;
			return true;

		case kShotDialog:
			if (_dialogSpeaker || _dialogListener) {
				const Object &speaker = _dialogSpeaker ? *_dialogSpeaker : *_dialogListener;

				float x, y, z, facing;
				speaker.getOrientation(x, y, z, facing);

				const glm::vec3 speakerEyes = getEyes(speaker);
				glm::vec3 listenerEyes;
				if (_dialogListener && _dialogListener != &speaker)
					listenerEyes = getEyes(*_dialogListener);

				shot = computeDialogShot(_dialogAngle, speakerEyes, facing,
				                         (_dialogListener && _dialogListener != &speaker) ? &listenerEyes : nullptr);
				return true;
			}
			break;

		case kShotFocus:
			break;
	}

	// Scripted cutscenes: along a path, or around the focus object.
	const Object *focus = _cameraTarget ? _cameraTarget : _cinematicFocus;

	if (_pathStart && _pathEnd) {
		float x1, y1, z1, x2, y2, z2;
		_pathStart->getPosition(x1, y1, z1);
		_pathEnd->getPosition(x2, y2, z2);

		float t = (_pathDuration > 0.0f) ? (_pathTime / _pathDuration) : 1.0f;
		t = CLIP(t, 0.0f, 1.0f);
		t = t * t * (3.0f - 2.0f * t);

		const glm::vec3 start(x1, y1, z1), end(x2, y2, z2);
		shot.position  = start + (end - start) * t;
		shot.direction = focus ? (getEyes(*focus) - shot.position) : (end - start);
		if (glm::length(shot.direction) < 0.001f)
			shot.direction = _viewDirection;
		return true;
	}

	if (focus) {
		shot = computeFocusShot(getEyes(*focus), _yaw, _focusDistance, _focusElevation);
		return true;
	}

	return false;
}

void CameraController::applyView(const glm::vec3 &position, const glm::vec3 &direction) {
	float pitch, yaw;
	directionToOrientation(direction, pitch, yaw);

	_viewPosition  = position;
	_viewDirection = direction;
	_haveView      = true;

	// Keep the orbit parameters in step, so restoreGameplayCamera() blends
	// from what is actually on screen.
	_pitch = pitch;
	_yaw   = yaw;
	_distance = 2.0f;
	_target = position + glm::vec3(-std::sin(yaw), std::cos(yaw), 0.0f) * _distance - glm::vec3(0.0f, 0.0f, _height);

	glm::vec3 shaken = position;
	if (_shakeTime > 0.0f) {
		shaken.x += RNG.getNext(-1.0f, 1.0f) * _shakeIntensity;
		shaken.y += RNG.getNext(-1.0f, 1.0f) * _shakeIntensity;
		shaken.z += RNG.getNext(-1.0f, 1.0f) * _shakeIntensity;
	}

	CameraMan.setPosition(shaken.x, shaken.y, shaken.z);
	CameraMan.setOrientation(pitch, 0.0f, Common::rad2deg(yaw));
	CameraMan.update();
}

void CameraController::updateCinematic(float frameTime) {
	if (_shakeTime > 0.0f)
		_shakeTime -= frameTime;

	if (_holding) {
		_holdTime -= frameTime;
		if (_holdTime <= 0.0f)
			_holding = false;

		CameraMan.update();
		return;
	}

	if (_pathStart && _pathEnd)
		_pathTime += frameTime;

	CinematicShot shot;
	if (!computeShot(shot)) {
		CameraMan.update();
		return;
	}

	if (_shotType == kShotFocus && _pathStart && _pathEnd && _pathTime >= _pathDuration) {
		// Stay where the path ended.
		_staticPosition  = shot.position;
		_staticDirection = shot.direction;
		_shotType = kShotStatic;
		_pathStart = nullptr;
		_pathEnd = nullptr;
	}

	glm::vec3 position  = shot.position;
	glm::vec3 direction = shot.direction;

	if (_blendDuration > 0.0f) {
		_blendTime += frameTime;

		float t = CLIP(_blendTime / _blendDuration, 0.0f, 1.0f);
		const float smooth = t * t * (3.0f - 2.0f * t);

		const glm::vec3 fromDirection = glm::normalize(_blendFromDirection);
		const glm::vec3 toDirection   = glm::normalize(direction);

		position  = _blendFromPosition + (position - _blendFromPosition) * smooth;
		direction = fromDirection + (toDirection - fromDirection) * smooth;
		if (glm::length(direction) < 0.001f)
			direction = toDirection;

		if (t >= 1.0f)
			_blendDuration = 0.0f;
	}

	applyView(position, direction);
}

void CameraController::cameraMoveAlongPath(Object *start, Object *end, float duration) {
	// A script may pass tags that fail to resolve to objects; bail rather than
	// dereference a null waypoint while interpolating the path.
	if (!start || !end) {
		warning("CameraController::cameraMoveAlongPath(): missing %s endpoint; ignoring",
		        !start ? "start" : "end");
		_pathStart = nullptr;
		_pathEnd = nullptr;
		return;
	}

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
	if (blendTime <= 0.0f || !_cinematic) {
		resetToOrbit();
		return;
	}

	Creature *partyLeader = _module->getPartyLeader();
	if (!partyLeader) {
		resetToOrbit();
		return;
	}

	_restoreStartYaw = _yaw;
	_restoreStartPitch = _pitch;
	_restoreStartDistance = _distance > 0.0f ? _distance : _actualDistance;
	_restoreStartTarget = _target;

	float x, y, z, angle;
	partyLeader->getPosition(x, y, z);
	_restoreDestTarget = glm::vec3(x, y, z + partyLeader->getCameraHeight());

	if (Area *area = _module->getCurrentArea()) {
		const Area::CameraStyle &style = area->getCameraStyle();
		_restoreDestDistance = style.distance;
		_restoreDestPitch = style.pitch;
	} else {
		_restoreDestDistance = _restoreStartDistance;
		_restoreDestPitch = _restoreStartPitch;
	}

	partyLeader->getOrientation(x, y, z, angle);
	_restoreDestYaw = Common::deg2rad(angle - 15.0f);

	_restoreTime = 0.0f;
	_restoreDuration = blendTime;
	_restoringGameplay = true;
	_dirty = true;
}

void CameraController::enterCinematicMode() {
	if (_cinematic && _shotType == kShotFocus)
		return;

	startShot(kShotFocus);
}

void CameraController::forgetObject(const Object *object) {
	if (_dialogSpeaker == object)
		_dialogSpeaker = nullptr;
	if (_dialogListener == object)
		_dialogListener = nullptr;
	if (_cinematicFocus == object)
		_cinematicFocus = nullptr;
	if (_cameraTarget == object)
		_cameraTarget = nullptr;
	if (_pathStart == object || _pathEnd == object) {
		_pathStart = nullptr;
		_pathEnd = nullptr;
	}
}

void CameraController::resetToOrbit(bool keepView) {
	const bool wasCinematic = _cinematic;

	_restoringGameplay = false;
	_cinematic = false;
	_shotType = kShotFocus;
	_cinematicFocus = nullptr;
	_cameraTarget = nullptr;
	_dialogSpeaker = nullptr;
	_dialogListener = nullptr;
	_pathStart = nullptr;
	_pathEnd = nullptr;
	_blendDuration = 0.0f;
	_holding = false;
	_haveView = false;

	releaseAnimatedCamera();

	if (wasCinematic) {
		// Back to the gameplay camera's distance, pitch and field of view.
		if (!keepView)
			_yaw = _orbitYaw;

		if (Area *area = _module->getCurrentArea()) {
			const Area::CameraStyle &style = area->getCameraStyle();
			_distance = style.distance;
			_actualDistance = style.distance;
			_pitch = style.pitch;
			_height = style.height;
		}

		setShotFieldOfView(0.0f);
	}

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
