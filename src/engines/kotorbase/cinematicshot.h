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
 *  Camera shot math for KotOR cutscenes and conversations.
 *
 *  Conventions (matching Graphics::CameraManager): the camera's orientation is
 *  (pitch, roll, yaw) in degrees, applied as Rz(yaw) * Ry(roll) * Rx(pitch) to
 *  a camera looking down -Z. Pitch 0 looks straight down, 90 is horizontal.
 *  Yaw 0 looks along +Y; yaw y looks along (-sin y, cos y). An object facing
 *  angle a (degrees, as in Object::setOrientation) faces (-sin a, cos a).
 */

#ifndef ENGINES_KOTORBASE_CINEMATICSHOT_H
#define ENGINES_KOTORBASE_CINEMATICSHOT_H

#include "external/glm/vec3.hpp"
#include "external/glm/mat4x4.hpp"

namespace Engines {

namespace KotORBase {

/** DLG "CameraAngle" values. */
enum DialogCameraAngle {
	kDialogCameraAuto     = 0,
	kDialogCameraFace     = 1,
	kDialogCameraShoulder = 2,
	kDialogCameraWide     = 3,
	kDialogCameraAnimated = 4,
	kDialogCameraRandom   = 5,
	kDialogCameraStatic   = 6
};

/** A camera placement: where it is and where it looks. */
struct CinematicShot {
	glm::vec3 position { 0.0f };
	glm::vec3 direction { 0.0f, 1.0f, 0.0f }; ///< Need not be normalized.
};

/** Forward vector of an object facing angle (degrees). */
glm::vec3 facingToForward(float facing);

/** Camera orientation (pitch in degrees, yaw in radians) looking along dir. */
void directionToOrientation(const glm::vec3 &direction, float &pitch, float &yaw);

/** Look direction of a camera whose world transform is given (looks down local -Z). */
glm::vec3 lookDirectionFromTransform(const glm::mat4 &transform);

/** Conversation shot between a speaker and a listener (eye positions).
 *
 *  speakerFacing is the speaker's facing angle (degrees), used for face
 *  close-ups and when there's no distinct listener.
 */
CinematicShot computeDialogShot(int cameraAngle,
                                const glm::vec3 &speakerEyes, float speakerFacing,
                                const glm::vec3 *listenerEyes);

/** A GIT static camera: Position, Orientation (w, x, y, z), Height and Pitch (degrees). */
CinematicShot computeStaticShot(const float position[3], const float orientation[4],
                                float height, float pitch);

/** A shot of a focus point from a yaw (radians), distance and elevation (degrees). */
CinematicShot computeFocusShot(const glm::vec3 &focus, float yaw, float distance, float elevation);

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_CINEMATICSHOT_H
