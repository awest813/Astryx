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
 *  Unit tests for the engine features the Endar Spire's cutscene, combat and
 *  effect scripts use: PlayExplosion's screen shake, CorridorExplosion's
 *  beam-linked damage, CutsceneAttack, damage to placeables, facings
 *  (SetFacing, SetFacingPoint, Location), ActionPlayAnimation, scripted moves,
 *  CancelCombat, SetLightsaberPowered and ActionCastFakeSpellAtObject.
 */

#include <cstdlib>
#include <cmath>
#include <memory>

#include "gtest/gtest.h"

#include "src/common/memreadstream.h"

#include "src/aurora/2dafile.h"
#include "src/aurora/nwscript/types.h"
#include "src/aurora/nwscript/variable.h"
#include "src/aurora/nwscript/functioncontext.h"

#include "src/graphics/aurora/model.h"

#include "src/engines/kotorbase/action.h"
#include "src/engines/kotorbase/animationnames.h"
#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/effect.h"
#include "src/engines/kotorbase/game.h"
#include "src/engines/kotorbase/location.h"
#include "src/engines/kotorbase/situated.h"
#include "src/engines/kotorbase/types.h"
#include "src/engines/kotorbase/visualeffects.h"
#include "src/engines/kotorbase/script/functions.h"

using namespace Engines::KotORBase;

using Aurora::NWScript::FunctionContext;
using Aurora::NWScript::Signature;
using Aurora::NWScript::kTypeVoid;
using Aurora::NWScript::kTypeInt;
using Aurora::NWScript::kTypeFloat;
using Aurora::NWScript::kTypeObject;
using Aurora::NWScript::kTypeVector;
using Aurora::NWScript::kTypeEngineType;

namespace {

// Script constants (nwscript.nss).
const float kDirectionEast  =   0.0f;
const float kDirectionNorth =  90.0f;
const float kDirectionWest  = 180.0f;
const float kDirectionSouth = 270.0f;

const int kAnimationLoopingPause         =   0;
const int kAnimationLoopingListen        =   2;
const int kAnimationLoopingTalkNormal    =   5;
const int kAnimationLoopingReady         =  19;
const int kAnimationFireForgetHeadTurnL  = 100;

const int kScriptAttackHit = 1;
const int kScriptAttackMiss          = 4;

const int kForcePowerChoke = 9; // Any spells.2da row does here.

/** The natives under test don't need a module. */
class TestGame : public Game {
public:
	TestGame() : Game(dummyConsole()) {}

	bool hasModule(const Common::UString &) const override { return false; }
	Functions &getFunctions() override { std::abort(); }
	void run() override {}
	void showLevelUpGUI(Creature *) override {}
	void openStoreGUI(const Common::UString &) override {}

protected:
	const Common::UString &getDefaultMenuMusic() const override {
		static const Common::UString kMusic;
		return kMusic;
	}

private:
	static Engines::Console &dummyConsole() {
		// Game only stores the console's address.
		alignas(16) static unsigned char storage[16];
		return reinterpret_cast<Engines::Console &>(storage);
	}
};

class TestFunctions : public Functions {
public:
	explicit TestFunctions(Game &game) : Functions(game) {}

	void registerFunctions() override {}

	using Functions::setFacing;
	using Functions::getFacing;
	using Functions::setFacingPoint;
	using Functions::locationCreate;
	using Functions::getFacingFromLocation;
	using Functions::cancelCombat;
	using Functions::setLightsaberPowered;
	using Functions::actionPlayAnimation;
	using Functions::cutsceneAttack;
	using Functions::actionMoveToObject;
	using Functions::actionForceMoveToObject;
	using Functions::actionCastFakeSpellAtObject;
	using Functions::playRumblePattern;
	using Functions::applyEffectToNonCreature;
};

class TestCreature : public Creature {
public:
	TestCreature() : Creature() {}

protected:
	void getPartModelsPC(PartModels &parts, uint32_t state, uint8_t textureVariation) override {
		(void)parts;
		(void)state;
		(void)textureVariation;
	}
};

/** A placeable without a blueprint: the Endar Spire's exploding barrel. */
class TestPlaceable : public Situated {
public:
	TestPlaceable(int hitPoints) : Situated(kObjectTypePlaceable) {
		setMaxHitPoints(hitPoints);
		setCurrentHitPoints(hitPoints);
	}

	void setHardness(int hardness) { _hardness = hardness; }

	bool isOpen() const override { return false; }

protected:
	void loadObject(const Aurora::GFF3Struct &) override {}
	void loadAppearance() override {}
};

struct Fixture {
	TestGame game;
	TestFunctions functions { game };
};

void setHP(Creature &c, int hp) {
	c.setMaxHitPoints(hp);
	c.setCurrentHitPoints(hp);
}

float getOrientationAngle(const Object &object) {
	float x, y, z, angle;
	object.getOrientation(x, y, z, angle);
	return angle;
}

float callGetFacing(Fixture &f, Creature &creature) {
	FunctionContext ctx("GetFacing");
	ctx.setSignature(Signature { kTypeFloat, kTypeObject });
	ctx.getParams()[0] = static_cast<Aurora::NWScript::Object *>(&creature);
	f.functions.getFacing(ctx);
	return ctx.getReturn().getFloat();
}

void callSetFacing(Fixture &f, Creature &caller, float facing) {
	FunctionContext ctx("SetFacing");
	ctx.setSignature(Signature { kTypeVoid, kTypeFloat });
	ctx.setCaller(&caller);
	ctx.getParams()[0] = facing;
	f.functions.setFacing(ctx);
}

Aurora::TwoDAFile *makeTwoDA(const char *text) {
	Common::MemoryReadStream stream(text);
	return new Aurora::TwoDAFile(stream);
}

} // End of anonymous namespace

// --- Facings ---

GTEST_TEST(EndarSpireEffects, scriptFacingsMatchTheOrientationOfLookingThatWay) {
	TestCreature creature;
	creature.setPosition(0.0f, 0.0f, 0.0f);

	// makeLookAt() is how creatures face where they walk and whom they fight.
	creature.makeLookAt(1.0f, 0.0f);
	EXPECT_NEAR(orientationToScriptFacing(getOrientationAngle(creature)), kDirectionEast, 0.01f);
	creature.makeLookAt(0.0f, 1.0f);
	EXPECT_NEAR(orientationToScriptFacing(getOrientationAngle(creature)), kDirectionNorth, 0.01f);
	creature.makeLookAt(-1.0f, 0.0f);
	EXPECT_NEAR(orientationToScriptFacing(getOrientationAngle(creature)), kDirectionWest, 0.01f);
	creature.makeLookAt(0.0f, -1.0f);
	EXPECT_NEAR(orientationToScriptFacing(getOrientationAngle(creature)), kDirectionSouth, 0.01f);

	// Round trips, and k_pend_xpboost's negative facing.
	EXPECT_NEAR(orientationToScriptFacing(scriptFacingToOrientation(-57.54f)), 302.46f, 0.01f);
	EXPECT_NEAR(orientationToScriptFacing(scriptFacingToOrientation(241.83f)), 241.83f, 0.01f);
}

GTEST_TEST(EndarSpireEffects, setFacingTurnsTheCallerThatWay) {
	// cp_end01_pcjump: Trask faces west, the player east, towards each other.
	Fixture f;
	TestCreature trask;
	trask.setPosition(0.0f, 0.0f, 0.0f);

	callSetFacing(f, trask, kDirectionWest);
	EXPECT_NEAR(callGetFacing(f, trask), kDirectionWest, 0.01f);

	TestCreature lookingWest;
	lookingWest.setPosition(0.0f, 0.0f, 0.0f);
	lookingWest.makeLookAt(-5.0f, 0.0f);
	EXPECT_NEAR(getOrientationAngle(trask), getOrientationAngle(lookingWest), 0.01f);

	callSetFacing(f, trask, kDirectionSouth);
	EXPECT_NEAR(callGetFacing(f, trask), kDirectionSouth, 0.01f);
}

GTEST_TEST(EndarSpireEffects, setFacingPointLooksAtThePoint) {
	Fixture f;
	TestCreature trask;
	trask.setPosition(10.0f, 10.0f, 0.0f);

	FunctionContext ctx("SetFacingPoint");
	ctx.setSignature(Signature { kTypeVoid, kTypeVector });
	ctx.setCaller(&trask);
	ctx.getParams()[0].setVector(10.0f, 4.0f, 0.0f); // Due south
	f.functions.setFacingPoint(ctx);

	EXPECT_NEAR(callGetFacing(f, trask), kDirectionSouth, 0.01f);
}

GTEST_TEST(EndarSpireEffects, locationFacingsAreScriptFacings) {
	// cp_end_trasksp_d: Location(Vector(16.34, 20.5, -1.27), 180.0) faces west.
	Fixture f;

	FunctionContext create("Location");
	create.setSignature(Signature { kTypeEngineType, kTypeVector, kTypeFloat });
	create.getParams()[0].setVector(16.34f, 20.5f, -1.27f);
	create.getParams()[1] = kDirectionWest;
	f.functions.locationCreate(create);

	const Location *location = dynamic_cast<const Location *>(create.getReturn().getEngineType());
	ASSERT_NE(location, nullptr);

	// Jumping there gives this orientation.
	TestCreature lookingWest;
	lookingWest.setPosition(0.0f, 0.0f, 0.0f);
	lookingWest.makeLookAt(-5.0f, 0.0f);
	EXPECT_NEAR(location->getFacing(), getOrientationAngle(lookingWest), 0.01f);

	FunctionContext facing("GetFacingFromLocation");
	facing.setSignature(Signature { kTypeFloat, kTypeEngineType });
	facing.getParams()[0] = create.getReturn().getEngineType();
	f.functions.getFacingFromLocation(facing);
	EXPECT_NEAR(facing.getReturn().getFloat(), kDirectionWest, 0.01f);
}

// --- Combat ---

GTEST_TEST(EndarSpireEffects, cancelCombatCancelsTheCreatureGiven) {
	// cp_end_trasksp_d: the helper placeable's script cancels Trask's and the PC's combat.
	Fixture f;
	TestCreature helper, trask, sith;

	trask.startCombat(&sith, 0);
	helper.startCombat(&sith, 0);
	ASSERT_TRUE(trask.isInCombat());

	FunctionContext ctx("CancelCombat");
	ctx.setSignature(Signature { kTypeVoid, kTypeObject });
	ctx.setCaller(&helper);
	ctx.getParams()[0] = static_cast<Aurora::NWScript::Object *>(&trask);
	f.functions.cancelCombat(ctx);

	EXPECT_FALSE(trask.isInCombat());
	EXPECT_TRUE(helper.isInCombat());
}

GTEST_TEST(EndarSpireEffects, cutsceneAttackQueuesTheScriptedBlow) {
	// k_pend_cut2_1: CutsceneAttack(oRepSol, 156, ATTACK_RESULT_HIT_SUCCESSFUL, 140)
	Fixture f;
	TestCreature sith, soldier;
	setHP(soldier, 30);

	FunctionContext ctx("CutsceneAttack");
	ctx.setSignature(Signature { kTypeVoid, kTypeObject, kTypeInt, kTypeInt, kTypeInt });
	ctx.setCaller(&sith);
	ctx.getParams()[0] = static_cast<Aurora::NWScript::Object *>(&soldier);
	ctx.getParams()[1] = static_cast<int32_t>(156);
	ctx.getParams()[2] = static_cast<int32_t>(kScriptAttackHit);
	ctx.getParams()[3] = static_cast<int32_t>(140);
	f.functions.cutsceneAttack(ctx);

	const Action *action = sith.getCurrentAction();
	ASSERT_NE(action, nullptr);
	EXPECT_EQ(action->type, kActionCutsceneAttack);
	EXPECT_EQ(action->object, static_cast<Object *>(&soldier));
	EXPECT_EQ(action->actionID, 156);
	EXPECT_EQ(action->damage, 140);
	EXPECT_TRUE(action->choreographyFlags & kCutsceneAttackForceHit);
	EXPECT_TRUE(action->choreographyFlags & kCutsceneAttackFinishingBlow);

	// The blow lands: the soldier dies (his OnDeath, and so 1007, runs from the area).
	sith.performCutsceneAttack(&soldier, action->choreographyFlags, action->damage, action->actionID);
	EXPECT_TRUE(soldier.isDead());
	EXPECT_TRUE(soldier.consumeDeathEvent());
}

GTEST_TEST(EndarSpireEffects, cutsceneAttackMissesAndShotsAtPlaceables) {
	// k_pend_explosion: the Sith shoots the barrel, a soldier shoots and misses.
	Fixture f;
	TestCreature sith, soldier;
	TestPlaceable barrel(10);
	setHP(sith, 30);

	FunctionContext ctx("CutsceneAttack");
	ctx.setSignature(Signature { kTypeVoid, kTypeObject, kTypeInt, kTypeInt, kTypeInt });
	ctx.setCaller(&sith);
	ctx.getParams()[0] = static_cast<Aurora::NWScript::Object *>(&barrel);
	ctx.getParams()[1] = static_cast<int32_t>(239);
	ctx.getParams()[2] = static_cast<int32_t>(kScriptAttackHit);
	ctx.getParams()[3] = static_cast<int32_t>(0);
	f.functions.cutsceneAttack(ctx);

	const Action *shot = sith.getCurrentAction();
	ASSERT_NE(shot, nullptr);
	EXPECT_EQ(shot->object, static_cast<Object *>(&barrel));
	EXPECT_TRUE(shot->choreographyFlags & kCutsceneAttackNoDamage);
	EXPECT_EQ(shot->damage, 0);

	ctx.setCaller(&soldier);
	ctx.getParams()[0] = static_cast<Aurora::NWScript::Object *>(&sith);
	ctx.getParams()[2] = static_cast<int32_t>(kScriptAttackMiss);
	ctx.getParams()[3] = static_cast<int32_t>(50);
	f.functions.cutsceneAttack(ctx);

	const Action *miss = soldier.getCurrentAction();
	ASSERT_NE(miss, nullptr);
	EXPECT_TRUE(miss->choreographyFlags & kCutsceneAttackForceMiss);
	EXPECT_EQ(miss->damage, 0);

	soldier.performCutsceneAttack(&sith, miss->choreographyFlags, miss->damage, miss->actionID);
	EXPECT_EQ(sith.getCurrentHitPoints(), 30);
}

GTEST_TEST(EndarSpireEffects, corridorExplosionDamageIsAppliedWithTheBeam) {
	// CorridorExplosion(): EffectLinkEffects(EffectDamage(5), EffectBeam(...)).
	const Effect zap = linkEffects(Effect(kKotOREffectDamage, 5), Effect(kKotOREffectBeam, 2));

	TestCreature pc;
	setHP(pc, 20);
	pc.applyEffect(zap, 2.0f);
	EXPECT_EQ(pc.getCurrentHitPoints(), 15);

	// The player is Min1HP on the Endar Spire: hurt, but never killed by it.
	pc.setCurrentHitPoints(3);
	pc.setMinOneHitPoints(true);
	pc.applyEffect(zap, 2.0f);
	EXPECT_EQ(pc.getCurrentHitPoints(), 1);
	EXPECT_FALSE(pc.isDead());

	// On a placeable as well.
	Fixture f;
	TestPlaceable panel(10);
	f.functions.applyEffectToNonCreature(zap, panel, nullptr, 0.0f);
	EXPECT_EQ(panel.getCurrentHitPoints(), 5);
}

GTEST_TEST(EndarSpireEffects, effectDamageDestroysAPlaceable) {
	// k_pend_explosion: ApplyEffectToObject(INSTANT, EffectDamage(100), oExplode).
	Fixture f;
	TestCreature sith;
	TestPlaceable barrel(10);
	barrel.setHardness(5); // Effects aren't blows: no Hardness.

	f.functions.applyEffectToNonCreature(Effect(kKotOREffectDamage, 100), barrel, &sith, 0.0f);

	EXPECT_EQ(barrel.getCurrentHitPoints(), 0);
	EXPECT_TRUE(barrel.isDead());
	EXPECT_EQ(barrel.getLastHostileActor(), static_cast<Object *>(&sith));
	EXPECT_TRUE(barrel.consumeDamagedEvent());
	EXPECT_TRUE(barrel.consumeDeathEvent());

	// Plot placeables and those without hit points can't be harmed.
	TestPlaceable plot(10);
	plot.setPlotFlag(true);
	plot.takeDamage(100, nullptr);
	EXPECT_FALSE(plot.isDead());
	EXPECT_EQ(plot.getCurrentHitPoints(), 10);

	TestPlaceable scenery(0);
	scenery.takeDamage(100, nullptr);
	EXPECT_FALSE(scenery.isDead());
	EXPECT_FALSE(scenery.consumeDeathEvent());

	// Blows still lose the Hardness.
	TestPlaceable crate(10);
	crate.setHardness(5);
	crate.takeBashDamage(8, &sith);
	EXPECT_EQ(crate.getCurrentHitPoints(), 7);
}

// --- Screen shake ---

GTEST_TEST(EndarSpireEffects, screenShakeVisualShakesTheCamera) {
	std::unique_ptr<Aurora::TwoDAFile> withColumns(makeTwoDA(
		"2DA V2.0\n"
		"\n"
		"   label                          type_fd shaketype shakedelay shakeduration\n"
		"0  VFX_IMP_SCREEN_SHAKE           F       1         0.0        2.0\n"
		"1  VFX_FNF_GRENADE_FRAGMENTATION  F       ****      ****       ****\n"
		"2  VFX_FNF_BIG_BANG               F       3         0.0        1500\n"));

	VisualEffectInfo info;
	float duration = 0.0f, intensity = 0.0f;

	ASSERT_TRUE(loadVisualEffect(*withColumns, 0, info));
	EXPECT_TRUE(getVisualEffectShake(info, duration, intensity));
	EXPECT_FLOAT_EQ(duration, 2.0f);
	EXPECT_GT(intensity, 0.0f);

	ASSERT_TRUE(loadVisualEffect(*withColumns, 1, info));
	EXPECT_FALSE(getVisualEffectShake(info, duration, intensity));

	const float lightShake = intensity;
	ASSERT_TRUE(loadVisualEffect(*withColumns, 2, info));
	EXPECT_TRUE(getVisualEffectShake(info, duration, intensity));
	EXPECT_FLOAT_EQ(duration, 1.5f); // Milliseconds
	EXPECT_GE(intensity, lightShake);

	// A table without the shake columns: the screen shake row still shakes.
	std::unique_ptr<Aurora::TwoDAFile> withoutColumns(makeTwoDA(
		"2DA V2.0\n"
		"\n"
		"   label                          type_fd\n"
		"0  VFX_IMP_SCREEN_SHAKE           F\n"
		"1  VFX_FNF_GRENADE_FRAGMENTATION  F\n"));

	ASSERT_TRUE(loadVisualEffect(*withoutColumns, 0, info));
	EXPECT_TRUE(getVisualEffectShake(info, duration, intensity));
	EXPECT_GT(duration, 0.0f);

	ASSERT_TRUE(loadVisualEffect(*withoutColumns, 1, info));
	EXPECT_FALSE(getVisualEffectShake(info, duration, intensity));
}

GTEST_TEST(EndarSpireEffects, playRumblePatternIsHarmless) {
	Fixture f;
	FunctionContext ctx("PlayRumblePattern");
	ctx.setSignature(Signature { kTypeInt, kTypeInt });
	ctx.getParams()[0] = static_cast<int32_t>(14);
	f.functions.playRumblePattern(ctx);
	EXPECT_EQ(ctx.getReturn().getInt(), 0);
}

// --- Animations ---

GTEST_TEST(EndarSpireEffects, scriptAnimationConstantsMapToTheirAnimations) {
	// Without animations.2da: the usual model animation names.
	EXPECT_STREQ(getScriptAnimationName(kAnimationLoopingPause, -1, false).c_str(), "pause1");
	EXPECT_STREQ(getScriptAnimationName(kAnimationLoopingListen, -1, false).c_str(), "listen");
	EXPECT_STREQ(getScriptAnimationName(kAnimationLoopingTalkNormal, -1, false).c_str(), "talknorm");
	EXPECT_STREQ(getScriptAnimationName(kAnimationFireForgetHeadTurnL, -1, false).c_str(), "hturnl");

	// READY is the stance of the weapon held: Bandon's lightsaber, a soldier's rifle.
	EXPECT_STREQ(getScriptAnimationName(kAnimationLoopingReady, 2, false).c_str(), "g2r1");
	EXPECT_STREQ(getScriptAnimationName(kAnimationLoopingReady, 7, false).c_str(), "g7r1");
	EXPECT_STREQ(getScriptAnimationName(kAnimationLoopingReady, -1, false).c_str(), "g8r1");

	EXPECT_TRUE(getScriptAnimationName(999, -1, false).empty());

	EXPECT_TRUE(isFireAndForgetScriptAnimation(kAnimationFireForgetHeadTurnL));
	EXPECT_FALSE(isFireAndForgetScriptAnimation(kAnimationLoopingReady));
}

GTEST_TEST(EndarSpireEffects, playAnimationTiming) {
	// k_pend_cut23: ActionPlayAnimation(ANIMATION_LOOPING_READY, 1.0, 10.0)
	PlayAnimationTiming t = getPlayAnimationTiming(kAnimationLoopingReady, 10.0f, 1.0f, 2.0f);
	EXPECT_FLOAT_EQ(t.modelLength, 10.0f);
	EXPECT_FLOAT_EQ(t.wait, 10.0f);
	EXPECT_TRUE(t.backToDefault);

	// k_pend_xpboost: ActionPlayAnimation(ANIMATION_LOOPING_PAUSE, 1.0, 0.1)
	t = getPlayAnimationTiming(kAnimationLoopingPause, 0.1f, 1.0f, 3.0f);
	EXPECT_FLOAT_EQ(t.wait, 0.1f);

	// k_pend_cut32: ActionPlayAnimation(ANIMATION_FIREFORGET_HEAD_TURN_LEFT): once.
	t = getPlayAnimationTiming(kAnimationFireForgetHeadTurnL, 0.0f, 1.0f, 1.5f);
	EXPECT_FLOAT_EQ(t.modelLength, 0.0f);
	EXPECT_FLOAT_EQ(t.wait, 1.5f);
	EXPECT_FALSE(t.backToDefault);

	// Fire-and-forget animations ignore a duration; the speed shortens them.
	t = getPlayAnimationTiming(kAnimationFireForgetHeadTurnL, 5.0f, 2.0f, 1.5f);
	EXPECT_FLOAT_EQ(t.modelLength, 0.0f);
	EXPECT_FLOAT_EQ(t.wait, 0.75f);

	// A negative duration loops until the next animation, without holding the queue.
	t = getPlayAnimationTiming(kAnimationLoopingTalkNormal, -1.0f, 1.0f, 2.0f);
	EXPECT_FLOAT_EQ(t.modelLength, -1.0f);
	EXPECT_FLOAT_EQ(t.wait, 0.0f);
}

GTEST_TEST(EndarSpireEffects, actionPlayAnimationQueuesTheRequest) {
	Fixture f;
	TestCreature bandon;

	FunctionContext ctx("ActionPlayAnimation");
	ctx.setSignature(Signature { kTypeVoid, kTypeInt, kTypeFloat, kTypeFloat });
	ctx.setCaller(&bandon);
	ctx.getParams()[0] = static_cast<int32_t>(kAnimationLoopingReady);
	ctx.getParams()[1] = 1.0f;
	ctx.getParams()[2] = 10.0f;
	f.functions.actionPlayAnimation(ctx);

	const Action *action = bandon.getCurrentAction();
	ASSERT_NE(action, nullptr);
	EXPECT_EQ(action->type, kActionPlayAnimation);
	EXPECT_EQ(action->actionID, kAnimationLoopingReady);
	EXPECT_FLOAT_EQ(action->range, 1.0f);
	EXPECT_FLOAT_EQ(action->startTime, 10.0f);
	EXPECT_EQ(action->step, 0);
}

// --- Movement, the Force, lightsabers ---

GTEST_TEST(EndarSpireEffects, scriptedMovesWalkOrRunAsAsked) {
	Fixture f;
	TestCreature bandon, waypoint;
	waypoint.setPosition(5.0f, 0.0f, 0.0f);

	// k_pend_cut23: ActionMoveToObject(oWP), Bandon walks.
	FunctionContext walk("ActionMoveToObject");
	walk.setSignature(Signature { kTypeVoid, kTypeObject, kTypeInt, kTypeFloat });
	walk.setCaller(&bandon);
	walk.getParams()[0] = static_cast<Aurora::NWScript::Object *>(&waypoint);
	walk.getParams()[1] = static_cast<int32_t>(0);
	walk.getParams()[2] = 1.0f;
	f.functions.actionMoveToObject(walk);

	const Action *action = bandon.getCurrentAction();
	ASSERT_NE(action, nullptr);
	EXPECT_EQ(action->type, kActionMoveToPoint);
	EXPECT_EQ(action->choreographyFlags, kMoveWalk);
	EXPECT_FLOAT_EQ(action->timeout, 0.0f);
	EXPECT_FLOAT_EQ(action->location.x, 5.0f);

	// k_pend_cut14: ActionForceMoveToObject(oDie_WP, TRUE), Trask runs and
	// jumps there should he not make it in time.
	TestCreature trask;
	FunctionContext run("ActionForceMoveToObject");
	run.setSignature(Signature { kTypeVoid, kTypeObject, kTypeInt, kTypeFloat, kTypeFloat });
	run.setCaller(&trask);
	run.getParams()[0] = static_cast<Aurora::NWScript::Object *>(&waypoint);
	run.getParams()[1] = static_cast<int32_t>(1);
	run.getParams()[2] = 1.0f;
	run.getParams()[3] = 30.0f;
	f.functions.actionForceMoveToObject(run);

	action = trask.getCurrentAction();
	ASSERT_NE(action, nullptr);
	EXPECT_EQ(action->choreographyFlags, kMoveRun);
	EXPECT_FLOAT_EQ(action->timeout, 30.0f);
}

GTEST_TEST(EndarSpireEffects, fakeCastIsQueuedAndHarmless) {
	// k_pend_cut23 (case 25): ActionCastFakeSpellAtObject(FORCE_POWER_CHOKE, oTrask).
	Fixture f;
	TestCreature bandon, trask;
	setHP(trask, 50);

	FunctionContext ctx("ActionCastFakeSpellAtObject");
	ctx.setSignature(Signature { kTypeVoid, kTypeInt, kTypeObject, kTypeInt });
	ctx.setCaller(&bandon);
	ctx.getParams()[0] = static_cast<int32_t>(kForcePowerChoke);
	ctx.getParams()[1] = static_cast<Aurora::NWScript::Object *>(&trask);
	ctx.getParams()[2] = static_cast<int32_t>(0);
	f.functions.actionCastFakeSpellAtObject(ctx);

	const Action *action = bandon.getCurrentAction();
	ASSERT_NE(action, nullptr);
	EXPECT_EQ(action->type, kActionCastSpell);
	EXPECT_EQ(action->actionID, kForcePowerChoke);
	EXPECT_EQ(action->object, static_cast<Object *>(&trask));
	EXPECT_TRUE(action->fake);
	EXPECT_EQ(trask.getCurrentHitPoints(), 50);
}

GTEST_TEST(EndarSpireEffects, setLightsaberPoweredOverride) {
	Fixture f;
	TestCreature bandon;

	auto call = [&](int bOverride, int bPowered) {
		FunctionContext ctx("SetLightsaberPowered");
		ctx.setSignature(Signature { kTypeVoid, kTypeObject, kTypeInt, kTypeInt, kTypeInt });
		ctx.getParams()[0] = static_cast<Aurora::NWScript::Object *>(&bandon);
		ctx.getParams()[1] = static_cast<int32_t>(bOverride);
		ctx.getParams()[2] = static_cast<int32_t>(bPowered);
		ctx.getParams()[3] = static_cast<int32_t>(0);
		f.functions.setLightsaberPowered(ctx);
	};

	call(1, 0);
	EXPECT_FALSE(bandon.isLightsaberPowered());

	// k_pend_cut23: SetLightsaberPowered(OBJECT_SELF, TRUE, TRUE, FALSE)
	call(1, 1);
	EXPECT_TRUE(bandon.isLightsaberPowered());

	call(1, 0);
	call(0, 0); // Back to the game's say.
	EXPECT_TRUE(bandon.isLightsaberPowered());
}

// DLG participant animations from 10000 have their own numbering, not the
// script constants' nor animations.2da's (as KotOR.js maps them).
GTEST_TEST(EndarSpireEffects, dialogAnimationNumbersMapToTheirRows) {
	EXPECT_EQ(Engines::KotORBase::getDialogAnimationRow(10038), 25); // Talk normal
	EXPECT_EQ(Engines::KotORBase::getDialogAnimationRow(10030), 18);
	EXPECT_EQ(Engines::KotORBase::getDialogAnimationRow(10508), 566);
	EXPECT_EQ(Engines::KotORBase::getDialogAnimationRow(10001), -1);
	EXPECT_EQ(Engines::KotORBase::getDialogAnimationRow(38), -1);
}
