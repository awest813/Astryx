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
 *  The HUD's target and self action menus (combat feats, Force powers,
 *  Security, usable items), as KotOR.js' ActionMenuManager builds them.
 */

#ifndef ENGINES_KOTORBASE_ACTIONMENU_H
#define ENGINES_KOTORBASE_ACTIONMENU_H

#include <array>
#include <map>
#include <utility>
#include <vector>
#include <cstdint>

#include "src/common/ustring.h"

namespace Aurora {
	class TwoDAFile;
}

namespace Engines {

namespace KotORBase {

class CreatureInfo;
class Item;

/** feat.2da "category" of the melee combat feats (Flurry, Power Attack, ...). */
static const int kFeatCategoryMeleeCombat  = 0x1104;
/** feat.2da "category" of the ranged combat feats (Rapid Shot, Sniper Shot, ...). */
static const int kFeatCategoryRangedCombat = 0x1111;

/** Number of option panels above the selected target (BTN_TARGET0-2). */
static const size_t kActionMenuTargetPanels = 3;
/** Number of option panels of the party leader's own actions (BTN_ACTION0-3). */
static const size_t kActionMenuSelfPanels   = 4;

/** Target panels, as KotOR.js fills them. */
enum ActionMenuTargetPanel {
	kTargetPanelAttack = 0, ///< Attack and the combat feats; bashing a locked object.
	kTargetPanelPower  = 1, ///< Hostile Force powers, Security on a locked object.
	kTargetPanelItem   = 2  ///< Grenades.
};

/** Self panels, matching the retail BTN_ACTION0-3 tooltips. */
enum ActionMenuSelfPanel {
	kSelfPanelPower   = 0, ///< Friendly Force powers.
	kSelfPanelMedical = 1, ///< Medical items (medpacs).
	kSelfPanelOther   = 2, ///< Other usable items (stims, shields, ...).
	kSelfPanelMine    = 3  ///< Mines to set ahead of the leader.
};

enum ActionMenuOptionType {
	kActionMenuAttack,   ///< Plain attack.
	kActionMenuFeat,     ///< Attack using a combat feat.
	kActionMenuPower,    ///< Cast a Force power.
	kActionMenuOpenLock, ///< Security: open the target's lock.
	kActionMenuItem,     ///< Use an inventory item (medpac, stim, grenade).
	kActionMenuItemPower,///< Cast the Force power of an equipped item (armband).
	kActionMenuDisarmMine, ///< Demolitions: disarm a detected mine.
	kActionMenuRecoverMine,///< Demolitions: recover a detected mine (harder; gives the mine).
	kActionMenuBash,       ///< Attack a locked door or container to break it open.
	kActionMenuSetMine     ///< Demolitions: lay a mine (itemTag) ahead of the leader.
};

struct ActionMenuOption {
	ActionMenuOptionType type { kActionMenuAttack };
	int id { -1 };             ///< Feat or power ID.
	Common::UString itemTag;   ///< The item to use (kActionMenuItem).
	Common::UString icon;      ///< Texture shown on the button.
	Common::UString name;      ///< Text shown in the description label.

	bool operator==(const ActionMenuOption &other) const;
};

/** One button's options, cycled with its up/down arrows. */
class ActionMenuPanel {
public:
	void clear();
	void add(const ActionMenuOption &option);

	bool empty() const;
	size_t size() const;
	const std::vector<ActionMenuOption> &getOptions() const;

	size_t getSelectedIndex() const;
	/** The selected option, or nullptr when the panel is empty. */
	const ActionMenuOption *getSelected() const;

	/** Select the previous option, wrapping around. */
	void previous();
	/** Select the next option, wrapping around. */
	void next();

	/** Replace the options, keeping the selected one selected if it's still offered. */
	void replace(const ActionMenuPanel &panel);

private:
	std::vector<ActionMenuOption> _options;
	size_t _selected { 0 };
};

/** What the menus need to know about a feat (feat.2da). */
struct ActionMenuFeatInfo {
	int category { -1 };     ///< feat.2da "category"; see kFeatCategory*.
	int successor { -1 };    ///< The feat that improves this one, -1 if none.
	Common::UString icon;
	Common::UString name;
};

/** What the menus need to know about a Force power (spells.2da). */
struct ActionMenuPowerInfo {
	bool hostile { false };  ///< Can be used on a hostile target ("forcehostile").
	bool friendly { false }; ///< Can be used on the caster ("forcefriendly").
	Common::UString icon;
	Common::UString name;
};

/** Feat and power information used to build the menus. */
class ActionMenuTalents {
public:
	ActionMenuTalents();

	/** Read the "category", "successor", "icon" and "name"/"label" columns. */
	void loadFeats(const Aurora::TwoDAFile &feat);
	/** Read "forcehostile"/"forcefriendly" (else "hostile"), "iconresref", "name"/"label". */
	void loadPowers(const Aurora::TwoDAFile &spells);

	/** Add the built-in combat feats and power hostility, for running without game data. */
	void loadDefaults();

	void setFeat(uint32_t feat, const ActionMenuFeatInfo &info);
	void setPower(uint32_t power, const ActionMenuPowerInfo &info);

	/** The feat's information, or nullptr when it isn't known. */
	const ActionMenuFeatInfo *getFeat(uint32_t feat) const;
	/** The power's information, or nullptr when it isn't known. */
	const ActionMenuPowerInfo *getPower(uint32_t power) const;

	/** The talents from feat.2da/spells.2da, falling back to the defaults. */
	static const ActionMenuTalents &get();

private:
	std::map<uint32_t, ActionMenuFeatInfo> _feats;
	std::map<uint32_t, ActionMenuPowerInfo> _powers;
};

/** An inventory item the menus may offer. */
struct ActionMenuItemEntry {
	enum Kind {
		kKindNone,    ///< Not usable from the action menus.
		kKindMedical, ///< Restores vitality.
		kKindOther,   ///< Other usable item (stim, shield, ...).
		kKindGrenade, ///< Thrown at the target.
		kKindMine     ///< Set on the ground.
	};

	Common::UString tag;
	Common::UString icon;
	Common::UString name;
	Kind kind { kKindNone };
};

/** Describe an inventory item for the menus: mine, medical (restores vitality),
 *  grenade, other usable item, or not usable from the menus. */
ActionMenuItemEntry describeActionMenuItem(const Item &item);

/** What the target is, for the target menu. */
enum ActionMenuTarget {
	kActionMenuTargetNone,     ///< Nothing selected.
	kActionMenuTargetHostile,  ///< A living creature hostile to the party.
	kActionMenuTargetFriendly, ///< Any other creature.
	kActionMenuTargetObject    ///< A door, placeable, item, ...
};

struct ActionMenuInput {
	const CreatureInfo *leader { nullptr }; ///< The party leader's feats and powers.
	ActionMenuTarget target { kActionMenuTargetNone };
	bool targetCanBeUnlocked { false }; ///< The target is locked and needs no key.
	bool targetIsMine { false };        ///< The target is a detected, disarmable mine.
	bool targetCanBeBashed { false };   ///< The target is a locked door or container that can be broken open.
	/** Force powers of the leader's equipped items (armbands): spell ID and item tag. */
	std::vector<std::pair<int, Common::UString>> itemPowers;
	bool rangedWeapon { false };        ///< The leader wields a ranged weapon.
	std::vector<ActionMenuItemEntry> items;
};

typedef std::array<ActionMenuPanel, kActionMenuTargetPanels> ActionMenuTargetPanels;
typedef std::array<ActionMenuPanel, kActionMenuSelfPanels>   ActionMenuSelfPanels;

/** Build the options shown above the target (KotOR.js' UpdateMenuActions). */
ActionMenuTargetPanels buildTargetActionMenu(const ActionMenuInput &input, const ActionMenuTalents &talents);
/** Build the leader's own options (friendly powers, medical and other items). */
ActionMenuSelfPanels buildSelfActionMenu(const ActionMenuInput &input, const ActionMenuTalents &talents);

/** The leader's known combat feats of @p category, without those whose successor is known too. */
std::vector<uint32_t> getUsableCombatFeats(const CreatureInfo &info, int category, const ActionMenuTalents &talents);

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_ACTIONMENU_H
