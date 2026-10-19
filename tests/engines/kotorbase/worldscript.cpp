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
 *  Unit tests for the world-script plumbing of KotOR games: trigger
 *  occupancy, heartbeat scheduling and the rules of traps (mines).
 */

#include <algorithm>
#include <map>
#include <memory>
#include <set>
#include <vector>

#include "gtest/gtest.h"

#include "src/common/memreadstream.h"
#include "src/common/memwritestream.h"

#include "src/aurora/gff3file.h"
#include "src/aurora/gff3writer.h"

#include "src/engines/kotorbase/triggeroccupancy.h"
#include "src/engines/kotorbase/heartbeatscheduler.h"
#include "src/engines/kotorbase/trap.h"

using namespace Engines::KotORBase;

static std::unique_ptr<Aurora::GFF3File> finish(Aurora::GFF3Writer &writer) {
	Common::MemoryWriteStreamDynamic out(true);
	writer.write(out);

	// The GFF takes over the buffer.
	out.setDisposable(false);
	return std::make_unique<Aurora::GFF3File>(
		new Common::MemoryReadStream(out.getData(), out.size(), true));
}

// --- Trigger occupancy ---

GTEST_TEST(TriggerOccupancy, EnterAndExit) {
	TriggerOccupancy trigger;

	EXPECT_EQ(trigger.update(7, false), TriggerOccupancy::kChangeNone);
	EXPECT_FALSE(trigger.contains(7));

	EXPECT_EQ(trigger.update(7, true), TriggerOccupancy::kChangeEntered);
	EXPECT_TRUE(trigger.contains(7));

	// Moving around inside fires nothing.
	EXPECT_EQ(trigger.update(7, true), TriggerOccupancy::kChangeNone);
	EXPECT_EQ(trigger.size(), 1u);

	EXPECT_EQ(trigger.update(7, false), TriggerOccupancy::kChangeExited);
	EXPECT_FALSE(trigger.contains(7));
	EXPECT_TRUE(trigger.empty());

	// Walking back in fires again.
	EXPECT_EQ(trigger.update(7, true), TriggerOccupancy::kChangeEntered);
}

GTEST_TEST(TriggerOccupancy, EveryCreatureCounts) {
	TriggerOccupancy trigger;

	EXPECT_EQ(trigger.update(1, true), TriggerOccupancy::kChangeEntered);
	EXPECT_EQ(trigger.update(2, true), TriggerOccupancy::kChangeEntered);
	EXPECT_EQ(trigger.update(3, true), TriggerOccupancy::kChangeEntered);

	// One leaving doesn't make the others leave.
	EXPECT_EQ(trigger.update(2, false), TriggerOccupancy::kChangeExited);
	EXPECT_TRUE(trigger.contains(1));
	EXPECT_TRUE(trigger.contains(3));

	const std::vector<uint32_t> expected = { 1, 3 };
	EXPECT_EQ(trigger.getOccupants(), expected);
}

GTEST_TEST(TriggerOccupancy, OverlappingTriggers) {
	// A creature walks from trigger A into the part where A and B overlap,
	// then on into B alone. Each trigger sees its own enter and exit.
	TriggerOccupancy a, b;
	const uint32_t creature = 42;

	EXPECT_EQ(a.update(creature, true),  TriggerOccupancy::kChangeEntered);
	EXPECT_EQ(b.update(creature, false), TriggerOccupancy::kChangeNone);

	// Overlap: B fires although the creature is still in A.
	EXPECT_EQ(a.update(creature, true), TriggerOccupancy::kChangeNone);
	EXPECT_EQ(b.update(creature, true), TriggerOccupancy::kChangeEntered);

	EXPECT_EQ(a.update(creature, false), TriggerOccupancy::kChangeExited);
	EXPECT_EQ(b.update(creature, true),  TriggerOccupancy::kChangeNone);
	EXPECT_TRUE(b.contains(creature));
}

GTEST_TEST(TriggerOccupancy, SilentChanges) {
	TriggerOccupancy trigger;

	// Placed inside at area load: no OnEnter later for standing there.
	EXPECT_TRUE(trigger.addSilently(5));
	EXPECT_FALSE(trigger.addSilently(5));
	EXPECT_EQ(trigger.update(5, true), TriggerOccupancy::kChangeNone);

	// Walking out still fires OnExit.
	EXPECT_EQ(trigger.update(5, false), TriggerOccupancy::kChangeExited);

	// Destroyed inside: forgotten without an OnExit.
	EXPECT_EQ(trigger.update(6, true), TriggerOccupancy::kChangeEntered);
	EXPECT_TRUE(trigger.removeSilently(6));
	EXPECT_FALSE(trigger.removeSilently(6));
	EXPECT_EQ(trigger.update(6, false), TriggerOccupancy::kChangeNone);

	trigger.update(8, true);
	trigger.clear();
	EXPECT_TRUE(trigger.empty());
}

// --- Heartbeats ---

GTEST_TEST(HeartbeatScheduler, RoundLength) {
	// The Aurora round: module, area and object heartbeats every 6 seconds.
	EXPECT_EQ(HeartbeatScheduler::kInterval, 6000u);
	EXPECT_EQ(HeartbeatScheduler().getInterval(), 6000u);
}

GTEST_TEST(HeartbeatScheduler, PhaseWithinInterval) {
	for (uint32_t id = 0; id < 1000; ++id)
		EXPECT_LT(HeartbeatScheduler::getPhase(id, 6000), 6000u);

	EXPECT_EQ(HeartbeatScheduler::getPhase(12345, 0), 0u);

	// Deterministic.
	EXPECT_EQ(HeartbeatScheduler::getPhase(77, 6000), HeartbeatScheduler::getPhase(77, 6000));
}

GTEST_TEST(HeartbeatScheduler, OncePerInterval) {
	HeartbeatScheduler scheduler(6000);

	const uint32_t start = 100000;
	for (uint32_t id = 1; id <= 50; ++id)
		scheduler.add(id, start);

	EXPECT_EQ(scheduler.size(), 50u);

	std::map<uint32_t, std::vector<uint32_t>> beats;

	// 30 seconds at 60 frames per second.
	for (uint32_t now = start; now <= start + 30000; now += 16) {
		std::vector<uint32_t> due;
		scheduler.collectDue(now, due);
		for (uint32_t id : due)
			beats[id].push_back(now);
	}

	for (uint32_t id = 1; id <= 50; ++id) {
		const std::vector<uint32_t> &times = beats[id];

		// The first one within one interval of being added, then every 6 s.
		ASSERT_GE(times.size(), 4u) << "object " << id;
		EXPECT_LE(times.front() - start, 6000u + 16u);

		for (size_t i = 1; i < times.size(); ++i) {
			EXPECT_GE(times[i] - times[i - 1], 6000u - 16u);
			EXPECT_LE(times[i] - times[i - 1], 6000u + 16u);
		}
	}
}

GTEST_TEST(HeartbeatScheduler, Staggered) {
	HeartbeatScheduler scheduler(6000);

	// Objects loaded together (consecutive IDs) don't all beat on one frame.
	for (uint32_t id = 1000; id < 1060; ++id)
		scheduler.add(id, 0);

	size_t largestFrame = 0;
	std::set<uint32_t> seen;
	for (uint32_t now = 0; now < 6000; now += 16) {
		std::vector<uint32_t> due;
		scheduler.collectDue(now, due);

		largestFrame = std::max(largestFrame, due.size());
		seen.insert(due.begin(), due.end());
	}

	EXPECT_EQ(seen.size(), 60u);
	EXPECT_LE(largestFrame, 4u);
}

GTEST_TEST(HeartbeatScheduler, AddRemove) {
	HeartbeatScheduler scheduler(6000);

	scheduler.add(3, 0);
	scheduler.add(3, 0); // Only once
	EXPECT_TRUE(scheduler.contains(3));
	EXPECT_EQ(scheduler.size(), 1u);

	scheduler.remove(3);
	EXPECT_FALSE(scheduler.contains(3));

	std::vector<uint32_t> due;
	scheduler.collectDue(100000, due);
	EXPECT_TRUE(due.empty());

	scheduler.add(4, 0);
	scheduler.clear();
	EXPECT_EQ(scheduler.size(), 0u);
}

GTEST_TEST(HeartbeatScheduler, LongStallRunsOnce) {
	HeartbeatScheduler scheduler(6000);
	scheduler.add(9, 0);

	// A minute without frames (loading, a modal menu): one heartbeat, not ten.
	std::vector<uint32_t> due;
	scheduler.collectDue(60000, due);
	EXPECT_EQ(due.size(), 1u);

	due.clear();
	scheduler.collectDue(60016, due);
	EXPECT_TRUE(due.empty());

	// And the next one comes a round later, at the same phase.
	due.clear();
	scheduler.collectDue(66000, due);
	EXPECT_EQ(due.size(), 1u);
}

// --- Trap rules ---

GTEST_TEST(TrapRules, Rolls) {
	// Out of combat, the "roll" is fixed (take 10/20); in combat it's the die.
	EXPECT_EQ(TrapRules::checkRoll(false, 10, 3), 10);
	EXPECT_EQ(TrapRules::checkRoll(false, 20, 3), 20);
	EXPECT_EQ(TrapRules::checkRoll(true,  10, 3), 3);
	EXPECT_EQ(TrapRules::checkRoll(true,  20, 17), 17);
}

GTEST_TEST(TrapRules, Detection) {
	// Awareness + 10 against the DC, out of combat.
	EXPECT_TRUE (TrapRules::detects(5, 10, 15));
	EXPECT_FALSE(TrapRules::detects(4, 10, 15));
	EXPECT_TRUE (TrapRules::detects(0, 20, 20));
	EXPECT_FALSE(TrapRules::detects(0,  1, 20));

	EXPECT_GT(TrapRules::kDetectRange, TrapRules::kDisarmRange);
}

GTEST_TEST(TrapRules, Disarm) {
	// Demolitions + roll against DisarmDC.
	EXPECT_EQ(TrapRules::disarm(8, 10, 18, false), TrapRules::kDisarmSucceeded);
	EXPECT_EQ(TrapRules::disarm(7, 10, 18, false), TrapRules::kDisarmFailed);

	// Failing by 5 or more sets it off.
	EXPECT_EQ(TrapRules::disarm(4, 10, 18, false), TrapRules::kDisarmFailed);
	EXPECT_EQ(TrapRules::disarm(3, 10, 18, false), TrapRules::kDisarmSetOff);
}

GTEST_TEST(TrapRules, Recover) {
	// Recovering is harder than disarming.
	EXPECT_EQ(TrapRules::getRecoverDC(18), 18 + TrapRules::kRecoverDCPenalty);
	EXPECT_GT(TrapRules::kRecoverDCPenalty, 0);

	EXPECT_EQ(TrapRules::disarm(8, 10, 18, false), TrapRules::kDisarmSucceeded);
	EXPECT_EQ(TrapRules::disarm(9, 10, 18, true),  TrapRules::kDisarmFailed);
	EXPECT_EQ(TrapRules::disarm(13, 10, 18, true), TrapRules::kDisarmSucceeded);
	EXPECT_EQ(TrapRules::disarm(3, 10, 18, true),  TrapRules::kDisarmSetOff);
}

GTEST_TEST(TrapRules, Unlock) {
	// Security + 20 (out of combat) against the lock DC.
	EXPECT_TRUE (TrapRules::unlocks(5, 20, 25));
	EXPECT_FALSE(TrapRules::unlocks(4, 20, 25));
	EXPECT_TRUE (TrapRules::unlocks(4 + 2, 20, 25)); // With a tunneler's bonus.
}

// --- Trap state ---

/** A mine trigger the way UTT blueprints and GIT instances store it. */
static std::unique_ptr<Aurora::GFF3File> makeMine() {
	Aurora::GFF3Writer writer(MKTAG('U', 'T', 'T', ' '));
	Aurora::GFF3WriterStructPtr top = writer.getTopLevel();

	top->addExoString("Tag", "tar_mine01");
	top->addByte("Type", 2);
	top->addUint32("Faction", 1);
	top->addByte("TrapFlag", 1);
	top->addByte("TrapType", 3);
	top->addByte("TrapDetectable", 1);
	top->addByte("TrapDetectDC", 22);
	top->addByte("TrapDisarmable", 1);
	top->addByte("DisarmDC", 19);
	top->addByte("TrapOneShot", 1);
	top->addExoString("KeyName", "");
	top->addResRef("OnTrapTriggered", "k_trp_generic");

	return finish(writer);
}

GTEST_TEST(Trap, Defaults) {
	Trap trap;

	EXPECT_FALSE(trap.isTrapped());
	EXPECT_FALSE(trap.isActive());
	EXPECT_FALSE(trap.isDetected());
	EXPECT_FALSE(trap.fire());
}

GTEST_TEST(Trap, LoadMine) {
	std::unique_ptr<Aurora::GFF3File> gff = makeMine();

	Trap trap;
	trap.load(gff->getTopLevel());

	EXPECT_TRUE(trap.isTrapped());
	EXPECT_TRUE(trap.isActive());
	EXPECT_TRUE(trap.isDetectable());
	EXPECT_TRUE(trap.isDisarmable());
	EXPECT_TRUE(trap.isOneShot());
	EXPECT_FALSE(trap.isFlagged());
	EXPECT_EQ(trap.getType(), 3);
	EXPECT_EQ(trap.getDetectDC(), 22);
	EXPECT_EQ(trap.getDisarmDC(), 19);
	EXPECT_FALSE(trap.isDetected());
}

GTEST_TEST(Trap, InstanceOverridesBlueprint) {
	std::unique_ptr<Aurora::GFF3File> blueprint = makeMine();

	Aurora::GFF3Writer writer(MKTAG('G', 'I', 'T', ' '));
	writer.getTopLevel()->addByte("TrapDetectDC", 30);
	std::unique_ptr<Aurora::GFF3File> instance = finish(writer);

	Trap trap;
	trap.load(blueprint->getTopLevel());
	trap.load(instance->getTopLevel());

	// Fields the instance doesn't have keep the blueprint's value.
	EXPECT_EQ(trap.getDetectDC(), 30);
	EXPECT_EQ(trap.getDisarmDC(), 19);
	EXPECT_TRUE(trap.isActive());
}

GTEST_TEST(Trap, Detection) {
	std::unique_ptr<Aurora::GFF3File> gff = makeMine();

	Trap trap;
	trap.load(gff->getTopLevel());

	trap.setDetectedBy(100);
	trap.setDetectedBy(100);
	trap.setDetectedBy(101);

	EXPECT_TRUE(trap.isDetected());
	EXPECT_TRUE(trap.isDetectedBy(100));
	EXPECT_TRUE(trap.isDetectedBy(101));
	EXPECT_FALSE(trap.isDetectedBy(102));
	EXPECT_EQ(trap.getDetectors().size(), 2u);

	trap.setDetectedBy(100, false);
	EXPECT_FALSE(trap.isDetectedBy(100));
	EXPECT_TRUE(trap.isDetected());
}

GTEST_TEST(Trap, FireOneShot) {
	std::unique_ptr<Aurora::GFF3File> gff = makeMine();

	Trap trap;
	trap.load(gff->getTopLevel());

	EXPECT_FALSE(trap.fire()); // Went off, and is spent.
	EXPECT_FALSE(trap.isActive());
	EXPECT_FALSE(trap.fire());
}

GTEST_TEST(Trap, FireRepeating) {
	std::unique_ptr<Aurora::GFF3File> gff = makeMine();

	Trap trap;
	trap.load(gff->getTopLevel());
	trap.setOneShot(false);

	EXPECT_TRUE(trap.fire());
	EXPECT_TRUE(trap.fire());
	EXPECT_TRUE(trap.isActive());
}

GTEST_TEST(Trap, Disable) {
	std::unique_ptr<Aurora::GFF3File> gff = makeMine();

	Trap trap;
	trap.load(gff->getTopLevel());
	trap.setDetectedBy(5);
	trap.setFlagged(true);

	trap.disable();

	EXPECT_FALSE(trap.isTrapped());
	EXPECT_FALSE(trap.isActive());
	EXPECT_FALSE(trap.isDetected());
	EXPECT_FALSE(trap.isFlagged());
}

GTEST_TEST(Trap, StateRoundTrip) {
	std::unique_ptr<Aurora::GFF3File> gff = makeMine();

	Trap trap;
	trap.load(gff->getTopLevel());
	trap.setDetectedBy(77);

	Aurora::GFF3Writer writer(MKTAG('G', 'F', 'F', ' '));
	trap.saveState(*writer.getTopLevel());
	std::unique_ptr<Aurora::GFF3File> state = finish(writer);

	Trap loaded;
	loaded.loadState(state->getTopLevel());

	EXPECT_TRUE(loaded.isTrapped());
	EXPECT_TRUE(loaded.isActive());
	EXPECT_EQ(loaded.getType(), 3);
	EXPECT_EQ(loaded.getDetectDC(), 22);
	EXPECT_EQ(loaded.getDisarmDC(), 19);

	// Object IDs don't survive; that the party had found it does.
	EXPECT_TRUE(loaded.isDetected());
	EXPECT_FALSE(loaded.isDetectedBy(77));
}

GTEST_TEST(Trap, SpentStateRoundTrip) {
	std::unique_ptr<Aurora::GFF3File> gff = makeMine();

	Trap trap;
	trap.load(gff->getTopLevel());
	trap.fire();

	Aurora::GFF3Writer writer(MKTAG('G', 'F', 'F', ' '));
	trap.saveState(*writer.getTopLevel());
	std::unique_ptr<Aurora::GFF3File> state = finish(writer);

	Trap loaded;
	loaded.loadState(state->getTopLevel());

	EXPECT_TRUE(loaded.isTrapped());
	EXPECT_FALSE(loaded.isActive());
	EXPECT_FALSE(loaded.isDetected());
}

GTEST_TEST(Trap, TypeInfo) {
	Trap trap;
	trap.setTypeInfo("plc_mine", "gr_explode", "g_i_trapkit001", 12345);

	EXPECT_EQ(trap.getModel(), "plc_mine");
	EXPECT_EQ(trap.getExplosionSound(), "gr_explode");
	EXPECT_EQ(trap.getItemResRef(), "g_i_trapkit001");
	EXPECT_EQ(trap.getNameStrRef(), 12345u);
}
