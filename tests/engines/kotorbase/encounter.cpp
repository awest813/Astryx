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
 *  Unit tests for the KotOR encounter spawn rules.
 */

#include <algorithm>
#include <cmath>
#include <memory>

#include "gtest/gtest.h"

#include "src/common/maths.h"
#include "src/common/memreadstream.h"
#include "src/common/memwritestream.h"

#include "src/aurora/gff3file.h"
#include "src/aurora/gff3writer.h"

#include "src/engines/kotorbase/types.h"
#include "src/engines/kotorbase/reputation.h"
#include "src/engines/kotorbase/encounterrules.h"

using namespace Engines::KotORBase;

static std::unique_ptr<Aurora::GFF3File> finish(Aurora::GFF3Writer &writer) {
	Common::MemoryWriteStreamDynamic out(true);
	writer.write(out);

	// The GFF takes over the buffer.
	out.setDisposable(false);
	return std::make_unique<Aurora::GFF3File>(
		new Common::MemoryReadStream(out.getData(), out.size(), true));
}

static void addCreature(Aurora::GFF3WriterList &list, const char *resRef, float cr, bool singleSpawn) {
	Aurora::GFF3WriterStructPtr creature = list.addStruct(0);
	creature->addSint32("Appearance", 5);
	creature->addResRef("ResRef", resRef);
	creature->addFloat("CR", cr);
	creature->addByte("SingleSpawn", singleSpawn ? 1 : 0);
}

/** A blueprint the way UTE files store it. */
static std::unique_ptr<Aurora::GFF3File> makeBlueprint() {
	Aurora::GFF3Writer writer(MKTAG('U', 'T', 'E', ' '));
	Aurora::GFF3WriterStructPtr top = writer.getTopLevel();

	top->addByte("Active", 1);
	top->addSint32("Difficulty", 2);
	top->addSint32("DifficultyIndex", 2);
	top->addUint32("Faction", 1);
	top->addSint32("MaxCreatures", 4);
	top->addSint32("RecCreatures", 3);
	top->addByte("PlayerOnly", 0);
	top->addByte("Reset", 1);
	top->addSint32("ResetTime", 60);
	top->addSint32("Respawns", 2);
	top->addSint32("SpawnOption", 1);
	top->addExoString("Tag", "tar_rakghoul_enc");
	top->addResRef("TemplateResRef", "tar_rakghoul_enc");
	top->addResRef("OnEntered", "k_enc_enter");

	Aurora::GFF3WriterListPtr creatures = top->addList("CreatureList");
	addCreature(*creatures, "tar_rakghoul001", 3.0f, false);
	addCreature(*creatures, "tar_rakghoul002", 5.0f, true);
	addCreature(*creatures, "", 1.0f, false);

	return finish(writer);
}

GTEST_TEST(KotOREncounter, uteFieldsAreParsed) {
	std::unique_ptr<Aurora::GFF3File> ute = makeBlueprint();

	EncounterRules rules;
	rules.load(ute->getTopLevel());

	EXPECT_TRUE(rules.active);
	EXPECT_EQ(rules.difficultyIndex, 2);
	EXPECT_EQ(rules.faction, 1U);
	EXPECT_EQ(rules.maxCreatures, 4);
	EXPECT_EQ(rules.recCreatures, 3);
	EXPECT_FALSE(rules.playerOnly);
	EXPECT_TRUE(rules.reset);
	EXPECT_EQ(rules.resetTime, 60);
	EXPECT_EQ(rules.respawns, 2);
	EXPECT_EQ(rules.spawnOption, EncounterRules::kSpawnSingleShot);

	// The entry without a blueprint is left out.
	ASSERT_EQ(rules.creatures.size(), 2U);
	EXPECT_TRUE(rules.creatures[0].resRef.equalsIgnoreCase("tar_rakghoul001"));
	EXPECT_FLOAT_EQ(rules.creatures[0].cr, 3.0f);
	EXPECT_FALSE(rules.creatures[0].singleSpawn);
	EXPECT_TRUE(rules.creatures[1].resRef.equalsIgnoreCase("tar_rakghoul002"));
	EXPECT_TRUE(rules.creatures[1].singleSpawn);
}

GTEST_TEST(KotOREncounter, gitInstanceOverridesBlueprint) {
	std::unique_ptr<Aurora::GFF3File> ute = makeBlueprint();

	Aurora::GFF3Writer writer(MKTAG('G', 'I', 'T', ' '));
	Aurora::GFF3WriterStructPtr instance = writer.getTopLevel();
	instance->addByte("Active", 0);
	instance->addUint32("Faction", 0xFFFFFFFF);
	instance->addSint32("SpawnOption", 0);
	addCreature(*instance->addList("CreatureList"), "dan_kinrath", 2.0f, false);
	std::unique_ptr<Aurora::GFF3File> git = finish(writer);

	EncounterRules rules;
	rules.load(ute->getTopLevel());
	rules.load(git->getTopLevel());

	EXPECT_FALSE(rules.active);
	EXPECT_EQ(rules.faction, 0U); // -1: no faction.
	EXPECT_EQ(rules.spawnOption, EncounterRules::kSpawnContinuous);

	// Fields the instance lacks keep the blueprint's values.
	EXPECT_EQ(rules.recCreatures, 3);
	EXPECT_EQ(rules.resetTime, 60);

	ASSERT_EQ(rules.creatures.size(), 1U);
	EXPECT_TRUE(rules.creatures[0].resRef.equalsIgnoreCase("dan_kinrath"));
}

GTEST_TEST(KotOREncounter, gitGeometryIsRelativeAndSpawnPointsAbsolute) {
	Aurora::GFF3Writer writer(MKTAG('G', 'I', 'T', ' '));
	Aurora::GFF3WriterStructPtr instance = writer.getTopLevel();
	instance->addFloat("XPosition", 10.0f);
	instance->addFloat("YPosition", 20.0f);
	instance->addFloat("ZPosition", 30.0f);

	Aurora::GFF3WriterListPtr geometry = instance->addList("Geometry");
	const float corners[3][3] = { { 0.0f, 0.0f, 0.0f }, { 1.0f, 2.0f, -10.0f }, { -3.0f, 4.0f, 0.5f } };
	for (const auto &corner : corners) {
		Aurora::GFF3WriterStructPtr point = geometry->addStruct(1);
		point->addFloat("X", corner[0]);
		point->addFloat("Y", corner[1]);
		point->addFloat("Z", corner[2]);
	}

	Aurora::GFF3WriterStructPtr spawn = instance->addList("SpawnPointList")->addStruct(0);
	spawn->addFloat("X", 12.5f);
	spawn->addFloat("Y", 21.0f);
	spawn->addFloat("Z", 30.25f);
	spawn->addFloat("Orientation", 1.5f);

	std::unique_ptr<Aurora::GFF3File> git = finish(writer);

	// Encounter Format, 2.3: the polygon is relative to the encounter's position.
	const std::vector<glm::vec3> polygon = readEncounterGeometry(git->getTopLevel());
	ASSERT_EQ(polygon.size(), 3U);
	EXPECT_FLOAT_EQ(polygon[0].x, 10.0f);
	EXPECT_FLOAT_EQ(polygon[0].y, 20.0f);
	EXPECT_FLOAT_EQ(polygon[0].z, 30.0f);
	EXPECT_FLOAT_EQ(polygon[1].x, 11.0f);
	EXPECT_FLOAT_EQ(polygon[1].y, 22.0f);
	EXPECT_FLOAT_EQ(polygon[1].z, 20.0f);
	EXPECT_FLOAT_EQ(polygon[2].x, 7.0f);
	EXPECT_FLOAT_EQ(polygon[2].y, 24.0f);

	// ... while the spawn points are area coordinates.
	const std::vector<EncounterSpawnPoint> points = readEncounterSpawnPoints(git->getTopLevel());
	ASSERT_EQ(points.size(), 1U);
	EXPECT_FLOAT_EQ(points[0].position.x, 12.5f);
	EXPECT_FLOAT_EQ(points[0].position.y, 21.0f);
	EXPECT_FLOAT_EQ(points[0].position.z, 30.25f);
	EXPECT_FLOAT_EQ(points[0].orientation, 1.5f);
}

GTEST_TEST(KotOREncounter, spawnCountFollowsRecAndMaxCreatures) {
	EncounterRules rules;

	rules.recCreatures = 3;
	rules.maxCreatures = 4;
	EXPECT_EQ(rules.getSpawnCount(0), 3);
	EXPECT_EQ(rules.getSpawnCount(1), 2);
	EXPECT_EQ(rules.getSpawnCount(5), 0);

	// Never more than MaxCreatures alive at a time.
	rules.recCreatures = 6;
	rules.maxCreatures = 2;
	EXPECT_EQ(rules.getSpawnCount(0), 2);
	EXPECT_EQ(rules.getSpawnCount(1), 1);

	// Broken values still spawn something.
	rules.recCreatures = 0;
	rules.maxCreatures = 0;
	EXPECT_EQ(rules.getSpawnCount(0), 1);

	rules.recCreatures = 2;
	rules.maxCreatures = 0;
	EXPECT_EQ(rules.getSpawnCount(0), 2);
}

GTEST_TEST(KotOREncounter, singleSpawnCreaturesAreNeverDoubled) {
	EncounterRules rules;
	rules.creatures.resize(2);
	rules.creatures[0].resRef      = "boss";
	rules.creatures[0].singleSpawn = true;
	rules.creatures[1].resRef      = "minion";

	// Always pick the first candidate.
	auto first = [](size_t) { return static_cast<size_t>(0); };

	std::vector<size_t> picks = rules.selectCreatures(3, {}, first);
	ASSERT_EQ(picks.size(), 3U);
	EXPECT_EQ(picks[0], 0U);
	EXPECT_EQ(picks[1], 1U);
	EXPECT_EQ(picks[2], 1U);

	// The boss is still alive from an earlier spawn.
	picks = rules.selectCreatures(2, { "BOSS" }, first);
	ASSERT_EQ(picks.size(), 2U);
	EXPECT_EQ(picks[0], 1U);
	EXPECT_EQ(picks[1], 1U);

	// Only single spawns: fewer creatures than asked for.
	rules.creatures[1].singleSpawn = true;
	picks = rules.selectCreatures(4, {}, first);
	EXPECT_EQ(picks.size(), 2U);

	// Out-of-range random values are clamped.
	picks = rules.selectCreatures(1, {}, [](size_t n) { return n + 5; });
	ASSERT_EQ(picks.size(), 1U);
	EXPECT_EQ(picks[0], 1U);

	EncounterRules empty;
	EXPECT_TRUE(empty.selectCreatures(3, {}, first).empty());
}

GTEST_TEST(KotOREncounter, onlyHostileCreaturesTrigger) {
	EXPECT_TRUE (EncounterRules::canTrigger(false, true,  true));
	EXPECT_TRUE (EncounterRules::canTrigger(false, false, true));
	EXPECT_FALSE(EncounterRules::canTrigger(false, true,  false));
	EXPECT_TRUE (EncounterRules::canTrigger(true,  true,  true));
	EXPECT_FALSE(EncounterRules::canTrigger(true,  false, true));
	EXPECT_FALSE(EncounterRules::canTrigger(true,  true,  false));

	// The hostility the encounter asks for comes from the faction reputations.
	ReputationMan.reset();
	ReputationMan.useBuiltInDefaults();
	EXPECT_TRUE (ReputationMan.isHostile(kFactionHostile1, kFactionPlayer));
	EXPECT_FALSE(ReputationMan.isHostile(kFactionFriendly1, kFactionPlayer));
	EXPECT_FALSE(ReputationMan.isHostile(kFactionNeutral, kFactionPlayer));
}

GTEST_TEST(KotOREncounter, singleShotFiresOnceAndReportsExhaustion) {
	EncounterRules rules;
	rules.spawnOption = EncounterRules::kSpawnSingleShot;

	ASSERT_TRUE(rules.fire());
	EXPECT_EQ(rules.numberSpawned, 1);
	EXPECT_TRUE(rules.exhausted);
	EXPECT_FALSE(rules.wantsReplacements(true, 0));

	// Entering again does nothing.
	EXPECT_FALSE(rules.fire());
	EXPECT_EQ(rules.numberSpawned, 1);

	// Exhausted once its creatures are gone, and only once.
	EXPECT_EQ(rules.update(1.0, true, 2), EncounterRules::kEventNone);
	EXPECT_EQ(rules.update(2.0, true, 0), EncounterRules::kEventExhausted);
	EXPECT_EQ(rules.update(3.0, false, 0), EncounterRules::kEventNone);

	// Without Reset, it stays exhausted.
	EXPECT_EQ(rules.update(100000.0, false, 0), EncounterRules::kEventNone);
	EXPECT_FALSE(rules.fire());
}

GTEST_TEST(KotOREncounter, inactiveEncountersDoNotFire) {
	EncounterRules rules;
	rules.active = false;

	EXPECT_FALSE(rules.fire());
	EXPECT_EQ(rules.numberSpawned, 0);

	rules.active = true;
	EXPECT_TRUE(rules.fire());
}

GTEST_TEST(KotOREncounter, resetRearmsAfterResetTime) {
	EncounterRules rules;
	rules.reset     = true;
	rules.resetTime = 30;
	rules.respawns  = 1;

	ASSERT_TRUE(rules.fire());

	// The countdown starts once all its creatures are gone.
	EXPECT_EQ(rules.update(10.0, false, 1), EncounterRules::kEventNone);
	EXPECT_EQ(rules.update(20.0, false, 0), EncounterRules::kEventExhausted);
	EXPECT_EQ(rules.update(49.0, false, 0), EncounterRules::kEventNone);
	EXPECT_FALSE(rules.fire());

	EXPECT_EQ(rules.update(50.0, false, 0), EncounterRules::kEventReset);
	EXPECT_FALSE(rules.exhausted);

	// Second (and, with one respawn, last) spawn.
	ASSERT_TRUE(rules.fire());
	EXPECT_EQ(rules.numberSpawned, 2);
	EXPECT_EQ(rules.update(60.0, false, 0), EncounterRules::kEventExhausted);
	EXPECT_EQ(rules.update(1000.0, false, 0), EncounterRules::kEventNone);
	EXPECT_FALSE(rules.fire());

	// Respawns -1: no limit.
	rules.respawns = -1;
	rules.exhaustReported = false;
	EXPECT_EQ(rules.update(1000.0, false, 0), EncounterRules::kEventExhausted);
	EXPECT_EQ(rules.update(1030.0, false, 0), EncounterRules::kEventReset);
	EXPECT_TRUE(rules.fire());
}

GTEST_TEST(KotOREncounter, continuousReplacesWhileHostilesStayInside) {
	EncounterRules rules;
	rules.spawnOption  = EncounterRules::kSpawnContinuous;
	rules.recCreatures = 2;
	rules.maxCreatures = 3;

	EXPECT_FALSE(rules.wantsReplacements(true, 0)); // Not fired yet.

	ASSERT_TRUE(rules.fire());
	EXPECT_FALSE(rules.wantsReplacements(true, 2));
	EXPECT_TRUE (rules.wantsReplacements(true, 1));
	EXPECT_FALSE(rules.wantsReplacements(false, 1));

	// Nothing exhausted while the party stays and replacements come.
	EXPECT_EQ(rules.update(1.0, true, 0), EncounterRules::kEventNone);
	EXPECT_TRUE(rules.wantsReplacements(true, 0));

	// Switched off by a script: no more replacements.
	rules.active = false;
	EXPECT_FALSE(rules.wantsReplacements(true, 0));
	rules.active = true;

	// The party left: the spawn is over.
	EXPECT_EQ(rules.update(2.0, false, 0), EncounterRules::kEventExhausted);
	EXPECT_FALSE(rules.wantsReplacements(true, 0));
	EXPECT_FALSE(rules.fire());
}

GTEST_TEST(KotOREncounter, stateSurvivesSaving) {
	EncounterRules rules;
	rules.reset     = true;
	rules.resetTime = 30;
	rules.respawns  = 3;
	rules.active    = false;
	rules.difficultyIndex = 4;
	rules.exhausted       = true;
	rules.exhaustReported = true;
	rules.numberSpawned   = 2;
	rules.resetStart      = 123.5;

	Aurora::GFF3Writer writer(MKTAG('G', 'F', 'F', ' '));
	rules.saveState(*writer.getTopLevel());
	std::unique_ptr<Aurora::GFF3File> state = finish(writer);

	EncounterRules loaded;
	loaded.reset     = true;
	loaded.resetTime = 30;
	loaded.loadState(state->getTopLevel());

	EXPECT_FALSE(loaded.active);
	EXPECT_EQ(loaded.respawns, 3);
	EXPECT_EQ(loaded.difficultyIndex, 4);
	EXPECT_TRUE(loaded.exhausted);
	EXPECT_TRUE(loaded.exhaustReported);
	EXPECT_EQ(loaded.numberSpawned, 2);
	EXPECT_DOUBLE_EQ(loaded.resetStart, 123.5);

	// The reset countdown carries on where it was.
	EXPECT_EQ(loaded.update(150.0, false, 0), EncounterRules::kEventNone);
	EXPECT_EQ(loaded.update(153.5, false, 0), EncounterRules::kEventReset);
}

GTEST_TEST(KotOREncounter, spawnPointsAreUsedInTurn) {
	std::vector<EncounterSpawnPoint> points(2);
	points[0].position    = glm::vec3(1.0f, 1.0f, 0.0f);
	points[0].orientation = static_cast<float>(M_PI) / 2.0f;
	points[1].position    = glm::vec3(5.0f, 5.0f, 1.0f);

	const std::vector<EncounterSpawnLocation> locations =
		computeEncounterSpawnLocations(points, {}, glm::vec3(0.0f), 3, 1);
	ASSERT_EQ(locations.size(), 3U);

	EXPECT_TRUE(locations[0].fromSpawnPoint);
	EXPECT_FLOAT_EQ(locations[0].position.x, 5.0f);
	EXPECT_FLOAT_EQ(locations[0].position.z, 1.0f);
	EXPECT_FLOAT_EQ(locations[0].facing, 0.0f);

	EXPECT_FLOAT_EQ(locations[1].position.x, 1.0f);
	EXPECT_FLOAT_EQ(locations[1].position.y, 1.0f);
	EXPECT_FLOAT_EQ(locations[1].facing, 90.0f);

	// Back at the second point: spread out, not stacked.
	const glm::vec3 delta = locations[2].position - points[1].position;
	EXPECT_NEAR(std::sqrt(delta.x * delta.x + delta.y * delta.y), 1.0f, 0.001f);
	EXPECT_FLOAT_EQ(locations[2].position.z, 1.0f);
}

GTEST_TEST(KotOREncounter, withoutSpawnPointsCreaturesComeFromFarCorners) {
	// A 20 x 20 square; whoever fired it stands near the south-west corner.
	const std::vector<glm::vec3> polygon = {
		glm::vec3( 0.0f,  0.0f, 0.0f), glm::vec3(20.0f,  0.0f, 0.0f),
		glm::vec3(20.0f, 20.0f, 0.0f), glm::vec3( 0.0f, 20.0f, 0.0f)
	};
	const glm::vec3 triggerer(2.0f, 2.0f, 0.0f);

	const std::vector<EncounterSpawnLocation> locations =
		computeEncounterSpawnLocations({}, polygon, triggerer, 2, 0);
	ASSERT_EQ(locations.size(), 2U);

	// The north-east corner first, pulled inside the polygon.
	EXPECT_FALSE(locations[0].fromSpawnPoint);
	EXPECT_GT(locations[0].position.x, 18.0f);
	EXPECT_LT(locations[0].position.x, 20.0f);
	EXPECT_GT(locations[0].position.y, 18.0f);
	EXPECT_LT(locations[0].position.y, 20.0f);

	// Facing whoever fired the encounter (south-west: 135 degrees).
	EXPECT_NEAR(std::fmod(locations[0].facing + 360.0f, 360.0f), 135.0f, 0.01f);

	// Then one of the two side corners, never the near one.
	EXPECT_GT(std::max(locations[1].position.x, locations[1].position.y), 18.0f);
	EXPECT_LT(std::min(locations[1].position.x, locations[1].position.y), 2.0f);
}
