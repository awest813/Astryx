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

#ifndef ENGINES_KOTORBASE_CAMERACONTROLLER_H
#define ENGINES_KOTORBASE_CAMERACONTROLLER_H

#include <cstdint>
#include <memory>

#include "external/glm/vec3.hpp"
#include "external/glm/mat4x4.hpp"

#include "src/common/ustring.h"
#include "src/events/types.h"
#include "src/engines/kotorbase/types.h"

namespace Graphics {
	namespace Aurora {
		class Model;
		class ModelNode;
	}
}

namespace Engines {

namespace KotORBase {

class Module;
class Object;
struct CinematicShot;

/** A model node's transform within its model, composed from the node chain
 *  (the node's cached absolute transform isn't refreshed every animation frame). */
glm::mat4 composeNodeTransform(const Graphics::Aurora::ModelNode &node);

class CameraController {
public:
	CameraController(Module *module);

	// Flying camera

	bool isFlyCamera() const;

	void toggleFlyCamera();


	float getYaw() const;

	void setYaw(float value);
	void setPitch(float value);
	void setDistance(float value);

	void setCameraMode(CameraMode mode, Object *target = nullptr);

	void updateTarget();
	void updateCameraStyle();
	bool handleEvent(const Events::Event &e);
	void processRotation(float frameTime);
	void processMovement(float frameTime);
	void stopMovement();

	/** Apply current yaw/distance to CameraMan immediately (frameTime 0). Used when
	 *  scripts or dialogue set facing so the view matches KotOR without waiting a frame. */
	void syncOrbitingCamera();

	// Cinematic camera

	/** Conversation shot (DLG CameraAngle) of a speaker talking to a listener. */
	void setDialogShot(int cameraAngle, Object *speaker, Object *listener);
	/** Cut to a static camera from the area's GIT. Returns false if there is none. */
	bool setStaticShot(uint32_t cameraID);
	/** Fly an animated camera (DLG CameraModel + CameraAnimation). Returns false if unavailable. */
	bool setAnimatedShot(const Common::UString &cameraModel, uint32_t cameraAnimation);
	/** Field of view for the current cinematic shot (degrees); 0 restores the default. */
	void setShotFieldOfView(float fov);

	void setCinematicCamera(uint32_t cameraID, float cameraAngle, const Common::UString &cameraModel);
	void setCinematicFocus(Object *target);
	void setCameraTarget(Object *target);
	void cameraTransitionToTarget(float blendTime);
	void cameraMoveAlongPath(Object *start, Object *end, float duration);
	void cameraHold(float duration);
	void restoreGameplayCamera(float blendTime);
	/** Leave cinematic mode. Unless keepView, the gameplay camera from before the cinematic returns. */
	void resetToOrbit(bool keepView = false);
	/** Drop every reference to an object that is about to be destroyed. */
	void forgetObject(const Object *object);

	/** Enter scripted camera control without resetting yaw/distance. */
	void enterCinematicMode();

	void shake(float duration, float intensity);

private:
	Module *_module;
	bool _flycam { false };
	bool _cinematic { false };

	float _shakeTime { 0.0f };
	float _shakeIntensity { 0.0f };

	glm::vec3 _target;
	float _distance { 0.0f };
	float _yaw { 0.0f };
	float _pitch { 0.0f };
	float _height { 0.0f };

	// Cinematic settings

	enum ShotType {
		kShotFocus,    ///< Look at the focus object (scripted cutscenes), or along a path.
		kShotStatic,   ///< A static GIT camera.
		kShotDialog,   ///< Conversation framing of speaker and listener.
		kShotAnimated  ///< An animated camera model.
	};

	ShotType        _shotType { kShotFocus };
	glm::vec3       _staticPosition { 0.0f };
	glm::vec3       _staticDirection { 0.0f, 1.0f, 0.0f };
	int             _dialogAngle { 0 };
	Object         *_dialogSpeaker { nullptr };
	Object         *_dialogListener { nullptr };
	float           _focusDistance { 4.0f };
	float           _focusElevation { 10.0f };
	Object         *_cinematicFocus { nullptr };
	Object         *_cameraTarget { nullptr };

	std::unique_ptr<Graphics::Aurora::Model> _animatedCamera;
	Common::UString _animatedCameraName;

	float _shotFieldOfView { 0.0f };

	/** Blending from the previous view into the current shot. */
	bool      _haveView { false };
	glm::vec3 _viewPosition { 0.0f };
	glm::vec3 _viewDirection { 0.0f, 1.0f, 0.0f };
	glm::vec3 _blendFromPosition { 0.0f };
	glm::vec3 _blendFromDirection { 0.0f, 1.0f, 0.0f };
	float     _blendTime { 0.0f };
	float     _blendDuration { 0.0f };

	/** The gameplay camera to return to after a cinematic. */
	float _orbitYaw { 0.0f };

	// Pathing/Transitions
	Object *_pathStart { nullptr };
	Object *_pathEnd { nullptr };
	float  _pathTime { 0.0f };
	float  _pathDuration { 0.0f };


	bool   _holding { false };
	float  _holdTime { 0.0f };

	// Smooth return from cinematic to orbit camera
	bool      _restoringGameplay { false };
	float     _restoreTime { 0.0f };
	float     _restoreDuration { 0.0f };
	float     _restoreStartYaw { 0.0f };
	float     _restoreStartPitch { 0.0f };
	float     _restoreStartDistance { 0.0f };
	glm::vec3 _restoreStartTarget { 0.0f };
	float     _restoreDestYaw { 0.0f };
	float     _restoreDestPitch { 0.0f };
	float     _restoreDestDistance { 0.0f };
	glm::vec3 _restoreDestTarget { 0.0f };

	bool _clockwiseMovementWanted { false };
	bool _counterClockwiseMovementWanted { false };
	bool _dirty { false };

	// Obstacles

	bool _obstacleExists { false };
	float _obstacleDistance { 0.0f };
	float _actualDistance { 0.0f };


	inline glm::vec3 getCameraPosition(float distance) const;

	/** Switch into cinematic mode, remembering the gameplay camera. */
	void beginCinematic();
	/** Start a new shot, cutting (blendTime 0) or blending from the current view. */
	void startShot(ShotType type, float blendTime = 0.0f);
	/** Where the current shot wants the camera this frame. */
	bool computeShot(CinematicShot &shot) const;
	void updateCinematic(float frameTime);
	void applyView(const glm::vec3 &position, const glm::vec3 &direction);
	glm::vec3 getEyes(const Object &object) const;
	void releaseAnimatedCamera();

	inline bool shouldMoveClockwise() const;
	inline bool shouldMoveCounterClockwise() const;
};

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_CAMERACONTROLLER_H
