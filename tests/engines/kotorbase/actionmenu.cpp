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
 *  Unit tests for the HUD's target and self action menus.
 */

#include <memory>

#include "gtest/gtest.h"

#include "src/common/memreadstream.h"

#include "src/aurora/2dafile.h"

#include "src/engines/kotorbase/actionmenu.h"
#include "src/engines/kotorbase/creatureinfo.h"
#include "src/engines/kotorbase/types.h"

using namespace Engines::KotORBase;

static std::unique_ptr<Aurora::TwoDAFile> makeTwoDA(const char *text) {
	Common::MemoryReadStream stream(text);
	return std::make_unique<Aurora::TwoDAFile>(stream);
}

static ActionMenuTalents makeTalents() {
	ActionMenuTalents talents;

	ActionMenuFeatInfo flurry;
	flurry.category = kFeatCategoryMeleeCombat;
	flurry.successor = 91;
	flurry.icon = "ip_flurry";
	flurry.name = "Flurry";
	talents.setFeat(11, flurry);

	ActionMenuFeatInfo improvedFlurry;
	improvedFlurry.category = kFeatCategoryMeleeCombat;
	improvedFlurry.successor = 53;
	improvedFlurry.icon = "ip_flurry2";
	improvedFlurry.name = "Improved Flurry";
	talents.setFeat(91, improvedFlurry);

	ActionMenuFeatInfo powerAttack;
	powerAttack.category = kFeatCategoryMeleeCombat;
	powerAttack.name = "Power Attack";
	talents.setFeat(28, powerAttack);

	ActionMenuFeatInfo rapidShot;
	rapidShot.category = kFeatCategoryRangedCombat;
	rapidShot.name = "Rapid Shot";
	talents.setFeat(30, rapidShot);

	ActionMenuFeatInfo toughness; // Passive: never on the menu.
	talents.setFeat(84, toughness);

	ActionMenuPowerInfo stun;
	stun.hostile = true;
	stun.icon = "ip_stun";
	stun.name = "Stun";
	talents.setPower(5, stun);

	ActionMenuPowerInfo heal;
	heal.friendly = true;
	heal.name = "Heal";
	talents.setPower(1, heal);

	ActionMenuPowerInfo both;
	both.hostile = true;
	both.friendly = true;
	both.name = "Both";
	talents.setPower(9, both);

	return talents;
}

static CreatureInfo makeLeader() {
	CreatureInfo info;
	info.addFeat(11); // Flurry
	info.addFeat(91); // Improved Flurry (replaces Flurry on the menu)
	info.addFeat(28); // Power Attack
	info.addFeat(30); // Rapid Shot
	info.addFeat(84); // Toughness
	info.addForcePower(5);
	info.addForcePower(1);
	info.addForcePower(9);
	info.addForcePower(77); // Unknown power: not offered.
	return info;
}

static std::vector<int> optionIDs(const ActionMenuPanel &panel) {
	std::vector<int> ids;
	for (const ActionMenuOption &option : panel.getOptions())
		ids.push_back(option.id);
	return ids;
}

GTEST_TEST(KotORBaseActionMenu, panelCyclesAndWraps) {
	ActionMenuPanel panel;
	EXPECT_EQ(panel.getSelected(), nullptr);
	panel.next();
	panel.previous();
	EXPECT_EQ(panel.getSelectedIndex(), 0u);

	for (int i = 0; i < 3; ++i) {
		ActionMenuOption option;
		option.type = kActionMenuPower;
		option.id = i;
		panel.add(option);
	}

	EXPECT_EQ(panel.getSelected()->id, 0);
	panel.previous();
	EXPECT_EQ(panel.getSelected()->id, 2);
	panel.next();
	panel.next();
	EXPECT_EQ(panel.getSelected()->id, 1);
}

GTEST_TEST(KotORBaseActionMenu, panelReplaceKeepsSelection) {
	ActionMenuPanel panel, rebuilt;
	for (int i = 0; i < 3; ++i) {
		ActionMenuOption option;
		option.type = kActionMenuPower;
		option.id = i;
		panel.add(option);
		if (i != 0)
			rebuilt.add(option);
	}

	panel.next(); // Select power 1.
	panel.replace(rebuilt);
	EXPECT_EQ(panel.getSelected()->id, 1);
	EXPECT_EQ(panel.getSelectedIndex(), 0u);

	ActionMenuPanel gone;
	ActionMenuOption other;
	other.type = kActionMenuAttack;
	gone.add(other);
	panel.replace(gone);
	EXPECT_EQ(panel.getSelected()->type, kActionMenuAttack);
}

GTEST_TEST(KotORBaseActionMenu, usableCombatFeatsDropSupersededFeats) {
	const ActionMenuTalents talents = makeTalents();
	const CreatureInfo leader = makeLeader();

	const std::vector<uint32_t> melee = getUsableCombatFeats(leader, kFeatCategoryMeleeCombat, talents);
	ASSERT_EQ(melee.size(), 2u);
	EXPECT_EQ(melee[0], 91u);
	EXPECT_EQ(melee[1], 28u);

	const std::vector<uint32_t> ranged = getUsableCombatFeats(leader, kFeatCategoryRangedCombat, talents);
	ASSERT_EQ(ranged.size(), 1u);
	EXPECT_EQ(ranged[0], 30u);
}

GTEST_TEST(KotORBaseActionMenu, hostileTargetGetsAttacksFeatsAndHostilePowers) {
	const ActionMenuTalents talents = makeTalents();
	const CreatureInfo leader = makeLeader();

	ActionMenuInput input;
	input.leader = &leader;
	input.target = kActionMenuTargetHostile;

	ActionMenuItemEntry grenade;
	grenade.tag = "g_w_fraggren01";
	grenade.kind = ActionMenuItemEntry::kKindGrenade;
	ActionMenuItemEntry medpac;
	medpac.tag = "g_i_medeqpmnt01";
	medpac.kind = ActionMenuItemEntry::kKindMedical;
	input.items = { grenade, medpac };

	ActionMenuTargetPanels panels = buildTargetActionMenu(input, talents);

	const ActionMenuPanel &attack = panels[kTargetPanelAttack];
	ASSERT_EQ(attack.size(), 3u);
	EXPECT_EQ(attack.getOptions()[0].type, kActionMenuAttack);
	EXPECT_EQ(attack.getOptions()[0].icon, "i_attack");
	EXPECT_EQ(attack.getOptions()[1].type, kActionMenuFeat);
	EXPECT_EQ(attack.getOptions()[1].id, 91);
	EXPECT_EQ(attack.getOptions()[1].icon, "ip_flurry2");
	EXPECT_EQ(attack.getOptions()[2].id, 28);

	EXPECT_EQ(optionIDs(panels[kTargetPanelPower]), (std::vector<int>{ 5, 9 }));
	EXPECT_EQ(panels[kTargetPanelPower].getOptions()[0].type, kActionMenuPower);

	ASSERT_EQ(panels[kTargetPanelItem].size(), 1u);
	EXPECT_EQ(panels[kTargetPanelItem].getOptions()[0].itemTag, "g_w_fraggren01");

	// A ranged weapon swaps the melee feats for the ranged ones.
	input.rangedWeapon = true;
	panels = buildTargetActionMenu(input, talents);
	EXPECT_EQ(optionIDs(panels[kTargetPanelAttack]), (std::vector<int>{ -1, 30 }));
}

GTEST_TEST(KotORBaseActionMenu, minesAreSetFromTheSelfMenu) {
	const ActionMenuTalents talents = makeTalents();
	const CreatureInfo leader = makeLeader();

	ActionMenuInput input;
	input.leader = &leader;

	ActionMenuItemEntry mine;
	mine.tag  = "g_i_trapkit001";
	mine.name = "Minor Frag Mine";
	mine.kind = ActionMenuItemEntry::kKindMine;
	ActionMenuItemEntry medpac;
	medpac.tag = "g_i_medeqpmnt01";
	medpac.kind = ActionMenuItemEntry::kKindMedical;
	input.items = { mine, medpac };

	const ActionMenuSelfPanels panels = buildSelfActionMenu(input, talents);
	ASSERT_EQ(panels[kSelfPanelMine].size(), 1u);
	EXPECT_EQ(panels[kSelfPanelMine].getOptions()[0].type, kActionMenuSetMine);
	EXPECT_EQ(panels[kSelfPanelMine].getOptions()[0].itemTag, "g_i_trapkit001");
	EXPECT_EQ(panels[kSelfPanelMine].getOptions()[0].name, "Minor Frag Mine");
	EXPECT_EQ(panels[kSelfPanelMedical].size(), 1u);

	// Not thrown at a target like a grenade.
	input.target = kActionMenuTargetHostile;
	EXPECT_TRUE(buildTargetActionMenu(input, talents)[kTargetPanelItem].empty());
}

GTEST_TEST(KotORBaseActionMenu, noHostileActionsOnNonHostileTargets) {
	const ActionMenuTalents talents = makeTalents();
	const CreatureInfo leader = makeLeader();

	ActionMenuItemEntry grenade;
	grenade.tag = "g_w_fraggren01";
	grenade.kind = ActionMenuItemEntry::kKindGrenade;

	ActionMenuInput input;
	input.leader = &leader;
	input.items = { grenade };

	for (ActionMenuTarget target : { kActionMenuTargetNone, kActionMenuTargetFriendly, kActionMenuTargetObject }) {
		input.target = target;
		const ActionMenuTargetPanels panels = buildTargetActionMenu(input, talents);
		for (const ActionMenuPanel &panel : panels)
			EXPECT_TRUE(panel.empty()) << "target kind " << target;
	}
}

GTEST_TEST(KotORBaseActionMenu, lockedObjectOffersSecurity) {
	const ActionMenuTalents talents = makeTalents();
	const CreatureInfo leader = makeLeader();

	ActionMenuInput input;
	input.leader = &leader;
	input.target = kActionMenuTargetObject;
	input.targetCanBeUnlocked = true;

	const ActionMenuTargetPanels panels = buildTargetActionMenu(input, talents);
	EXPECT_TRUE(panels[kTargetPanelAttack].empty());
	ASSERT_EQ(panels[kTargetPanelPower].size(), 1u);
	EXPECT_EQ(panels[kTargetPanelPower].getOptions()[0].type, kActionMenuOpenLock);
	EXPECT_EQ(panels[kTargetPanelPower].getOptions()[0].icon, "isk_security");
	EXPECT_TRUE(panels[kTargetPanelItem].empty());
}

GTEST_TEST(KotORBaseActionMenu, selfMenuHasFriendlyPowersAndItems) {
	const ActionMenuTalents talents = makeTalents();
	const CreatureInfo leader = makeLeader();

	ActionMenuInput input;
	input.leader = &leader;

	ActionMenuItemEntry medpac, stim, grenade, junk;
	medpac.tag = "medpac";
	medpac.kind = ActionMenuItemEntry::kKindMedical;
	stim.tag = "stim";
	stim.kind = ActionMenuItemEntry::kKindOther;
	grenade.tag = "grenade";
	grenade.kind = ActionMenuItemEntry::kKindGrenade;
	junk.tag = "junk";
	input.items = { medpac, stim, grenade, junk };

	const ActionMenuSelfPanels panels = buildSelfActionMenu(input, talents);
	EXPECT_EQ(optionIDs(panels[kSelfPanelPower]), (std::vector<int>{ 1, 9 }));
	ASSERT_EQ(panels[kSelfPanelMedical].size(), 1u);
	EXPECT_EQ(panels[kSelfPanelMedical].getOptions()[0].itemTag, "medpac");
	EXPECT_EQ(panels[kSelfPanelMedical].getOptions()[0].name, "medpac");
	ASSERT_EQ(panels[kSelfPanelOther].size(), 1u);
	EXPECT_EQ(panels[kSelfPanelOther].getOptions()[0].itemTag, "stim");
	EXPECT_TRUE(panels[kSelfPanelMine].empty());
}

GTEST_TEST(KotORBaseActionMenu, loadFeatsReadsHexCategoriesAndSuccessors) {
	std::unique_ptr<Aurora::TwoDAFile> feat = makeTwoDA(
		"2DA V2.0\n"
		"\n"
		"    label            name  icon        category  successor\n"
		"0   Toughness        ****  ip_tough    ****      ****\n"
		"1   Flurry           ****  ip_flurry   0x1104    2\n"
		"2   ImprovedFlurry   ****  ip_flurry2  0x1104    ****\n"
		"3   RapidShot        ****  ip_rapid    0x1111    ****\n");

	ActionMenuTalents talents;
	talents.loadFeats(*feat);

	ASSERT_NE(talents.getFeat(0), nullptr);
	EXPECT_EQ(talents.getFeat(0)->category, -1);
	EXPECT_EQ(talents.getFeat(1)->category, kFeatCategoryMeleeCombat);
	EXPECT_EQ(talents.getFeat(1)->successor, 2);
	EXPECT_EQ(talents.getFeat(1)->icon, "ip_flurry");
	EXPECT_EQ(talents.getFeat(1)->name, "Flurry");
	EXPECT_EQ(talents.getFeat(2)->successor, -1);
	EXPECT_EQ(talents.getFeat(3)->category, kFeatCategoryRangedCombat);
	EXPECT_EQ(talents.getFeat(4), nullptr);
}

GTEST_TEST(KotORBaseActionMenu, loadPowersReadsForceHostileAndFriendly) {
	std::unique_ptr<Aurora::TwoDAFile> spells = makeTwoDA(
		"2DA V2.0\n"
		"\n"
		"    label        name  iconresref  forcehostile  forcefriendly\n"
		"0   Heal         ****  ip_heal     ****          1\n"
		"1   Stun         ****  ip_stun     1             ****\n"
		"2   Odd          ****  ****        0             0\n"
		"3   ItemOnly     ****  ****        ****          ****\n");

	ActionMenuTalents talents;
	talents.loadPowers(*spells);

	EXPECT_FALSE(talents.getPower(0)->hostile);
	EXPECT_TRUE(talents.getPower(0)->friendly);
	EXPECT_EQ(talents.getPower(0)->icon, "ip_heal");
	EXPECT_TRUE(talents.getPower(1)->hostile);
	EXPECT_FALSE(talents.getPower(1)->friendly);
	EXPECT_TRUE(talents.getPower(2)->hostile);
	EXPECT_TRUE(talents.getPower(2)->friendly);
	EXPECT_FALSE(talents.getPower(3)->hostile);
	EXPECT_FALSE(talents.getPower(3)->friendly);

	// Without the forcehostile/forcefriendly columns, "hostile" decides.
	std::unique_ptr<Aurora::TwoDAFile> old = makeTwoDA(
		"2DA V2.0\n"
		"\n"
		"    label  hostile\n"
		"0   Heal   0\n"
		"1   Stun   1\n");

	ActionMenuTalents fallback;
	fallback.loadPowers(*old);
	EXPECT_TRUE(fallback.getPower(0)->friendly);
	EXPECT_FALSE(fallback.getPower(0)->hostile);
	EXPECT_TRUE(fallback.getPower(1)->hostile);
	EXPECT_FALSE(fallback.getPower(1)->friendly);
}

GTEST_TEST(KotORBaseActionMenu, defaultsWithoutGameData) {
	ActionMenuTalents talents;
	talents.loadDefaults();

	CreatureInfo leader;
	leader.addFeat(kFeatFlurry);
	leader.addFeat(kFeatImprovedFlurry);
	leader.addFeat(kFeatRapidShot);
	leader.addForcePower(5); // Stun
	leader.addForcePower(1); // Heal

	EXPECT_EQ(getUsableCombatFeats(leader, kFeatCategoryMeleeCombat, talents),
	          (std::vector<uint32_t>{ kFeatImprovedFlurry }));
	EXPECT_EQ(getUsableCombatFeats(leader, kFeatCategoryRangedCombat, talents),
	          (std::vector<uint32_t>{ kFeatRapidShot }));

	ActionMenuInput input;
	input.leader = &leader;
	input.target = kActionMenuTargetHostile;

	EXPECT_EQ(optionIDs(buildTargetActionMenu(input, talents)[kTargetPanelPower]), (std::vector<int>{ 5 }));
	EXPECT_EQ(optionIDs(buildSelfActionMenu(input, talents)[kSelfPanelPower]), (std::vector<int>{ 1 }));
}

GTEST_TEST(KotORBaseActionMenu, detectedMineOffersDisarmAndRecover) {
	const ActionMenuTalents talents = makeTalents();
	const CreatureInfo leader = makeLeader();

	ActionMenuInput input;
	input.leader = &leader;
	input.target = kActionMenuTargetObject;
	input.targetIsMine = true;

	const ActionMenuTargetPanels panels = buildTargetActionMenu(input, talents);
	EXPECT_TRUE(panels[kTargetPanelAttack].empty());
	ASSERT_EQ(panels[kTargetPanelPower].size(), 2u);
	EXPECT_EQ(panels[kTargetPanelPower].getOptions()[0].type, kActionMenuDisarmMine);
	EXPECT_EQ(panels[kTargetPanelPower].getOptions()[1].type, kActionMenuRecoverMine);
}

GTEST_TEST(KotORBaseActionMenu, lockedObjectOffersBashAndSecurity) {
	const ActionMenuTalents talents = makeTalents();
	const CreatureInfo leader = makeLeader();

	ActionMenuInput input;
	input.leader = &leader;
	input.target = kActionMenuTargetObject;
	input.targetCanBeUnlocked = true;
	input.targetCanBeBashed = true;

	const ActionMenuTargetPanels panels = buildTargetActionMenu(input, talents);
	ASSERT_EQ(panels[kTargetPanelAttack].size(), 1u);
	EXPECT_EQ(panels[kTargetPanelAttack].getOptions()[0].type, kActionMenuBash);
	EXPECT_EQ(panels[kTargetPanelAttack].getOptions()[0].icon, "i_attack");
	ASSERT_EQ(panels[kTargetPanelPower].size(), 1u);
	EXPECT_EQ(panels[kTargetPanelPower].getOptions()[0].type, kActionMenuOpenLock);

	// Plot, NotBlastable or already broken: Security only (and no mine on the ground).
	EXPECT_TRUE(buildSelfActionMenu(input, talents)[kSelfPanelMine].empty());
	input.targetCanBeBashed = false;
	EXPECT_TRUE(buildTargetActionMenu(input, talents)[kTargetPanelAttack].empty());
}

GTEST_TEST(KotORBaseActionMenu, equippedItemPowersAreSelfOptions) {
	const ActionMenuTalents talents = makeTalents();
	const CreatureInfo leader = makeLeader();

	ActionMenuInput input;
	input.leader = &leader;
	input.itemPowers = { { 1, "g_i_frarmbnds01" }, { 5, "g_i_hostile01" }, { 42, "g_i_unknown01" } };

	const ActionMenuSelfPanels panels = buildSelfActionMenu(input, talents);

	// The leader's friendly powers, then the item powers that aren't purely hostile.
	const std::vector<ActionMenuOption> &options = panels[kSelfPanelPower].getOptions();
	ASSERT_EQ(options.size(), 4u);
	EXPECT_EQ(options[2].type, kActionMenuItemPower);
	EXPECT_EQ(options[2].id, 1);
	EXPECT_EQ(options[2].itemTag, "g_i_frarmbnds01");
	EXPECT_EQ(options[3].id, 42);
	EXPECT_EQ(options[3].name, "g_i_unknown01"); // No spells.2da name: the item tag.
}
