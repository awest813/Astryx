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
 */

#include <algorithm>
#include <cmath>

#include "external/glm/geometric.hpp"
#include "external/glm/gtc/matrix_transform.hpp"
#include "external/glm/gtc/quaternion.hpp"

#include "src/common/maths.h"
#include "src/common/strutil.h"
#include "src/common/string.h"

#include "src/engines/kotorbase/cinematicshot.h"

namespace Engines {

namespace KotORBase {

/** Horizontal unit vector, or a fallback when v has no horizontal extent. */
static glm::vec3 horizontal(const glm::vec3 &v, const glm::vec3 &fallback) {
	const glm::vec3 h(v.x, v.y, 0.0f);
	const float length = glm::length(h);
	return (length > 0.001f) ? h / length : fallback;
}

glm::vec3 facingToForward(float facing) {
	const float a = Common::deg2rad(facing);
	return glm::vec3(-std::sin(a), std::cos(a), 0.0f);
}

void directionToOrientation(const glm::vec3 &direction, float &pitch, float &yaw) {
	const float h = std::sqrt(direction.x * direction.x + direction.y * direction.y);

	yaw   = (h > 0.0001f) ? std::atan2(-direction.x, direction.y) : 0.0f;
	pitch = 90.0f + Common::rad2deg(std::atan2(direction.z, h));
}

glm::vec3 lookDirectionFromTransform(const glm::mat4 &transform) {
	return glm::vec3(transform * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
}

CinematicShot computeDialogShot(int cameraAngle,
                                const glm::vec3 &speakerEyes, float speakerFacing,
                                const glm::vec3 *listenerEyes) {

	const glm::vec3 up(0.0f, 0.0f, 1.0f);
	const glm::vec3 speakerForward = facingToForward(speakerFacing);

	// Without a distinct listener every shot is a close-up of the speaker.
	const bool haveListener = listenerEyes && glm::length(*listenerEyes - speakerEyes) > 0.05f;

	CinematicShot shot;

	if (!haveListener || cameraAngle == kDialogCameraFace) {
		// In front of the speaker, slightly off-axis, looking at the face.
		const glm::vec3 side = glm::cross(speakerForward, up);
		shot.position  = speakerEyes + speakerForward * 1.1f + side * 0.2f;
		shot.direction = speakerEyes - shot.position;
		return shot;
	}

	const glm::vec3 listener = *listenerEyes;

	// Unit vector from the listener to the speaker, and its right-hand side.
	const glm::vec3 toSpeaker = horizontal(speakerEyes - listener, -speakerForward);
	const glm::vec3 side      = glm::cross(toSpeaker, up);

	if (cameraAngle == kDialogCameraWide) {
		// Side-on two-shot of both participants.
		const glm::vec3 middle = (speakerEyes + listener) * 0.5f;
		const float spread = glm::length(glm::vec3(speakerEyes.x - listener.x, speakerEyes.y - listener.y, 0.0f));
		const float distance = std::max(3.0f, spread * 1.5f);

		shot.position  = middle + side * distance + up * 0.3f;
		shot.direction = middle - shot.position;
		return shot;
	}

	// Auto, shoulder, random and anything unknown: over the listener's
	// shoulder, framing the speaker.
	shot.position  = listener - toSpeaker * 0.9f + side * 0.45f + up * 0.15f;
	shot.direction = speakerEyes - shot.position;
	return shot;
}

CinematicShot computeStaticShot(const float position[3], const float orientation[4],
                                float height, float pitch) {

	// GFF orientations are stored as (w, x, y, z).
	const glm::quat rotation(orientation[0], orientation[1], orientation[2], orientation[3]);

	glm::mat4 transform(1.0f);
	transform  = glm::translate(transform, glm::vec3(position[0], position[1], position[2] + height));
	transform *= glm::mat4_cast(rotation);
	transform  = glm::rotate(transform, Common::deg2rad(pitch), glm::vec3(1.0f, 0.0f, 0.0f));

	CinematicShot shot;
	shot.position  = glm::vec3(transform[3]);
	shot.direction = lookDirectionFromTransform(transform);
	return shot;
}

std::vector<Common::UString> cutsceneAnimationNames(uint32_t number) {
	std::vector<Common::UString> names;
	if (number < 1200)
		return names;

	names.push_back(Common::String::format("CUT%03dW", static_cast<int>(number) - 1199));
	names.push_back(Common::String::format("CUT%03dW", static_cast<int>(number) - 1200));
	return names;
}

CinematicShot computeFocusShot(const glm::vec3 &focus, float yaw, float distance, float elevation) {
	// The camera looks along (-sin yaw, cos yaw); stand behind that, raised.
	const glm::vec3 forward(-std::sin(yaw), std::cos(yaw), 0.0f);

	CinematicShot shot;
	shot.position  = focus - forward * distance +
	                 glm::vec3(0.0f, 0.0f, distance * std::tan(Common::deg2rad(elevation)));
	shot.direction = focus - shot.position;
	return shot;
}

} // End of namespace KotORBase

} // End of namespace Engines
