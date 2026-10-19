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

#include <cstdlib>

#include <algorithm>

#include "src/common/util.h"

#include "src/aurora/2dafile.h"
#include "src/aurora/2dareg.h"
#include "src/aurora/talkman.h"

#include "src/engines/kotorbase/actionmenu.h"
#include "src/engines/kotorbase/actionexecutor.h"
#include "src/engines/kotorbase/creatureinfo.h"
#include "src/engines/kotorbase/item.h"
#include "src/engines/kotorbase/itemactions.h"
#include "src/engines/kotorbase/types.h"

namespace Engines {

namespace KotORBase {

static const char *kAttackIcon   = "i_attack";
static const char *kSecurityIcon = "isk_security";
static const char *kDemolitionsIcon = "isk_demolitions";

bool ActionMenuOption::operator==(const ActionMenuOption &other) const {
	return (type == other.type) && (id == other.id) && (itemTag == other.itemTag);
}

// ActionMenuPanel

void ActionMenuPanel::clear() {
	_options.clear();
	_selected = 0;
}

void ActionMenuPanel::add(const ActionMenuOption &option) {
	_options.push_back(option);
}

bool ActionMenuPanel::empty() const {
	return _options.empty();
}

size_t ActionMenuPanel::size() const {
	return _options.size();
}

const std::vector<ActionMenuOption> &ActionMenuPanel::getOptions() const {
	return _options;
}

size_t ActionMenuPanel::getSelectedIndex() const {
	return _selected;
}

const ActionMenuOption *ActionMenuPanel::getSelected() const {
	if (_options.empty())
		return nullptr;

	return &_options[MIN(_selected, _options.size() - 1)];
}

void ActionMenuPanel::previous() {
	if (_options.empty()) {
		_selected = 0;
		return;
	}

	_selected = (_selected == 0) ? (_options.size() - 1) : (_selected - 1);
}

void ActionMenuPanel::next() {
	if (_options.empty()) {
		_selected = 0;
		return;
	}

	_selected = (_selected + 1) % _options.size();
}

void ActionMenuPanel::replace(const ActionMenuPanel &panel) {
	const ActionMenuOption *selected = getSelected();
	const ActionMenuOption previous = selected ? *selected : ActionMenuOption();
	const bool hadSelection = selected != nullptr;

	_options = panel._options;
	_selected = 0;

	if (!hadSelection)
		return;

	for (size_t i = 0; i < _options.size(); ++i) {
		if (_options[i] == previous) {
			_selected = i;
			break;
		}
	}
}

// ActionMenuTalents

ActionMenuTalents::ActionMenuTalents() {
}

static int parseTwoDAInt(const Aurora::TwoDARow &row, const char *column, int fallback) {
	const Common::UString &value = row.getString(column);
	if (value.empty())
		return fallback;

	// feat.2da writes the categories in hex ("0x1104").
	char *end = nullptr;
	const long parsed = std::strtol(value.c_str(), &end, 0);
	if (end == value.c_str())
		return fallback;

	return static_cast<int>(parsed);
}

static bool hasColumn(const Aurora::TwoDAFile &twoda, const char *column) {
	return twoda.headerToColumn(column) != Aurora::kFieldIDInvalid;
}

static Common::UString talentName(const Aurora::TwoDARow &row) {
	if (!row.empty("name")) {
		const int strRef = row.getInt("name");
		if (strRef >= 0) {
			const Common::UString &name = TalkMan.getString(static_cast<uint32_t>(strRef));
			if (!name.empty())
				return name;
		}
	}

	return row.getString("label");
}

void ActionMenuTalents::loadFeats(const Aurora::TwoDAFile &feat) {
	const bool hasCategory  = hasColumn(feat, "category");
	const bool hasSuccessor = hasColumn(feat, "successor");
	const bool hasIcon      = hasColumn(feat, "icon");

	for (size_t i = 0; i < feat.getRowCount(); ++i) {
		const Aurora::TwoDARow &row = feat.getRow(i);

		ActionMenuFeatInfo info;
		if (hasCategory)
			info.category = parseTwoDAInt(row, "category", -1);
		if (hasSuccessor)
			info.successor = parseTwoDAInt(row, "successor", -1);
		if (hasIcon)
			info.icon = row.getString("icon");
		info.name = talentName(row);

		_feats[static_cast<uint32_t>(i)] = info;
	}
}

void ActionMenuTalents::loadPowers(const Aurora::TwoDAFile &spells) {
	const bool hasForceHostile  = hasColumn(spells, "forcehostile");
	const bool hasForceFriendly = hasColumn(spells, "forcefriendly");
	const bool hasHostile       = hasColumn(spells, "hostile");
	const bool hasIcon          = hasColumn(spells, "iconresref");

	for (size_t i = 0; i < spells.getRowCount(); ++i) {
		const Aurora::TwoDARow &row = spells.getRow(i);

		ActionMenuPowerInfo info;
		if (hasForceHostile || hasForceFriendly) {
			// As KotOR.js: a number in the column, whatever its value, allows the use.
			info.hostile  = hasForceHostile  && !row.empty("forcehostile");
			info.friendly = hasForceFriendly && !row.empty("forcefriendly");
		} else if (hasHostile) {
			info.hostile  = row.getInt("hostile") != 0;
			info.friendly = !info.hostile;
		} else
			info.friendly = true;

		if (hasIcon)
			info.icon = row.getString("iconresref");
		info.name = talentName(row);

		_powers[static_cast<uint32_t>(i)] = info;
	}
}

void ActionMenuTalents::loadDefaults() {
	struct DefaultFeat {
		uint32_t feat;
		int category;
		int successor;
		const char *name;
	};

	// The combat feats the combat rounds know (types.h FeatID numbering).
	static const DefaultFeat kDefaultFeats[] = {
		{ kFeatFlurry,                 kFeatCategoryMeleeCombat,  kFeatImprovedFlurry,         "Flurry"                   },
		{ kFeatImprovedFlurry,         kFeatCategoryMeleeCombat,  -1,                          "Improved Flurry"          },
		{ kFeatPowerAttack,            kFeatCategoryMeleeCombat,  kFeatImprovedPowerAttack,    "Power Attack"             },
		{ kFeatImprovedPowerAttack,    kFeatCategoryMeleeCombat,  -1,                          "Improved Power Attack"    },
		{ kFeatCriticalStrike,         kFeatCategoryMeleeCombat,  kFeatImprovedCriticalStrike, "Critical Strike"          },
		{ kFeatImprovedCriticalStrike, kFeatCategoryMeleeCombat,  -1,                          "Improved Critical Strike" },
		{ kFeatRapidShot,              kFeatCategoryRangedCombat, kFeatImprovedRapidShot,      "Rapid Shot"               },
		{ kFeatImprovedRapidShot,      kFeatCategoryRangedCombat, -1,                          "Improved Rapid Shot"      },
		{ kFeatPowerBlast,             kFeatCategoryRangedCombat, -1,                          "Power Blast"              },
		{ kFeatSniperShot,             kFeatCategoryRangedCombat, -1,                          "Sniper Shot"              }
	};

	for (const DefaultFeat &feat : kDefaultFeats) {
		ActionMenuFeatInfo info;
		info.category  = feat.category;
		info.successor = feat.successor;
		info.name      = feat.name;

		if (_feats.find(feat.feat) == _feats.end())
			_feats[feat.feat] = info;
	}

	// The powers ActionExecutor::executeCastSpell() knows without impact scripts.
	static const uint32_t kHostilePowers[]  = { 2, 4, 5, 14, 15, 25, 33, 34 };
	static const uint32_t kFriendlyPowers[] = { 1, 3 };

	for (uint32_t power : kHostilePowers) {
		ActionMenuPowerInfo info;
		info.hostile = true;
		if (_powers.find(power) == _powers.end())
			_powers[power] = info;
	}

	for (uint32_t power : kFriendlyPowers) {
		ActionMenuPowerInfo info;
		info.friendly = true;
		if (_powers.find(power) == _powers.end())
			_powers[power] = info;
	}
}

void ActionMenuTalents::setFeat(uint32_t feat, const ActionMenuFeatInfo &info) {
	_feats[feat] = info;
}

void ActionMenuTalents::setPower(uint32_t power, const ActionMenuPowerInfo &info) {
	_powers[power] = info;
}

const ActionMenuFeatInfo *ActionMenuTalents::getFeat(uint32_t feat) const {
	auto it = _feats.find(feat);
	return (it != _feats.end()) ? &it->second : nullptr;
}

const ActionMenuPowerInfo *ActionMenuTalents::getPower(uint32_t power) const {
	auto it = _powers.find(power);
	return (it != _powers.end()) ? &it->second : nullptr;
}

const ActionMenuTalents &ActionMenuTalents::get() {
	static ActionMenuTalents talents;
	static bool loaded = false;

	if (!loaded) {
		loaded = true;

		bool haveFeats = false, havePowers = false;
		try {
			talents.loadFeats(TwoDAReg.get2DA("feat"));
			haveFeats = true;
		} catch (...) {
		}

		try {
			talents.loadPowers(TwoDAReg.get2DA("spells"));
			havePowers = true;
		} catch (...) {
		}

		// Without the game tables, use what the engine knows itself.
		if (!haveFeats || !havePowers) {
			ActionMenuTalents defaults;
			defaults.loadDefaults();

			if (!haveFeats)
				talents._feats = defaults._feats;
			if (!havePowers)
				talents._powers = defaults._powers;
		}
	}

	return talents;
}

// Items

static bool isMedicalItem(const Item &item) {
	Common::UString itemClass = item.getItemClass();
	itemClass.makeLower();
	if (itemClass.contains("med"))
		return true;

	if (!item.hasItemProperty(kItemPropertyCastSpell))
		return false;

	const int spell = item.getItemPropertySubtype(kItemPropertyCastSpell, -1);
	const ActionExecutor::SpellInfo *info = (spell >= 0) ? ActionExecutor::getSpellInfo(spell) : nullptr;
	if (!info)
		return false;

	Common::UString label = info->label;
	label.makeLower();
	return label.contains("heal") || label.contains("medpac");
}

ActionMenuItemEntry describeActionMenuItem(const Item &item) {
	ActionMenuItemEntry entry;
	entry.tag  = item.getTag();
	entry.icon = item.getIcon();
	entry.name = item.getName();

	if (isEquipableItem(item) || !isUsableConsumable(item))
		entry.kind = ActionMenuItemEntry::kKindNone;
	else if (item.getItemClass().equalsIgnoreCase("grenade"))
		entry.kind = ActionMenuItemEntry::kKindGrenade;
	else if (isMedicalItem(item))
		entry.kind = ActionMenuItemEntry::kKindMedical;
	else
		entry.kind = ActionMenuItemEntry::kKindOther;

	return entry;
}

// Building the menus

std::vector<uint32_t> getUsableCombatFeats(const CreatureInfo &info, int category, const ActionMenuTalents &talents) {
	std::vector<uint32_t> feats;

	const std::vector<uint32_t> &known = info.getFeats();
	for (uint32_t feat : known) {
		const ActionMenuFeatInfo *featInfo = talents.getFeat(feat);
		if (!featInfo || (featInfo->category != category))
			continue;

		// As KotOR.js: only the best of a feat chain (Flurry -> Improved Flurry -> ...).
		if ((featInfo->successor >= 0) &&
		    (std::find(known.begin(), known.end(), static_cast<uint32_t>(featInfo->successor)) != known.end()))
			continue;

		feats.push_back(feat);
	}

	return feats;
}

static ActionMenuOption makeTalentOption(ActionMenuOptionType type, uint32_t id,
                                         const Common::UString &icon, const Common::UString &name) {
	ActionMenuOption option;
	option.type = type;
	option.id   = static_cast<int>(id);
	option.icon = icon;
	option.name = name;

	return option;
}

static ActionMenuOption makeItemOption(const ActionMenuItemEntry &item) {
	ActionMenuOption option;
	option.type    = kActionMenuItem;
	option.itemTag = item.tag;
	option.icon    = item.icon;
	option.name    = item.name.empty() ? item.tag : item.name;

	return option;
}

ActionMenuTargetPanels buildTargetActionMenu(const ActionMenuInput &input, const ActionMenuTalents &talents) {
	ActionMenuTargetPanels panels;

	if (input.target == kActionMenuTargetNone)
		return panels;

	// Security on a locked door or placeable.
	if (input.targetCanBeUnlocked) {
		ActionMenuOption option;
		option.type = kActionMenuOpenLock;
		option.icon = kSecurityIcon;
		option.name = "Security";
		panels[kTargetPanelPower].add(option);
	}

	// Demolitions on a mine the party has spotted.
	if (input.targetIsMine) {
		ActionMenuOption disarm;
		disarm.type = kActionMenuDisarmMine;
		disarm.icon = kDemolitionsIcon;
		disarm.name = "Disarm Mine";
		panels[kTargetPanelPower].add(disarm);

		ActionMenuOption recover;
		recover.type = kActionMenuRecoverMine;
		recover.icon = kDemolitionsIcon;
		recover.name = "Recover Mine";
		panels[kTargetPanelPower].add(recover);
	}

	// A physical attack on a locked door or container, as in KotOR.js.
	if (input.targetCanBeBashed) {
		ActionMenuOption bash;
		bash.type = kActionMenuBash;
		bash.icon = kAttackIcon;
		bash.name = "Bash";
		panels[kTargetPanelAttack].add(bash);
	}

	// Hostile actions are only offered against hostile creatures.
	if (input.target != kActionMenuTargetHostile)
		return panels;

	ActionMenuOption attack;
	attack.type = kActionMenuAttack;
	attack.icon = kAttackIcon;
	attack.name = "Attack";
	panels[kTargetPanelAttack].add(attack);

	if (input.leader) {
		const int category = input.rangedWeapon ? kFeatCategoryRangedCombat : kFeatCategoryMeleeCombat;
		for (uint32_t feat : getUsableCombatFeats(*input.leader, category, talents)) {
			const ActionMenuFeatInfo *info = talents.getFeat(feat);
			panels[kTargetPanelAttack].add(makeTalentOption(kActionMenuFeat, feat, info->icon, info->name));
		}

		for (uint32_t power : input.leader->getForcePowers()) {
			const ActionMenuPowerInfo *info = talents.getPower(power);
			if (!info || !info->hostile)
				continue;

			panels[kTargetPanelPower].add(makeTalentOption(kActionMenuPower, power, info->icon, info->name));
		}
	}

	for (const ActionMenuItemEntry &item : input.items)
		if (item.kind == ActionMenuItemEntry::kKindGrenade)
			panels[kTargetPanelItem].add(makeItemOption(item));

	return panels;
}

ActionMenuSelfPanels buildSelfActionMenu(const ActionMenuInput &input, const ActionMenuTalents &talents) {
	ActionMenuSelfPanels panels;

	if (input.leader) {
		for (uint32_t power : input.leader->getForcePowers()) {
			const ActionMenuPowerInfo *info = talents.getPower(power);
			if (!info || !info->friendly)
				continue;

			panels[kSelfPanelPower].add(makeTalentOption(kActionMenuPower, power, info->icon, info->name));
		}
	}

	// Powers of equipped items (energy shields and the like): used on oneself.
	for (const auto &power : input.itemPowers) {
		const ActionMenuPowerInfo *info = talents.getPower(static_cast<uint32_t>(power.first));
		if (info && info->hostile && !info->friendly)
			continue;

		ActionMenuOption option = makeTalentOption(kActionMenuItemPower, static_cast<uint32_t>(power.first),
		                                           info ? info->icon : Common::UString(),
		                                           info ? info->name : Common::UString());
		option.itemTag = power.second;
		if (option.name.empty())
			option.name = power.second;
		panels[kSelfPanelPower].add(option);
	}

	for (const ActionMenuItemEntry &item : input.items) {
		if (item.kind == ActionMenuItemEntry::kKindMedical)
			panels[kSelfPanelMedical].add(makeItemOption(item));
		else if (item.kind == ActionMenuItemEntry::kKindOther)
			panels[kSelfPanelOther].add(makeItemOption(item));
	}

	return panels;
}

} // End of namespace KotORBase

} // End of namespace Engines
