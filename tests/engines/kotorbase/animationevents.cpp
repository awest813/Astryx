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
 *  Unit tests for the events of model animations.
 */

#include <set>
#include <vector>

#include "gtest/gtest.h"

#include "src/common/ustring.h"

#include "src/graphics/aurora/animation.h"

#include "src/engines/kotorbase/animationnames.h"

using Graphics::Aurora::Animation;
using Engines::KotORBase::HasAnimation;
using Engines::KotORBase::chooseAttackAnimation;
using Engines::KotORBase::chooseDefenseAnimation;
using Engines::KotORBase::isMeleeWeaponAnimation;

/** A walk cycle: two footsteps, and events at both ends of the loop. */
static void makeWalk(Animation &anim) {
	anim.setLength(1.0f);
	anim.addEvent(0.0f , "start");
	anim.addEvent(0.25f, "snd_footstep");
	anim.addEvent(0.75f, "snd_footstep");
	anim.addEvent(1.0f , "end");
}

static std::vector<Common::UString> collect(const Animation &anim, float from, float to, bool loopEnd) {
	std::vector<Common::UString> names;
	anim.collectEvents(from, to, loopEnd, names);
	return names;
}

GTEST_TEST(AnimationEvents, eventsAreKept) {
	Animation anim;
	makeWalk(anim);

	ASSERT_EQ(anim.getEvents().size(), 4u);
	EXPECT_FLOAT_EQ(anim.getEvents()[1].time, 0.25f);
	EXPECT_STREQ(anim.getEvents()[1].name.c_str(), "snd_footstep");
}

GTEST_TEST(AnimationEvents, stepCollectsWhatItPasses) {
	Animation anim;
	makeWalk(anim);

	EXPECT_TRUE(collect(anim, 0.3f, 0.5f, false).empty());

	const std::vector<Common::UString> step = collect(anim, 0.2f, 0.3f, false);
	ASSERT_EQ(step.size(), 1u);
	EXPECT_STREQ(step[0].c_str(), "snd_footstep");

	// A long step passes both footsteps, in order
	const std::vector<Common::UString> both = collect(anim, 0.1f, 0.9f, false);
	ASSERT_EQ(both.size(), 2u);
}

GTEST_TEST(AnimationEvents, stepIsHalfOpen) {
	Animation anim;
	makeWalk(anim);

	// An event exactly at a step's boundary fires once: in the step starting there
	EXPECT_TRUE(collect(anim, 0.1f, 0.25f, false).empty());
	EXPECT_EQ(collect(anim, 0.25f, 0.4f, false).size(), 1u);

	// A step that doesn't advance (a new animation's first frame, frozen time) fires nothing
	EXPECT_TRUE(collect(anim, 0.0f, 0.0f, false).empty());
	EXPECT_TRUE(collect(anim, 0.25f, 0.25f, false).empty());
}

GTEST_TEST(AnimationEvents, loopEndIncludesTheLastFrame) {
	Animation anim;
	makeWalk(anim);

	const std::vector<Common::UString> last = collect(anim, 0.9f, 1.0f, true);
	ASSERT_EQ(last.size(), 1u);
	EXPECT_STREQ(last[0].c_str(), "end");

	EXPECT_TRUE(collect(anim, 0.9f, 1.0f, false).empty());
}

GTEST_TEST(AnimationEvents, wrapAroundCollectsBothEnds) {
	Animation anim;
	makeWalk(anim);

	// From 0.8 past the end of the loop to 0.3 into the next one
	const std::vector<Common::UString> wrapped = collect(anim, 0.8f, 0.3f, false);
	ASSERT_EQ(wrapped.size(), 3u);
	EXPECT_STREQ(wrapped[0].c_str(), "end");
	EXPECT_STREQ(wrapped[1].c_str(), "start");
	EXPECT_STREQ(wrapped[2].c_str(), "snd_footstep");
}

GTEST_TEST(AnimationEvents, loopingFiresEachEventOncePerLoop) {
	Animation anim;
	makeWalk(anim);

	/* Step through three loops the way the animation channel does: the step
	 * reaching the end of a loop includes the last frame, the next loop starts
	 * at 0. Uneven steps, so that some land on events exactly. */
	const float steps[] = { 0.1f, 0.15f, 0.2f, 0.3f, 0.25f };

	std::vector<Common::UString> fired;
	float time = 0.0f;
	int loops = 0;
	size_t s = 0;

	while (loops < 3) {
		float next = time + steps[s++ % (sizeof(steps) / sizeof(steps[0]))];

		if (next >= anim.getLength()) {
			anim.collectEvents(time, anim.getLength(), true, fired);
			time = 0.0f;
			loops++;
			continue;
		}

		anim.collectEvents(time, next, false, fired);
		time = next;
	}

	size_t footsteps = 0, starts = 0, ends = 0;
	for (const Common::UString &name : fired) {
		if (name == "snd_footstep")
			footsteps++;
		else if (name == "start")
			starts++;
		else if (name == "end")
			ends++;
	}

	EXPECT_EQ(footsteps, 6u);
	EXPECT_EQ(starts, 3u);
	EXPECT_EQ(ends, 3u);
}

GTEST_TEST(AnimationEvents, noEventsNoNames) {
	Animation anim;
	anim.setLength(2.0f);

	EXPECT_TRUE(collect(anim, 0.0f, 2.0f, true).empty());
	EXPECT_TRUE(collect(anim, 1.5f, 0.5f, false).empty());
}

// --- Combat animations ---

static HasAnimation modelWith(const std::set<Common::UString> &anims) {
	return [anims](const Common::UString &anim) { return anims.count(anim) > 0; };
}

GTEST_TEST(KotORCombatAnimations, meleeOrRanged) {
	EXPECT_TRUE(isMeleeWeaponAnimation(1));
	EXPECT_TRUE(isMeleeWeaponAnimation(4));
	EXPECT_TRUE(isMeleeWeaponAnimation(8));
	EXPECT_FALSE(isMeleeWeaponAnimation(5));
	EXPECT_FALSE(isMeleeWeaponAnimation(7));
	EXPECT_FALSE(isMeleeWeaponAnimation(-1));
}

GTEST_TEST(KotORCombatAnimations, attackByWeaponAndEngagement) {
	const HasAnimation human = modelWith({ "g2a1", "g2a2", "c2a1", "c2a2", "c2a3", "c2a4", "c2a5",
	                                       "b5a1", "g8a1", "g8a2", "b7a1" });

	// Ranged weapons shoot
	EXPECT_STREQ(chooseAttackAnimation(5, false, 0, human).c_str(), "b5a1");
	EXPECT_STREQ(chooseAttackAnimation(7, true , 3, human).c_str(), "b7a1");

	// Melee, not engaged: the plain attacks
	EXPECT_STREQ(chooseAttackAnimation(2, false, 0, human).c_str(), "g2a1");
	EXPECT_STREQ(chooseAttackAnimation(2, false, 1, human).c_str(), "g2a2");

	// Dueling: one of the five dueling attacks
	EXPECT_STREQ(chooseAttackAnimation(2, true, 0, human).c_str(), "c2a1");
	EXPECT_STREQ(chooseAttackAnimation(2, true, 7, human).c_str(), "c2a3");

	// Unarmed has no dueling attacks
	EXPECT_STREQ(chooseAttackAnimation(8, true, 1, human).c_str(), "g8a2");
}

GTEST_TEST(KotORCombatAnimations, attackFallsBackToWhatTheModelHas) {
	const HasAnimation beast = modelWith({ "g8a1" });

	EXPECT_STREQ(chooseAttackAnimation(8, false, 1, beast).c_str(), "g8a1");
	EXPECT_TRUE (chooseAttackAnimation(2, true , 1, beast).empty());
	EXPECT_TRUE (chooseAttackAnimation(-1, false, 0, beast).empty());

	// A model without the second variant uses the first
	const HasAnimation plain = modelWith({ "g1a1" });
	EXPECT_STREQ(chooseAttackAnimation(1, true, 1, plain).c_str(), "g1a1");
}

GTEST_TEST(KotORCombatAnimations, defenseAnswersTheAttack) {
	const HasAnimation human = modelWith({ "g2g1", "c2p1", "c2p3", "g5g1", "g8g1" });

	// The matching parry of a dueling attack
	EXPECT_STREQ(chooseDefenseAnimation(2, "c2a3", human).c_str(), "c2p3");
	// No such parry in the model, or another kind of weapon: the plain dodge
	EXPECT_STREQ(chooseDefenseAnimation(2, "c2a4", human).c_str(), "g2g1");
	EXPECT_STREQ(chooseDefenseAnimation(5, "c2a1", human).c_str(), "g5g1");
	EXPECT_STREQ(chooseDefenseAnimation(2, "g2a1", human).c_str(), "g2g1");
	EXPECT_STREQ(chooseDefenseAnimation(8, ""    , human).c_str(), "g8g1");

	EXPECT_TRUE(chooseDefenseAnimation(7, "b7a1", human).empty());
	EXPECT_TRUE(chooseDefenseAnimation(-1, "c2a1", human).empty());
}
