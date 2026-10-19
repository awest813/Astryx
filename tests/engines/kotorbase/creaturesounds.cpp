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
 *  Unit tests for creature sounds (footsteps, weapons) and visual effect tables.
 */

#include <memory>
#include <set>

#include "gtest/gtest.h"

#include "src/common/ustring.h"
#include "src/common/memreadstream.h"

#include "src/aurora/2dafile.h"

#include "src/engines/kotorbase/creaturesounds.h"
#include "src/engines/kotorbase/visualeffects.h"

using namespace Engines::KotORBase;

static Aurora::TwoDAFile *makeTwoDA(const char *text) {
	Common::MemoryReadStream stream(text);
	return new Aurora::TwoDAFile(stream);
}

static const char *kFootsteps =
	"2DA V2.0\n"
	"\n"
	"   label   rolling     dirt0   dirt1   dirt2   stone0  stone1  stone2  metal0  metal1  metal2  puddles0 grass0\n"
	"0  human   ****        fs_d0   fs_d1   fs_d2   fs_s0   fs_s1   ****    fs_m0   fs_m1   fs_m2   fs_p0    ****\n"
	"1  droid   fs_roll     ****    ****    ****    ****    ****    ****    ****    ****    ****    ****     ****\n";

GTEST_TEST(KotORCreatureSounds, footstepSurfaceColumns) {
	EXPECT_STREQ(getFootstepSurfaceColumn(1),  "dirt");
	EXPECT_STREQ(getFootstepSurfaceColumn(3),  "grass");
	EXPECT_STREQ(getFootstepSurfaceColumn(4),  "stone");
	EXPECT_STREQ(getFootstepSurfaceColumn(5),  "wood");
	EXPECT_STREQ(getFootstepSurfaceColumn(6),  "water");
	EXPECT_STREQ(getFootstepSurfaceColumn(9),  "carpet");
	EXPECT_STREQ(getFootstepSurfaceColumn(10), "metal");
	EXPECT_STREQ(getFootstepSurfaceColumn(11), "puddles");
	EXPECT_STREQ(getFootstepSurfaceColumn(13), "puddles");
	EXPECT_STREQ(getFootstepSurfaceColumn(14), "leaves");

	// Undefined and unknown surfaces (or none found) sound like dirt
	EXPECT_STREQ(getFootstepSurfaceColumn(0),          "dirt");
	EXPECT_STREQ(getFootstepSurfaceColumn(0xFFFFFFFF), "dirt");
}

GTEST_TEST(KotORCreatureSounds, footstepBySurfaceAndVariant) {
	std::unique_ptr<Aurora::TwoDAFile> footsteps(makeTwoDA(kFootsteps));

	EXPECT_STREQ(getFootstepSound(*footsteps, 0, 1, 0).c_str(), "fs_d0");
	EXPECT_STREQ(getFootstepSound(*footsteps, 0, 1, 2).c_str(), "fs_d2");
	EXPECT_STREQ(getFootstepSound(*footsteps, 0, 4, 1).c_str(), "fs_s1");
	EXPECT_STREQ(getFootstepSound(*footsteps, 0, 10, 2).c_str(), "fs_m2");
	EXPECT_STREQ(getFootstepSound(*footsteps, 0, 13, 0).c_str(), "fs_p0");

	// An empty variant falls back to the surface's first sound
	EXPECT_STREQ(getFootstepSound(*footsteps, 0, 4, 2).c_str(), "fs_s0");
	EXPECT_STREQ(getFootstepSound(*footsteps, 0, 4, 7).c_str(), "fs_s0");
}

GTEST_TEST(KotORCreatureSounds, footstepFallsBackToRolling) {
	std::unique_ptr<Aurora::TwoDAFile> footsteps(makeTwoDA(kFootsteps));

	// Droids that roll have no steps, just the rolling sound
	EXPECT_STREQ(getFootstepSound(*footsteps, 1, 4, 0).c_str(), "fs_roll");

	// No grass sound and nothing rolling: silence
	EXPECT_TRUE(getFootstepSound(*footsteps, 0, 3, 0).empty());

	// No such footstep type
	EXPECT_TRUE(getFootstepSound(*footsteps, -1, 1, 0).empty());
	EXPECT_TRUE(getFootstepSound(*footsteps,  5, 1, 0).empty());
}

GTEST_TEST(KotORCreatureSounds, animationEventNames) {
	EXPECT_EQ(getAnimationEventSound("snd_footstep"), kAnimationEventSoundFootstep);
	EXPECT_EQ(getAnimationEventSound("Swingshort"),   kAnimationEventSoundSwingShort);
	EXPECT_EQ(getAnimationEventSound("swinglong"),    kAnimationEventSoundSwingLong);
	EXPECT_EQ(getAnimationEventSound("SwingTwirl"),   kAnimationEventSoundSwingTwirl);
	EXPECT_EQ(getAnimationEventSound("HitParry"),     kAnimationEventSoundParry);
	EXPECT_EQ(getAnimationEventSound("Clash"),        kAnimationEventSoundClash);
	EXPECT_EQ(getAnimationEventSound("Contact"),      kAnimationEventSoundClash);
	EXPECT_EQ(getAnimationEventSound("Hit"),          kAnimationEventSoundHit);

	EXPECT_EQ(getAnimationEventSound("blur_start"),   kAnimationEventSoundNone);
	EXPECT_EQ(getAnimationEventSound("detonate"),     kAnimationEventSoundNone);
	EXPECT_EQ(getAnimationEventSound(""),             kAnimationEventSoundNone);
}

static const char *kBaseItems =
	"2DA V2.0\n"
	"\n"
	"   label          weaponmattype  powereditem\n"
	"0  sword          1              0\n"
	"1  lightsaber     2              1\n"
	"2  armband        ****           ****\n"
	"3  oldformat      ****           1\n";

static const char *kWeaponSounds =
	"2DA V2.0\n"
	"\n"
	"   label   swingshort0 swingshort1 swingshort2 swinglong0 parry0 parry1 clash0 leather0 leather1 metal0 armor0\n"
	"0  unarmed ****        ****        ****        ****       ****   ****   ****   pnch0    pnch1    ****   ****\n"
	"1  blade   sw_s0       sw_s1       ****        sw_l0      pr0    pr1    cl0    bl_h0    bl_h1    bl_m0  ****\n"
	"2  saber   ls_s0       ls_s1       ls_s2       ls_l0      lp0    ****   lc0    ls_h0    ****     ls_m0  ls_a0\n";

GTEST_TEST(KotORCreatureSounds, weaponSoundRow) {
	std::unique_ptr<Aurora::TwoDAFile> baseItems(makeTwoDA(kBaseItems));

	EXPECT_EQ(getWeaponSoundRow(*baseItems, 0), 1);
	EXPECT_EQ(getWeaponSoundRow(*baseItems, 1), 2);
	EXPECT_EQ(getWeaponSoundRow(*baseItems, 2), -1);
	EXPECT_EQ(getWeaponSoundRow(*baseItems, 3), 1);
	EXPECT_EQ(getWeaponSoundRow(*baseItems, 9), -1);
	EXPECT_EQ(getWeaponSoundRow(*baseItems, -1), -1);
}

GTEST_TEST(KotORCreatureSounds, weaponSwingsAndParries) {
	std::unique_ptr<Aurora::TwoDAFile> sounds(makeTwoDA(kWeaponSounds));

	EXPECT_STREQ(getWeaponSound(*sounds, 2, kAnimationEventSoundSwingShort, kWeaponHitLeather, 2).c_str(), "ls_s2");
	EXPECT_STREQ(getWeaponSound(*sounds, 1, kAnimationEventSoundSwingShort, kWeaponHitLeather, 1).c_str(), "sw_s1");
	// Missing variant: the first one
	EXPECT_STREQ(getWeaponSound(*sounds, 1, kAnimationEventSoundSwingShort, kWeaponHitLeather, 2).c_str(), "sw_s0");
	EXPECT_STREQ(getWeaponSound(*sounds, 1, kAnimationEventSoundSwingLong , kWeaponHitLeather, 0).c_str(), "sw_l0");
	EXPECT_STREQ(getWeaponSound(*sounds, 1, kAnimationEventSoundParry     , kWeaponHitLeather, 1).c_str(), "pr1");
	EXPECT_STREQ(getWeaponSound(*sounds, 2, kAnimationEventSoundParry     , kWeaponHitLeather, 1).c_str(), "lp0");
	EXPECT_STREQ(getWeaponSound(*sounds, 2, kAnimationEventSoundClash     , kWeaponHitLeather, 0).c_str(), "lc0");

	EXPECT_EQ(getWeaponSoundVariantCount(kAnimationEventSoundSwingShort), 3);
	EXPECT_EQ(getWeaponSoundVariantCount(kAnimationEventSoundHit), 2);
	EXPECT_EQ(getWeaponSoundVariantCount(kAnimationEventSoundFootstep), 0);
}

GTEST_TEST(KotORCreatureSounds, weaponHitsByMaterial) {
	std::unique_ptr<Aurora::TwoDAFile> sounds(makeTwoDA(kWeaponSounds));

	EXPECT_STREQ(getWeaponSound(*sounds, 1, kAnimationEventSoundHit, kWeaponHitLeather, 1).c_str(), "bl_h1");
	EXPECT_STREQ(getWeaponSound(*sounds, 1, kAnimationEventSoundHit, kWeaponHitMetal  , 0).c_str(), "bl_m0");
	EXPECT_STREQ(getWeaponSound(*sounds, 2, kAnimationEventSoundHit, kWeaponHitArmor  , 0).c_str(), "ls_a0");
	EXPECT_STREQ(getWeaponSound(*sounds, 0, kAnimationEventSoundHit, kWeaponHitLeather, 0).c_str(), "pnch0");

	// No armour sound for this weapon
	EXPECT_TRUE(getWeaponSound(*sounds, 1, kAnimationEventSoundHit, kWeaponHitArmor, 0).empty());
	// Not a weapon sound, no such row
	EXPECT_TRUE(getWeaponSound(*sounds, 1, kAnimationEventSoundFootstep, kWeaponHitLeather, 0).empty());
	EXPECT_TRUE(getWeaponSound(*sounds, 7, kAnimationEventSoundHit, kWeaponHitLeather, 0).empty());
}

static const char *kVisualEffects =
	"2DA V2.0\n"
	"\n"
	"   label        type_fd imp_headcon_node imp_impact_node imp_root_s_node imp_root_m_node imp_root_l_node imp_root_h_node soundimpact\n"
	"0  imp_heal     F       ****             v_heal_imp      ****            v_heal_rt_m     v_heal_rt_l     ****            v_heal\n"
	"1  dur_shield   D       v_shld_head      v_shld          v_shld_rt_s     v_shld_rt_m     ****            v_shld_rt_h     ****\n"
	"2  beam_light   B       ****             ****            ****            ****            ****            ****            ****\n"
	"3  nothing      ****    ****             ****            ****            ****            ****            ****            ****\n";

GTEST_TEST(KotORVisualEffects, loadRow) {
	std::unique_ptr<Aurora::TwoDAFile> vfx(makeTwoDA(kVisualEffects));

	VisualEffectInfo heal;
	ASSERT_TRUE(loadVisualEffect(*vfx, 0, heal));
	EXPECT_EQ(heal.id, 0);
	EXPECT_EQ(heal.type, 'F');
	EXPECT_FALSE(heal.isDuration());
	EXPECT_STREQ(heal.impactModel.c_str(), "v_heal_imp");
	EXPECT_TRUE(heal.headModel.empty());
	EXPECT_STREQ(heal.impactSound.c_str(), "v_heal");

	VisualEffectInfo shield;
	ASSERT_TRUE(loadVisualEffect(*vfx, 1, shield));
	EXPECT_TRUE(shield.isDuration());
	EXPECT_STREQ(shield.headModel.c_str(), "v_shld_head");
	EXPECT_TRUE(shield.impactSound.empty());

	VisualEffectInfo beam;
	ASSERT_TRUE(loadVisualEffect(*vfx, 2, beam));
	EXPECT_TRUE(beam.isBeam());

	// No type given: fire and forget
	VisualEffectInfo nothing;
	ASSERT_TRUE(loadVisualEffect(*vfx, 3, nothing));
	EXPECT_EQ(nothing.type, 'F');

	VisualEffectInfo none;
	EXPECT_FALSE(loadVisualEffect(*vfx, 4, none));
	EXPECT_FALSE(loadVisualEffect(*vfx, -1, none));
}

GTEST_TEST(KotORVisualEffects, rootModelBySize) {
	std::unique_ptr<Aurora::TwoDAFile> vfx(makeTwoDA(kVisualEffects));

	VisualEffectInfo shield;
	ASSERT_TRUE(loadVisualEffect(*vfx, 1, shield));

	EXPECT_STREQ(getVisualEffectRootModel(shield, 1).c_str(), "v_shld_rt_s"); // Tiny
	EXPECT_STREQ(getVisualEffectRootModel(shield, 2).c_str(), "v_shld_rt_s"); // Small
	EXPECT_STREQ(getVisualEffectRootModel(shield, 3).c_str(), "v_shld_rt_m"); // Medium
	EXPECT_TRUE (getVisualEffectRootModel(shield, 4).empty());                // Large: none
	EXPECT_STREQ(getVisualEffectRootModel(shield, 5).c_str(), "v_shld_rt_h"); // Huge
	EXPECT_STREQ(getVisualEffectRootModel(shield, 0).c_str(), "v_shld_rt_m"); // Unknown
}

GTEST_TEST(KotORVisualEffects, hookNodeChoice) {
	const std::set<Common::UString> humanoid = { "impact", "headconjure", "handconjure", "lhand", "rhand", "headhook" };
	const std::set<Common::UString> beast    = { "impact", "headhook" };
	const std::set<Common::UString> droid    = { "rhand" };
	const std::set<Common::UString> bare;

	auto has = [](const std::set<Common::UString> &nodes) {
		return [&nodes](const Common::UString &node) { return nodes.count(node) > 0; };
	};

	EXPECT_STREQ(chooseVisualEffectNode(kVisualEffectHookImpact, has(humanoid)).c_str(), "impact");
	EXPECT_STREQ(chooseVisualEffectNode(kVisualEffectHookHead  , has(humanoid)).c_str(), "headconjure");
	EXPECT_STREQ(chooseVisualEffectNode(kVisualEffectHookHand  , has(humanoid)).c_str(), "handconjure");
	EXPECT_TRUE (chooseVisualEffectNode(kVisualEffectHookRoot  , has(humanoid)).empty());

	// Fallbacks: the head hook, the impact node, then the model's root
	EXPECT_STREQ(chooseVisualEffectNode(kVisualEffectHookHead, has(beast)).c_str(), "headhook");
	EXPECT_STREQ(chooseVisualEffectNode(kVisualEffectHookHand, has(beast)).c_str(), "impact");
	EXPECT_STREQ(chooseVisualEffectNode(kVisualEffectHookHand, has(droid)).c_str(), "rhand");
	EXPECT_TRUE (chooseVisualEffectNode(kVisualEffectHookImpact, has(droid)).empty());
	EXPECT_TRUE (chooseVisualEffectNode(kVisualEffectHookHead  , has(bare)).empty());
}

GTEST_TEST(KotORVisualEffects, spellCastVisuals) {
	std::unique_ptr<Aurora::TwoDAFile> spells(makeTwoDA(
		"2DA V2.0\n"
		"\n"
		"   label       conjtime conjheadvfx conjhandvfx conjgrndvfx conjsoundvfx conjsoundmale conjsoundfemale casthandvfx castsound\n"
		"0  heal        1000     ****        v_conj_hnd  ****        v_conj       m_grunt       f_grunt         ****        v_cast\n"
		"1  plain       ****     ****        ****        ****        ****         ****          ****            ****        ****\n"
		"2  numeric     500      ****        ****        1003        ****         ****          ****            ****        ****\n"));

	SpellCastVisuals heal;
	ASSERT_TRUE(loadSpellCastVisuals(*spells, 0, heal));
	EXPECT_FALSE(heal.empty());
	EXPECT_STREQ(heal.conjureHand.c_str(), "v_conj_hnd");
	EXPECT_TRUE(heal.conjureHead.empty());
	EXPECT_STREQ(heal.conjureSound.c_str(), "v_conj");
	EXPECT_STREQ(heal.conjureSoundMale.c_str(), "m_grunt");
	EXPECT_STREQ(heal.conjureSoundFemale.c_str(), "f_grunt");
	EXPECT_STREQ(heal.castSound.c_str(), "v_cast");
	EXPECT_FLOAT_EQ(heal.conjureTime, 1.0f);

	SpellCastVisuals plain;
	ASSERT_TRUE(loadSpellCastVisuals(*spells, 1, plain));
	EXPECT_TRUE(plain.empty());
	EXPECT_FLOAT_EQ(plain.conjureTime, 0.0f);

	SpellCastVisuals numeric;
	ASSERT_TRUE(loadSpellCastVisuals(*spells, 2, numeric));
	EXPECT_STREQ(numeric.conjureGround.c_str(), "1003");

	EXPECT_FALSE(loadSpellCastVisuals(*spells, 3, plain));
}

GTEST_TEST(KotORVisualEffects, spellCastVisualsOtherSpelling) {
	std::unique_ptr<Aurora::TwoDAFile> spells(makeTwoDA(
		"2DA V2.0\n"
		"\n"
		"   label  conjhandvisual casthandvisual castheadvisual\n"
		"0  alt    v_alt_conj     v_alt_cast     ****\n"));

	SpellCastVisuals alt;
	ASSERT_TRUE(loadSpellCastVisuals(*spells, 0, alt));
	EXPECT_STREQ(alt.conjureHand.c_str(), "v_alt_conj");
	EXPECT_STREQ(alt.castHand.c_str(), "v_alt_cast");
	EXPECT_TRUE(alt.castHead.empty());
}
