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
 *  Unit tests for the KotOR cutscene/conversation camera shot math.
 */

#include <cmath>

#include "gtest/gtest.h"

#include "external/glm/geometric.hpp"
#include "external/glm/gtc/matrix_transform.hpp"

#include "src/common/maths.h"
#include "src/common/memreadstream.h"
#include "src/common/memwritestream.h"

#include "src/aurora/dlgfile.h"
#include "src/aurora/gff3writer.h"

#include "src/engines/kotorbase/cinematicshot.h"

using namespace Engines::KotORBase;

namespace {

/** Where a camera with this orientation looks, built the way the renderer
 *  builds it (GraphicsManager: Rz(yaw) * Ry(roll) * Rx(pitch), looking down -Z). */
glm::vec3 rendererLookDirection(float pitchDegrees, float yawRadians) {
	glm::mat4 m(1.0f);
	m = glm::rotate(m, yawRadians, glm::vec3(0.0f, 0.0f, 1.0f));
	m = glm::rotate(m, Common::deg2rad(pitchDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	return glm::normalize(glm::vec3(m * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));
}

void expectLooksAlong(const glm::vec3 &direction) {
	float pitch, yaw;
	directionToOrientation(direction, pitch, yaw);

	const glm::vec3 actual = rendererLookDirection(pitch, yaw);
	const glm::vec3 wanted = glm::normalize(direction);

	EXPECT_NEAR(actual.x, wanted.x, 1e-4f);
	EXPECT_NEAR(actual.y, wanted.y, 1e-4f);
	EXPECT_NEAR(actual.z, wanted.z, 1e-4f);
}

void expectLooksAt(const CinematicShot &shot, const glm::vec3 &point) {
	const glm::vec3 toPoint = glm::normalize(point - shot.position);
	EXPECT_GT(glm::dot(glm::normalize(shot.direction), toPoint), 0.999f);
}

} // End of anonymous namespace

GTEST_TEST(CinematicCamera, orientationMatchesRenderer) {
	expectLooksAlong(glm::vec3( 0.0f,  1.0f,  0.0f));
	expectLooksAlong(glm::vec3( 1.0f,  0.0f,  0.0f));
	expectLooksAlong(glm::vec3(-1.0f,  0.0f,  0.0f));
	expectLooksAlong(glm::vec3( 0.0f, -1.0f,  0.0f));
	expectLooksAlong(glm::vec3( 0.3f,  0.8f, -0.2f));
	expectLooksAlong(glm::vec3(-0.5f, -0.5f,  0.4f));
}

GTEST_TEST(CinematicCamera, horizontalIsPitchNinety) {
	float pitch, yaw;
	directionToOrientation(glm::vec3(0.0f, 1.0f, 0.0f), pitch, yaw);
	EXPECT_NEAR(pitch, 90.0f, 1e-4f);
	EXPECT_NEAR(yaw, 0.0f, 1e-4f);

	directionToOrientation(glm::vec3(0.0f, 0.0f, -1.0f), pitch, yaw);
	EXPECT_NEAR(pitch, 0.0f, 1e-4f);
}

GTEST_TEST(CinematicCamera, facingMatchesMakeLookAt) {
	// Object::makeLookAt() faces atan2(dy, dx) - 90 degrees.
	const glm::vec3 toTarget = glm::normalize(glm::vec3(3.0f, -2.0f, 0.0f));
	const float facing = Common::rad2deg(std::atan2(toTarget.y, toTarget.x)) - 90.0f;

	const glm::vec3 forward = facingToForward(facing);
	EXPECT_NEAR(forward.x, toTarget.x, 1e-4f);
	EXPECT_NEAR(forward.y, toTarget.y, 1e-4f);
}

GTEST_TEST(CinematicCamera, shoulderShotFramesSpeakerOverListener) {
	const glm::vec3 speaker (0.0f, 2.0f, 1.7f);
	const glm::vec3 listener(0.0f, 0.0f, 1.7f);

	const CinematicShot shot = computeDialogShot(kDialogCameraShoulder, speaker, 180.0f, &listener);

	// Behind the listener, looking at the speaker.
	EXPECT_LT(shot.position.y, listener.y);
	EXPECT_LT(glm::length(shot.position - listener), glm::length(shot.position - speaker));
	expectLooksAt(shot, speaker);

	// Auto behaves like the shoulder shot.
	const CinematicShot autoShot = computeDialogShot(kDialogCameraAuto, speaker, 180.0f, &listener);
	EXPECT_NEAR(glm::length(autoShot.position - shot.position), 0.0f, 1e-4f);
}

GTEST_TEST(CinematicCamera, faceShotIsInFrontOfSpeaker) {
	const glm::vec3 speaker (0.0f, 2.0f, 1.7f);
	const glm::vec3 listener(0.0f, 0.0f, 1.7f);

	// Facing 180 degrees: looking along -Y, towards the listener.
	const CinematicShot shot = computeDialogShot(kDialogCameraFace, speaker, 180.0f, &listener);

	EXPECT_GT(glm::dot(shot.position - speaker, facingToForward(180.0f)), 0.5f);
	expectLooksAt(shot, speaker);
}

GTEST_TEST(CinematicCamera, wideShotSeesBoth) {
	const glm::vec3 speaker (0.0f, 2.0f, 1.7f);
	const glm::vec3 listener(0.0f, 0.0f, 1.7f);

	const CinematicShot shot = computeDialogShot(kDialogCameraWide, speaker, 180.0f, &listener);

	EXPECT_NEAR(glm::length(shot.position - speaker), glm::length(shot.position - listener), 1e-3f);
	EXPECT_GE(glm::length(shot.position - (speaker + listener) * 0.5f), 3.0f);
	expectLooksAt(shot, (speaker + listener) * 0.5f);
}

GTEST_TEST(CinematicCamera, noListenerFallsBackToFace) {
	const glm::vec3 speaker(5.0f, 5.0f, 1.7f);

	const CinematicShot shot = computeDialogShot(kDialogCameraShoulder, speaker, 0.0f, nullptr);
	EXPECT_GT(glm::dot(shot.position - speaker, facingToForward(0.0f)), 0.5f);
	expectLooksAt(shot, speaker);
}

GTEST_TEST(CinematicCamera, staticCameraOrientation) {
	const float position[3] = { 1.0f, 2.0f, 0.0f };

	// Identity orientation, pitch 90: horizontal along +Y, raised by Height.
	const float identity[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
	CinematicShot shot = computeStaticShot(position, identity, 1.5f, 90.0f);
	EXPECT_NEAR(shot.position.z, 1.5f, 1e-4f);
	const glm::vec3 forward = glm::normalize(shot.direction);
	EXPECT_NEAR(forward.y, 1.0f, 1e-4f);

	// 90 degrees about Z (w, x, y, z): +Y turns into -X.
	const float half = std::sqrt(0.5f);
	const float turned[4] = { half, 0.0f, 0.0f, half };
	shot = computeStaticShot(position, turned, 0.0f, 90.0f);
	const glm::vec3 left = glm::normalize(shot.direction);
	EXPECT_NEAR(left.x, -1.0f, 1e-4f);
	EXPECT_NEAR(left.y,  0.0f, 1e-4f);
}

GTEST_TEST(CinematicCamera, focusShotLooksAtFocusFromAbove) {
	const glm::vec3 focus(0.0f, 0.0f, 1.7f);

	const CinematicShot shot = computeFocusShot(focus, 0.0f, 4.0f, 10.0f);

	// Yaw 0 looks along +Y, so the camera stands on the -Y side, raised.
	EXPECT_LT(shot.position.y, 0.0f);
	EXPECT_GT(shot.position.z, focus.z);
	expectLooksAt(shot, focus);
}

GTEST_TEST(CinematicCamera, dlgCutsceneFieldsAreParsed) {
	// A one-entry conversation the way KotOR stores it: CameraAngle and
	// friends are integers, not floats.
	Aurora::GFF3Writer writer(MKTAG('D', 'L', 'G', ' '));
	Aurora::GFF3WriterStructPtr top = writer.getTopLevel();
	top->addResRef("CameraModel", "end_cam");
	top->addUint32("ConversationType", 0);

	Aurora::GFF3WriterStructPtr entry = top->addList("EntryList")->addStruct();
	entry->addExoString("Speaker", "trask");
	entry->addExoString("Listener", "PLAYER");
	entry->addUint32("CameraAngle", 6);
	entry->addSint32("CameraID", 3);
	entry->addUint16("CameraAnimation", 1205);
	entry->addFloat("CamFieldOfView", 45.0f);
	entry->addUint32("Delay", 4);
	entry->addUint32("FadeType", 3);
	entry->addFloat("FadeLength", 1.5f);
	entry->addFloat("FadeDelay", 0.25f);
	entry->addVector("FadeColor", glm::vec3(1.0f, 0.5f, 0.0f));
	Aurora::GFF3WriterStructPtr anim = entry->addList("AnimList")->addStruct();
	anim->addExoString("Participant", "OWNER");
	anim->addUint16("Animation", 10010);
	entry->addList("RepliesList");

	top->addList("ReplyList");
	top->addList("StartingList")->addStruct()->addUint32("Index", 0);

	Common::MemoryWriteStreamDynamic out(true);
	writer.write(out);

	Aurora::DLGFile dlg(new Common::MemoryReadStream(out.getData(), out.size()));
	EXPECT_TRUE(dlg.getCameraModel().equalsIgnoreCase("end_cam"));

	dlg.startConversation();
	const Aurora::DLGFile::Line *line = dlg.getCurrentEntry();
	ASSERT_NE(line, nullptr);

	EXPECT_EQ(static_cast<int>(line->cameraAngle), kDialogCameraStatic);
	EXPECT_EQ(line->cameraID, 3U);
	EXPECT_EQ(line->cameraAnimation, 1205U);
	EXPECT_FLOAT_EQ(line->camFieldOfView, 45.0f);
	EXPECT_EQ(line->delay, 4U);
	EXPECT_EQ(line->fadeType, 3U);
	EXPECT_FLOAT_EQ(line->fadeLength, 1.5f);
	EXPECT_FLOAT_EQ(line->fadeDelay, 0.25f);
	EXPECT_FLOAT_EQ(line->fadeColor[0], 1.0f);
	EXPECT_FLOAT_EQ(line->fadeColor[1], 0.5f);
	EXPECT_FLOAT_EQ(line->fadeColor[2], 0.0f);
	EXPECT_TRUE(line->listener.equalsIgnoreCase("PLAYER"));
	ASSERT_EQ(line->animations.size(), 1U);
	EXPECT_TRUE(line->animations[0].participant.equalsIgnoreCase("OWNER"));
	EXPECT_EQ(line->animations[0].animation, 10010U);
}
