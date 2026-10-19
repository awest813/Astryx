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
 *  Unit tests for the swoop race / turret minigame simulation.
 */

#include <functional>
#include <map>
#include <memory>
#include <vector>

#include "gtest/gtest.h"

#include "src/common/memreadstream.h"
#include "src/common/memwritestream.h"

#include "src/aurora/gff3file.h"
#include "src/aurora/gff3writer.h"

#include "src/engines/kotorbase/minigamedesc.h"
#include "src/engines/kotorbase/object.h"
#include "src/engines/kotorbase/swoopminigame.h"

using namespace Engines::KotORBase;

namespace {

/** Records the scripts the simulation runs; a script can be given a behaviour. */
class RecordingHost : public MinigameHost {
public:
	struct Call {
		Common::UString script;
		Common::UString self;
	};

	std::vector<Call> calls;
	std::vector<Common::UString> animations;
	std::vector<Common::UString> deaths;
	std::map<Common::UString, std::function<void(MinigameObject &)>> behaviours;

	void runScript(const Common::UString &script, MinigameObject &self) override {
		calls.push_back({ script, self.getTag() });

		auto behaviour = behaviours.find(script);
		if (behaviour != behaviours.end())
			behaviour->second(self);
	}

	void playAnimation(MinigameObject &object, const Common::UString &animation, bool UNUSED(loop)) override {
		animations.push_back(object.getTag() + ":" + animation);
	}

	void onDeath(MinigameObject &object) override {
		deaths.push_back(object.getTag());
	}

	size_t count(const Common::UString &script) const {
		size_t n = 0;
		for (const Call &call : calls)
			if (call.script == script)
				n++;
		return n;
	}
};

MinigameFollowerDesc makeFollower(const Common::UString &model, float radius) {
	MinigameFollowerDesc desc;

	MinigameModelDesc modelDesc;
	modelDesc.model = model;
	desc.models.push_back(modelDesc);

	desc.sphereRadius = radius;
	return desc;
}

MinigameGunBankDesc makeGunBank(int damage, float rateOfFire, float speed, float lifespan) {
	MinigameGunBankDesc bank;
	bank.bullet.damage     = damage;
	bank.bullet.rateOfFire = rateOfFire;
	bank.bullet.speed      = speed;
	bank.bullet.lifespan   = lifespan;
	return bank;
}

/** A swoop race: player on a track, a finish trigger and a racer, one obstacle. */
MinigameDesc makeSwoopRace() {
	MinigameDesc desc;
	desc.type = kMinigameTypeSwoop;
	desc.lateralAccel = 4.0f;

	desc.player = makeFollower("swoop_player", 1.0f);
	desc.player.minSpeed  = 10.0f;
	desc.player.maxSpeed  = 30.0f;
	desc.player.accelSecs = 5.0f;
	desc.player.tunnelNeg = glm::vec3(-3.0f, 0.0f, 0.0f);
	desc.player.tunnelPos = glm::vec3( 3.0f, 0.0f, 0.0f);
	desc.player.bumpDamage = 5;
	desc.player.hitPoints = 20;
	desc.player.scripts[kMinigameScriptOnHitFollower] = "p_hitfollow";

	MinigameFollowerDesc finish = makeFollower("finish_line", 2.0f);
	finish.trigger = true;
	desc.enemies.push_back(finish);

	MinigameObstacleDesc obstacle;
	obstacle.name = "rock01";
	obstacle.scripts[kMinigameScriptOnHitFollower] = "o_hitfollow";
	desc.obstacles.push_back(obstacle);

	return desc;
}

/** A turret: the player with a gun, one fighter with a gun. */
MinigameDesc makeTurret() {
	MinigameDesc desc;
	desc.type = kMinigameTypeTurret;

	desc.player = makeFollower("turret", 1.0f);
	desc.player.hitPoints = 50;
	desc.player.maxHitPoints = 50;
	desc.player.tunnelNeg = glm::vec3(-20.0f, 0.0f, -90.0f);
	desc.player.tunnelPos = glm::vec3( 45.0f, 0.0f,  90.0f);
	desc.player.gunBanks.push_back(makeGunBank(10, 0.5f, 100.0f, 2.0f));

	MinigameFollowerDesc fighter = makeFollower("fighter", 2.0f);
	fighter.hitPoints = 15;
	fighter.maxHitPoints = 15;
	fighter.gunBanks.push_back(makeGunBank(4, 1.0f, 50.0f, 3.0f));
	fighter.scripts[kMinigameScriptOnDeath] = "e_death";
	desc.enemies.push_back(fighter);

	return desc;
}

} // End of anonymous namespace

// .--- ARE MiniGame struct

GTEST_TEST(MinigameDesc, parseAREStruct) {
	Aurora::GFF3Writer writer(MKTAG('A', 'R', 'E', ' '));
	Aurora::GFF3WriterStructPtr mg = writer.getTopLevel()->addStruct("MiniGame");

	mg->addUint32("Type", 1);
	mg->addFloat("CameraViewAngle", 60.0f);
	mg->addFloat("Near_Clip", 0.5f);
	mg->addFloat("Far_Clip", 500.0f);
	mg->addFloat("LateralAccel", 12.0f);
	mg->addFloat("MovementPerSec", 3.0f);
	mg->addUint32("UseInertia", 1);

	Aurora::GFF3WriterStructPtr mouse = mg->addStruct("Mouse");
	mouse->addUint32("AxisX", 1);
	mouse->addByte("FlipAxisY", 1);

	Aurora::GFF3WriterStructPtr player = mg->addStruct("Player");
	player->addFloat("Accel_Secs", 6.0f);
	player->addSint32("Bump_Damage", 3);
	player->addResRef("Camera", "mgcam");
	player->addUint32("Hit_Points", 40);
	player->addUint32("Max_HPs", 50);
	player->addFloat("Invince_Period", 1.5f);
	player->addFloat("Maximum_Speed", 80.0f);
	player->addFloat("Minimum_Speed", 20.0f);
	player->addSint32("Num_Loops", 2);
	player->addFloat("Sphere_Radius", 1.25f);
	player->addResRef("Track", "mgtrack01");
	player->addFloat("TunnelXNeg", -4.0f);
	player->addFloat("TunneXPos", 4.5f);      // retail misspelling
	player->addFloat("TunneZNeg", -1.0f);     // retail misspelling
	player->addFloat("TunnelZPos", 2.0f);
	player->addVector("TunnelInfinite", glm::vec3(0.0f, 1.0f, 0.0f));

	Aurora::GFF3WriterListPtr models = player->addList("Models");
	Aurora::GFF3WriterStructPtr model = models->addStruct();
	model->addResRef("Model", "mgplayer");
	model->addByte("RotatingModel", 1);
	model = models->addStruct();
	model->addResRef("Model", "mgbase");
	model->addByte("RotatingModel", 0);

	Aurora::GFF3WriterStructPtr bank = player->addList("Gun_Banks")->addStruct();
	bank->addUint32("BankID", 1);
	bank->addResRef("Gun_Model", "mggun");
	bank->addResRef("Fire_Sound", "mgfire");
	bank->addFloat("Sensing_Radius", 30.0f);
	Aurora::GFF3WriterStructPtr bullet = bank->addStruct("Bullet");
	bullet->addResRef("Bullet_Model", "mgbullet");
	bullet->addUint32("Damage", 7);
	bullet->addFloat("Lifespan", 2.5f);
	bullet->addFloat("Rate_Of_Fire", 0.25f);
	bullet->addFloat("Speed", 120.0f);
	bullet->addUint32("Target_Type", 2);

	Aurora::GFF3WriterStructPtr scripts = player->addStruct("Scripts");
	scripts->addResRef("OnHeartbeat", "mg_hb");
	scripts->addResRef("OnHitObstacle", "mg_obst");
	scripts->addResRef("OnTrackLoop", "mg_loop");

	Aurora::GFF3WriterStructPtr enemy = mg->addList("Enemies")->addStruct();
	enemy->addResRef("Track", "mgtrack02");
	enemy->addByte("Trigger", 1);
	enemy->addFloat("Sphere_Radius", 3.0f);
	enemy->addList("Models")->addStruct()->addResRef("Model", "mgenemy");

	Aurora::GFF3WriterStructPtr obstacle = mg->addList("Obstacles")->addStruct();
	obstacle->addResRef("Name", "mgobst01");
	obstacle->addStruct("Scripts")->addResRef("OnHitFollower", "mg_ohit");

	Common::MemoryWriteStreamDynamic out(true);
	writer.write(out);

	Aurora::GFF3File gff(new Common::MemoryReadStream(out.getData(), out.size()), MKTAG('A', 'R', 'E', ' '));
	const MinigameDesc desc = MinigameDesc::parse(gff.getTopLevel().getStruct("MiniGame"));

	EXPECT_EQ(desc.type, kMinigameTypeSwoop);
	EXPECT_FLOAT_EQ(desc.cameraViewAngle, 60.0f);
	EXPECT_FLOAT_EQ(desc.nearClip, 0.5f);
	EXPECT_FLOAT_EQ(desc.farClip, 500.0f);
	EXPECT_FLOAT_EQ(desc.lateralAccel, 12.0f);
	EXPECT_FLOAT_EQ(desc.movementPerSec, 3.0f);
	EXPECT_TRUE(desc.useInertia);
	EXPECT_EQ(desc.mouseAxisX, 1);
	EXPECT_TRUE(desc.mouseFlipY);

	const MinigameFollowerDesc &p = desc.player;
	EXPECT_FLOAT_EQ(p.accelSecs, 6.0f);
	EXPECT_EQ(p.bumpDamage, 3);
	EXPECT_STREQ(p.camera.c_str(), "mgcam");
	EXPECT_EQ(p.hitPoints, 40);
	EXPECT_EQ(p.maxHitPoints, 50);
	EXPECT_FLOAT_EQ(p.invincePeriod, 1.5f);
	EXPECT_FLOAT_EQ(p.maxSpeed, 80.0f);
	EXPECT_FLOAT_EQ(p.minSpeed, 20.0f);
	EXPECT_EQ(p.numLoops, 2);
	EXPECT_FLOAT_EQ(p.sphereRadius, 1.25f);
	EXPECT_STREQ(p.track.c_str(), "mgtrack01");
	EXPECT_FLOAT_EQ(p.tunnelNeg.x, -4.0f);
	EXPECT_FLOAT_EQ(p.tunnelPos.x, 4.5f);
	EXPECT_FLOAT_EQ(p.tunnelNeg.z, -1.0f);
	EXPECT_FLOAT_EQ(p.tunnelPos.z, 2.0f);
	EXPECT_FLOAT_EQ(p.tunnelInfinite.y, 1.0f);

	ASSERT_EQ(p.models.size(), 2U);
	EXPECT_STREQ(p.models[0].model.c_str(), "mgplayer");
	EXPECT_TRUE(p.models[0].rotating);
	EXPECT_FALSE(p.models[1].rotating);

	ASSERT_EQ(p.gunBanks.size(), 1U);
	EXPECT_EQ(p.gunBanks[0].bankID, 1U);
	EXPECT_STREQ(p.gunBanks[0].gunModel.c_str(), "mggun");
	EXPECT_STREQ(p.gunBanks[0].fireSound.c_str(), "mgfire");
	EXPECT_FLOAT_EQ(p.gunBanks[0].sensingRadius, 30.0f);
	EXPECT_STREQ(p.gunBanks[0].bullet.model.c_str(), "mgbullet");
	EXPECT_EQ(p.gunBanks[0].bullet.damage, 7);
	EXPECT_FLOAT_EQ(p.gunBanks[0].bullet.lifespan, 2.5f);
	EXPECT_FLOAT_EQ(p.gunBanks[0].bullet.rateOfFire, 0.25f);
	EXPECT_FLOAT_EQ(p.gunBanks[0].bullet.speed, 120.0f);
	EXPECT_EQ(p.gunBanks[0].bullet.targetType, 2U);

	EXPECT_STREQ(p.scripts[kMinigameScriptOnHeartbeat].c_str(), "mg_hb");
	EXPECT_STREQ(p.scripts[kMinigameScriptOnHitObstacle].c_str(), "mg_obst");
	EXPECT_STREQ(p.scripts[kMinigameScriptOnTrackLoop].c_str(), "mg_loop");
	EXPECT_TRUE(p.scripts[kMinigameScriptOnCreate].empty());

	ASSERT_EQ(desc.enemies.size(), 1U);
	EXPECT_TRUE(desc.enemies[0].trigger);
	EXPECT_STREQ(desc.enemies[0].track.c_str(), "mgtrack02");
	EXPECT_FLOAT_EQ(desc.enemies[0].sphereRadius, 3.0f);
	ASSERT_EQ(desc.enemies[0].models.size(), 1U);

	ASSERT_EQ(desc.obstacles.size(), 1U);
	EXPECT_STREQ(desc.obstacles[0].name.c_str(), "mgobst01");
	EXPECT_STREQ(desc.obstacles[0].scripts[kMinigameScriptOnHitFollower].c_str(), "mg_ohit");
}

GTEST_TEST(MinigameDesc, scriptLabels) {
	EXPECT_STREQ(getMinigameScriptLabel(kMinigameScriptOnAccelerate), "OnAccelerate");
	EXPECT_STREQ(getMinigameScriptLabel(kMinigameScriptOnTrackLoop), "OnTrackLoop");
	EXPECT_STREQ(getMinigameScriptLabel(kMinigameScriptMAX), "");
}

// '---

// .--- Geometry

GTEST_TEST(SwoopMinigameGeometry, segmentHitsSphere) {
	EXPECT_TRUE (segmentHitsSphere(glm::vec3(0, -5, 0), glm::vec3(0, 5, 0), glm::vec3(0.5f, 0, 0), 1.0f));
	EXPECT_FALSE(segmentHitsSphere(glm::vec3(0, -5, 0), glm::vec3(0, 5, 0), glm::vec3(2.0f, 0, 0), 1.0f));
	// A fast mover passes clean through the sphere between two frames.
	EXPECT_TRUE (segmentHitsSphere(glm::vec3(0, -50, 0), glm::vec3(0, 50, 0), glm::vec3(0, 0, 0), 0.5f));
	// A point (no movement).
	EXPECT_TRUE (segmentHitsSphere(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), glm::vec3(0, 0.5f, 0), 1.0f));
}

GTEST_TEST(SwoopMinigameGeometry, segmentHitsBox) {
	const glm::vec3 min(-1, 10, -1), max(1, 12, 1);
	EXPECT_TRUE (segmentHitsBox(glm::vec3(0, 0, 0), glm::vec3(0, 20, 0), min, max, 0.0f));
	EXPECT_FALSE(segmentHitsBox(glm::vec3(3, 0, 0), glm::vec3(3, 20, 0), min, max, 1.0f));
	EXPECT_TRUE (segmentHitsBox(glm::vec3(3, 0, 0), glm::vec3(3, 20, 0), min, max, 2.5f));
	EXPECT_FALSE(segmentHitsBox(glm::vec3(0, 0, 0), glm::vec3(0, 8, 0), min, max, 1.0f));
}

GTEST_TEST(SwoopMinigameGeometry, rayHitsSphere) {
	EXPECT_TRUE (rayHitsSphere(glm::vec3(0, 0, 0), glm::vec3(0, 1, 0), glm::vec3(0, 10, 0), 1.0f, 0.0f));
	EXPECT_FALSE(rayHitsSphere(glm::vec3(0, 0, 0), glm::vec3(0, -1, 0), glm::vec3(0, 10, 0), 1.0f, 0.0f));
	EXPECT_FALSE(rayHitsSphere(glm::vec3(0, 0, 0), glm::vec3(0, 1, 0), glm::vec3(0, 10, 0), 1.0f, 5.0f));
	EXPECT_FALSE(rayHitsSphere(glm::vec3(0, 0, 0), glm::vec3(1, 0, 0), glm::vec3(0, 10, 0), 1.0f, 0.0f));
}

GTEST_TEST(SwoopMinigameGeometry, clampToTunnel) {
	EXPECT_FLOAT_EQ(clampToTunnel( 5.0f, -2.0f, 2.0f, false),  2.0f);
	EXPECT_FLOAT_EQ(clampToTunnel(-5.0f, -2.0f, 2.0f, false), -2.0f);
	EXPECT_FLOAT_EQ(clampToTunnel( 5.0f, -2.0f, 2.0f, true ),  5.0f);
	// An empty tunnel doesn't lock the axis.
	EXPECT_FLOAT_EQ(clampToTunnel( 5.0f,  0.0f, 0.0f, false),  5.0f);
}

// '---

// .--- Swoop race

GTEST_TEST(SwoopMinigame, startSetsUpObjects) {
	SwoopMinigame &swoop = SwoopMinigame::get();
	swoop.start(makeSwoopRace(), { "rock01", "pad01" });

	EXPECT_TRUE(swoop.isActive());
	EXPECT_EQ(swoop.getType(), kMinigameTypeSwoop);
	EXPECT_EQ(swoop.getEnemyCount(), 1U);
	EXPECT_EQ(swoop.getObstacleCount(), 2U);

	ASSERT_NE(swoop.getPlayer(), nullptr);
	EXPECT_STREQ(swoop.getPlayer()->getTag().c_str(), "swoop_player");
	EXPECT_EQ(swoop.getObjectByName("FINISH_LINE"), swoop.getEnemy(0));
	EXPECT_EQ(swoop.getObjectByName("pad01"), swoop.getObstacle(1));
	EXPECT_EQ(swoop.getObjectByName("nothing"), nullptr);

	EXPECT_TRUE(swoop.isTrigger(swoop.getEnemy(0)));
	EXPECT_FALSE(swoop.isTrigger(swoop.getPlayer()));

	// Only the ARE obstacle with a matching name has scripts.
	EXPECT_STREQ(swoop.getObstacleState(0)->desc.scripts[kMinigameScriptOnHitFollower].c_str(), "o_hitfollow");
	EXPECT_TRUE(swoop.getObstacleState(1)->desc.scripts[kMinigameScriptOnHitFollower].empty());

	// Objects of other worlds aren't minigame objects.
	Object stranger(kObjectTypePlaceable);
	EXPECT_EQ(swoop.toMinigameObject(&stranger), nullptr);
	EXPECT_EQ(swoop.getFollower(&stranger), nullptr);

	swoop.reset();
	EXPECT_FALSE(swoop.isActive());
	EXPECT_EQ(swoop.getEnemyCount(), 0U);
}

GTEST_TEST(SwoopMinigame, speedStartsAtMinimumAndAccelerates) {
	SwoopMinigame &swoop = SwoopMinigame::get();
	swoop.start(makeSwoopRace(), {});

	swoop.step(1.0f);
	EXPECT_FLOAT_EQ(swoop.getSpeed(), 15.0f);       // min 10 + 5/s
	EXPECT_FLOAT_EQ(swoop.getPlayerOffset().y, 15.0f);

	swoop.step(1.0f);
	EXPECT_FLOAT_EQ(swoop.getSpeed(), 20.0f);
	EXPECT_FLOAT_EQ(swoop.getPlayerOffset().y, 35.0f);

	for (int i = 0; i < 10; ++i)
		swoop.step(1.0f);
	EXPECT_FLOAT_EQ(swoop.getSpeed(), 30.0f);       // capped at the maximum

	// Scripts can slow the player down; it never drops below the minimum.
	swoop.setSpeed(2.0f);
	swoop.step(0.1f);
	EXPECT_FLOAT_EQ(swoop.getSpeed(), 10.5f);
}

GTEST_TEST(SwoopMinigame, steeringStaysInTheTunnel) {
	SwoopMinigame &swoop = SwoopMinigame::get();
	swoop.start(makeSwoopRace(), {});

	MinigameInput input;
	input.right = true;
	swoop.setInput(input);
	swoop.step(0.5f);
	EXPECT_FLOAT_EQ(swoop.getPlayerOffset().x, 2.0f);   // 4/s lateral

	swoop.step(1.0f);
	EXPECT_FLOAT_EQ(swoop.getPlayerOffset().x, 3.0f);   // tunnel edge

	input.right = false;
	input.left = true;
	swoop.setInput(input);
	swoop.step(1.0f);
	EXPECT_FLOAT_EQ(swoop.getPlayerOffset().x, -1.0f);

	// No input: no sideways drift.
	swoop.setInput(MinigameInput());
	swoop.step(1.0f);
	EXPECT_FLOAT_EQ(swoop.getPlayerOffset().x, -1.0f);

	// Scripts move the walls; the player is pushed back inside.
	swoop.setTunnelNeg(glm::vec3(-0.5f, 0.0f, 0.0f));
	EXPECT_FLOAT_EQ(swoop.getPlayerOffset().x, -0.5f);

	swoop.setTunnelInfinite(glm::vec3(1.0f, 0.0f, 0.0f));
	swoop.setPlayerOffset(glm::vec3(-10.0f, swoop.getPlayerOffset().y, 0.0f));
	EXPECT_FLOAT_EQ(swoop.getPlayerOffset().x, -10.0f);
}

GTEST_TEST(SwoopMinigame, obstacleHitFiresOncePerContact) {
	SwoopMinigame &swoop = SwoopMinigame::get();
	RecordingHost host;
	swoop.start(makeSwoopRace(), { "rock01" });
	swoop.setHost(&host);

	MinigameObject &player = *swoop.getPlayer();
	swoop.setObstacleBounds(0, glm::vec3(-1.0f, 20.0f, 5.0f), glm::vec3(1.0f, 22.0f, 6.0f));

	// Approach: no contact yet.
	swoop.setFollowerPose(player, glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(0, 1, 0));
	EXPECT_TRUE(swoop.detectHits().empty());

	// Pass through it in one frame (and at a different height: a race is flat).
	swoop.setFollowerPose(player, glm::vec3(0.0f, 30.0f, 0.0f), glm::vec3(0, 1, 0));
	std::vector<MinigameHit> hits = swoop.detectHits();
	ASSERT_EQ(hits.size(), 1U);
	EXPECT_EQ(hits[0].type, MinigameHit::kFollowerHitObstacle);
	EXPECT_EQ(hits[0].subject, &player);
	EXPECT_EQ(hits[0].other, swoop.getObstacle(0));

	swoop.dispatchHits(hits);

	// The player has no OnHitObstacle: the default bump damage applies.
	EXPECT_EQ(swoop.getHitPoints(&player), 15);
	EXPECT_EQ(swoop.getLastHPChange(), -5);
	EXPECT_EQ(swoop.getLastObstacleHit(), swoop.getObstacle(0));
	// The obstacle's OnHitFollower ran, with the player as the follower hit.
	ASSERT_EQ(host.count("o_hitfollow"), 1U);
	EXPECT_STREQ(host.calls.back().self.c_str(), "rock01");
	EXPECT_EQ(swoop.getLastFollowerHit(), &player);

	// Still touching the far edge next frame: no second hit.
	swoop.setFollowerPose(player, glm::vec3(0.0f, 22.5f, 0.0f), glm::vec3(0, 1, 0));
	EXPECT_TRUE(swoop.detectHits().empty());

	// Leave, come back: a new hit.
	swoop.setFollowerPose(player, glm::vec3(0.0f, 40.0f, 0.0f), glm::vec3(0, 1, 0));
	EXPECT_TRUE(swoop.detectHits().empty());
	swoop.setFollowerPose(player, glm::vec3(0.0f, 50.0f, 0.0f), glm::vec3(0, 1, 0));
	EXPECT_TRUE(swoop.detectHits().empty());
	swoop.setFollowerPose(player, glm::vec3(0.0f, 21.0f, 0.0f), glm::vec3(0, 1, 0));
	EXPECT_EQ(swoop.detectHits().size(), 1U);

	swoop.setHost(nullptr);
}

GTEST_TEST(SwoopMinigame, playerOnHitObstacleScriptReplacesDefault) {
	SwoopMinigame &swoop = SwoopMinigame::get();
	RecordingHost host;

	MinigameDesc desc = makeSwoopRace();
	desc.player.scripts[kMinigameScriptOnHitObstacle] = "p_hitobst";
	swoop.start(desc, { "rock01" });
	swoop.setHost(&host);

	MinigameObject &player = *swoop.getPlayer();
	swoop.setObstacleBounds(0, glm::vec3(-1.0f, 5.0f, 0.0f), glm::vec3(1.0f, 6.0f, 0.0f));
	swoop.setFollowerPose(player, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0, 1, 0));
	swoop.setFollowerPose(player, glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(0, 1, 0));
	swoop.dispatchHits(swoop.detectHits());

	ASSERT_GE(host.calls.size(), 2U);
	EXPECT_STREQ(host.calls[0].script.c_str(), "p_hitobst");
	EXPECT_STREQ(host.calls[0].self.c_str(), "swoop_player");
	EXPECT_STREQ(host.calls[1].script.c_str(), "o_hitfollow");
	EXPECT_EQ(swoop.getHitPoints(&player), 20);   // no default bump damage

	swoop.setHost(nullptr);
}

GTEST_TEST(SwoopMinigame, crossingTheFinishTrigger) {
	SwoopMinigame &swoop = SwoopMinigame::get();
	RecordingHost host;
	swoop.start(makeSwoopRace(), {});
	swoop.setHost(&host);

	MinigameObject &player = *swoop.getPlayer();
	MinigameObject &finish = *swoop.getEnemy(0);

	swoop.setFollowerPose(finish, glm::vec3(0.0f, 100.0f, 4.0f), glm::vec3(0, 1, 0));
	swoop.setFollowerPose(player, glm::vec3(1.0f, 90.0f, 0.0f), glm::vec3(0, 1, 0));
	EXPECT_TRUE(swoop.detectHits().empty());

	swoop.setFollowerPose(player, glm::vec3(1.0f, 110.0f, 0.0f), glm::vec3(0, 1, 0));
	std::vector<MinigameHit> hits = swoop.detectHits();
	ASSERT_EQ(hits.size(), 1U);
	EXPECT_EQ(hits[0].type, MinigameHit::kFollowerHitFollower);

	MinigameObject *followerSeen = nullptr;
	host.behaviours["p_hitfollow"] = [&](MinigameObject &) {
		followerSeen = swoop.getLastFollowerHit();
	};
	swoop.dispatchHits(hits);

	EXPECT_EQ(host.count("p_hitfollow"), 1U);
	EXPECT_EQ(followerSeen, &finish);
	EXPECT_TRUE(swoop.isTrigger(followerSeen));

	swoop.setHost(nullptr);
}

GTEST_TEST(SwoopMinigame, createAndHeartbeatEvents) {
	SwoopMinigame &swoop = SwoopMinigame::get();
	RecordingHost host;

	MinigameDesc desc = makeSwoopRace();
	desc.player.scripts[kMinigameScriptOnCreate] = "p_create";
	desc.player.scripts[kMinigameScriptOnHeartbeat] = "p_hb";
	desc.player.scripts[kMinigameScriptOnAccelerate] = "p_accel";
	desc.enemies[0].scripts[kMinigameScriptOnCreate] = "e_create";
	desc.obstacles[0].scripts[kMinigameScriptOnCreate] = "o_create";
	swoop.start(desc, { "rock01" });
	swoop.setHost(&host);

	swoop.runCreateEvents();
	ASSERT_EQ(host.calls.size(), 3U);
	EXPECT_STREQ(host.calls[0].script.c_str(), "e_create");
	EXPECT_STREQ(host.calls[1].script.c_str(), "o_create");
	EXPECT_STREQ(host.calls[2].script.c_str(), "p_create");

	swoop.runHeartbeats();
	EXPECT_EQ(host.count("p_hb"), 1U);

	swoop.onAccelerate();
	swoop.onBrake();   // no script: nothing runs
	EXPECT_EQ(host.count("p_accel"), 1U);
	EXPECT_EQ(host.calls.size(), 5U);

	swoop.setHost(nullptr);
}

// '---

// .--- Turret

GTEST_TEST(SwoopMinigame, turretRotatesWithinTheTunnel) {
	SwoopMinigame &swoop = SwoopMinigame::get();
	swoop.start(makeTurret(), {});

	const float rate = SwoopMinigame::getTurretTurnRate();

	MinigameInput input;
	input.up = true;
	input.left = true;
	swoop.setInput(input);
	swoop.step(0.5f);
	EXPECT_FLOAT_EQ(swoop.getPlayerOffset().x, rate * 0.5f);
	EXPECT_FLOAT_EQ(swoop.getPlayerOffset().z, rate * 0.5f);

	swoop.step(2.0f);
	EXPECT_FLOAT_EQ(swoop.getPlayerOffset().x, 45.0f);
	EXPECT_FLOAT_EQ(swoop.getPlayerOffset().z, 90.0f);

	swoop.setReverseVerticalInput(true);
	swoop.step(0.1f);
	EXPECT_FLOAT_EQ(swoop.getPlayerOffset().x, 45.0f - rate * 0.1f);
	swoop.setReverseVerticalInput(false);

	// The turret doesn't drive forward.
	EXPECT_FLOAT_EQ(swoop.getPlayerOffset().y, 0.0f);
}

GTEST_TEST(SwoopMinigame, turretShootsDownAFighter) {
	SwoopMinigame &swoop = SwoopMinigame::get();
	RecordingHost host;
	swoop.start(makeTurret(), {});
	swoop.setHost(&host);

	MinigameObject &player = *swoop.getPlayer();
	MinigameObject &fighter = *swoop.getEnemy(0);

	swoop.setFollowerPose(player, glm::vec3(0.0f), glm::vec3(0, 1, 0));
	swoop.setFollowerPose(fighter, glm::vec3(0.0f, 50.0f, 0.0f), glm::vec3(1, 0, 0));

	ASSERT_TRUE(swoop.fire(player, 0, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0, 1, 0)));
	EXPECT_EQ(swoop.getLastBulletFiredDamage(), 10);
	EXPECT_FALSE(swoop.fire(player, 0, glm::vec3(0.0f), glm::vec3(0, 1, 0)));   // still reloading
	ASSERT_EQ(swoop.getBullets().size(), 1U);

	// 100/s: within half a second the bullet reaches the fighter.
	std::vector<MinigameHit> hits;
	for (int i = 0; (i < 10) && hits.empty(); ++i) {
		swoop.step(0.1f);
		hits = swoop.detectHits();
	}
	ASSERT_EQ(hits.size(), 1U);
	EXPECT_EQ(hits[0].type, MinigameHit::kBulletHitFollower);
	EXPECT_EQ(hits[0].subject, &fighter);

	swoop.dispatchHits(hits);
	EXPECT_EQ(swoop.getHitPoints(&fighter), 5);
	EXPECT_EQ(swoop.getLastBulletHitDamage(), 10);
	EXPECT_EQ(swoop.getLastBulletHitShooter(), &player);
	EXPECT_EQ(host.animations.back(), Common::UString("fighter:damage"));

	// The reload is over; the second shot kills it.
	swoop.step(0.5f);
	ASSERT_TRUE(swoop.fire(player, 0, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0, 1, 0)));
	hits.clear();
	for (int i = 0; (i < 10) && hits.empty(); ++i) {
		swoop.step(0.1f);
		hits = swoop.detectHits();
	}
	swoop.dispatchHits(hits);

	EXPECT_EQ(swoop.getHitPoints(&fighter), -5);
	EXPECT_EQ(host.count("e_death"), 1U);   // its OnDeath script, no default death
	EXPECT_TRUE(host.deaths.empty());
	EXPECT_FALSE(swoop.getEnemyState(0)->alive);

	// The dead don't die twice, nor shoot.
	swoop.step(0.1f);
	EXPECT_EQ(host.count("e_death"), 1U);
	EXPECT_TRUE(swoop.getEnemyFireRequests().empty());

	swoop.setHost(nullptr);
}

GTEST_TEST(SwoopMinigame, fightersShootWhenAimingAtThePlayer) {
	SwoopMinigame &swoop = SwoopMinigame::get();
	RecordingHost host;

	MinigameDesc desc = makeTurret();
	desc.player.scripts[kMinigameScriptOnHitBullet] = "p_hitbullet";
	swoop.start(desc, {});
	swoop.setHost(&host);

	MinigameObject &player = *swoop.getPlayer();
	MinigameObject &fighter = *swoop.getEnemy(0);

	swoop.setFollowerPose(player, glm::vec3(0.0f), glm::vec3(0, 1, 0));

	// Flying past, not aiming at the player.
	swoop.setFollowerPose(fighter, glm::vec3(0.0f, 40.0f, 0.0f), glm::vec3(1, 0, 0));
	EXPECT_TRUE(swoop.getEnemyFireRequests().empty());
	EXPECT_FALSE(swoop.getGunBank(&fighter, 0)->targeting);

	// Too far for its bullets (50/s * 3s).
	swoop.setFollowerPose(fighter, glm::vec3(0.0f, 200.0f, 0.0f), glm::vec3(0, -1, 0));
	EXPECT_TRUE(swoop.getEnemyFireRequests().empty());

	// Turned towards the player, in range.
	swoop.setFollowerPose(fighter, glm::vec3(0.0f, 40.0f, 0.0f), glm::vec3(0, -1, 0));
	std::vector<MinigameFireRequest> requests = swoop.getEnemyFireRequests();
	ASSERT_EQ(requests.size(), 1U);
	EXPECT_EQ(requests[0].shooter, &fighter);
	EXPECT_TRUE(swoop.getGunBank(&fighter, 0)->targeting);

	ASSERT_TRUE(swoop.fire(fighter, 0, glm::vec3(0.0f, 38.0f, 0.0f), glm::vec3(0, -1, 0)));

	// The player's OnHitBullet script takes over; it asks for the default.
	host.behaviours["p_hitbullet"] = [&](MinigameObject &self) {
		swoop.defaultBulletHit(self);
	};

	std::vector<MinigameHit> hits;
	for (int i = 0; (i < 20) && hits.empty(); ++i) {
		swoop.step(0.1f);
		hits = swoop.detectHits();
	}
	ASSERT_EQ(hits.size(), 1U);
	EXPECT_EQ(hits[0].subject, &player);
	swoop.dispatchHits(hits);

	EXPECT_EQ(host.count("p_hitbullet"), 1U);
	EXPECT_EQ(swoop.getHitPoints(&player), 46);
	EXPECT_EQ(swoop.getLastBulletHitShooter(), &fighter);

	// Invulnerable: no damage.
	swoop.setPlayerInvincibility(2.0f);
	swoop.damage(player, 10);
	EXPECT_EQ(swoop.getHitPoints(&player), 46);
	EXPECT_TRUE(swoop.isInvulnerable(&player));
	swoop.step(2.5f);
	EXPECT_FALSE(swoop.isInvulnerable(&player));

	swoop.setHost(nullptr);
}

GTEST_TEST(SwoopMinigame, defaultDeathAndHitPointAdjustments) {
	SwoopMinigame &swoop = SwoopMinigame::get();
	RecordingHost host;

	MinigameDesc desc = makeTurret();
	desc.enemies[0].scripts[kMinigameScriptOnDeath].clear();
	desc.enemies[0].invincePeriod = 1.0f;
	swoop.start(desc, {});
	swoop.setHost(&host);

	MinigameObject &fighter = *swoop.getEnemy(0);

	EXPECT_EQ(swoop.adjustHitPoints(&fighter, -5, false), 10);
	EXPECT_EQ(swoop.getLastHPChange(), -5);
	EXPECT_EQ(swoop.adjustHitPoints(&fighter, 100, false), 15);   // capped at Max_HPs
	EXPECT_EQ(swoop.adjustHitPoints(&fighter, 3, true), 3);

	swoop.startInvulnerability(&fighter);
	EXPECT_TRUE(swoop.isInvulnerable(&fighter));
	swoop.step(1.5f);
	EXPECT_FALSE(swoop.isInvulnerable(&fighter));

	// A script setting the hit points to 0 kills it, with the default death.
	swoop.adjustHitPoints(&fighter, 0, true);
	swoop.step(0.1f);
	EXPECT_FALSE(swoop.getEnemyState(0)->alive);
	ASSERT_EQ(host.deaths.size(), 1U);
	EXPECT_STREQ(host.deaths[0].c_str(), "fighter");
	EXPECT_EQ(host.animations.back(), Common::UString("fighter:die"));

	swoop.setHost(nullptr);
}

GTEST_TEST(SwoopMinigame, objectsWithoutHitPointsNeverDie) {
	SwoopMinigame &swoop = SwoopMinigame::get();
	RecordingHost host;
	swoop.start(makeSwoopRace(), {});
	swoop.setHost(&host);

	// The finish trigger has no hit points at all.
	swoop.step(1.0f);
	EXPECT_TRUE(swoop.getEnemyState(0)->alive);
	EXPECT_TRUE(host.deaths.empty());

	swoop.setHost(nullptr);
}

GTEST_TEST(SwoopMinigame, gunBankAndSoundSettings) {
	SwoopMinigame &swoop = SwoopMinigame::get();
	swoop.start(makeTurret(), {});

	MinigameObject &player = *swoop.getPlayer();
	EXPECT_EQ(swoop.getGunBankCount(&player), 1);
	ASSERT_NE(swoop.getGunBank(&player, 0), nullptr);
	EXPECT_EQ(swoop.getGunBank(&player, 1), nullptr);
	EXPECT_EQ(swoop.getGunBank(&player, -1), nullptr);

	swoop.getGunBank(&player, 0)->desc.bullet.damage = 25;
	EXPECT_EQ(swoop.getGunBank(&player, 0)->desc.bullet.damage, 25);

	swoop.getSound(&player, 1).volume = 64;
	EXPECT_EQ(swoop.getSound(&player, 1).volume, 64);
	EXPECT_EQ(swoop.getSound(&player, 0).volume, 127);

	swoop.setCameraClip(1.0f, 900.0f);
	EXPECT_FLOAT_EQ(swoop.getCameraNearClip(), 1.0f);
	EXPECT_FLOAT_EQ(swoop.getCameraFarClip(), 900.0f);
}

// '---
