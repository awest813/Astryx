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
 *  Workbench upgrade helpers for KotOR games.
 */

#include "src/common/debug.h"
#include "src/common/exception.h"
#include "src/common/strutil.h"

#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/inventory.h"
#include "src/engines/kotorbase/item.h"
#include "src/engines/kotorbase/itemactions.h"
#include "src/engines/kotorbase/itemupgrades.h"
#include "src/engines/kotorbase/module.h"

namespace Engines {

namespace KotORBase {

static Common::UString upgradeGlobalKey(const Common::UString &itemTag, int slot) {
	return Common::UString("UPG_") + itemTag + "_" + Common::composeString(slot);
}

static bool itemClassIs(const Item &item, const char *itemClass) {
	return item.getItemClass().equalsIgnoreCase(itemClass);
}

bool isUpgradeableItem(const Item &item) {
	return item.isSlotEquipable(kInventorySlotRightWeapon) ||
	       item.isSlotEquipable(kInventorySlotBody);
}

bool isUpgradeComponent(const Item &item) {
	if (isUpgradeableItem(item))
		return false;

	const Common::UString &cls = item.getItemClass();
	if (cls.contains("upg") || cls.contains("crystl") || cls.contains("fiber") ||
	    cls.contains("sept") || cls.contains("lsc") || cls.contains("lhbc"))
		return true;

	return itemClassIs(item, "upgrade") || itemClassIs(item, "upcrystl");
}

int getUpgradeSlotCount(const Item &item) {
	if (item.isSlotEquipable(kInventorySlotBody))
		return 2;

	if (item.isSlotEquipable(kInventorySlotRightWeapon)) {
		if (item.isRangedWeapon() || itemClassIs(item, "blaster") ||
		    itemClassIs(item, "blstrrifl") || itemClassIs(item, "blstrpstl"))
			return 4;
		return 3;
	}

	return 0;
}

Common::UString getAppliedUpgrade(const Module &module, const Common::UString &itemTag, int slot) {
	return module.getGlobalString(upgradeGlobalKey(itemTag, slot));
}

std::vector<Common::UString> getCompatibleUpgradeParts(const Item &target, const Inventory &inventory) {
	std::vector<Common::UString> parts;

	if (!isUpgradeableItem(target))
		return parts;

	for (const auto &entry : inventory.getItems()) {
		if (entry.second.count <= 0)
			continue;

		try {
			const Item part(entry.second.tag);
			if (isUpgradeComponent(part))
				parts.push_back(entry.second.tag);
		} catch (Common::Exception &e) {
			warning("getCompatibleUpgradeParts: %s", e.what());
		}
	}

	return parts;
}

ItemActionResult applyWorkbenchUpgrade(Module &module, Creature &inventoryOwner,
                                       const Common::UString &itemTag,
                                       const Common::UString &upgradeTag, int slot) {
	ItemActionResult result;

	if (itemTag.empty() || upgradeTag.empty()) {
		result.message = "Select an item and upgrade part.";
		return result;
	}

	if (!inventoryOwner.getInventory().hasItem(itemTag)) {
		result.message = "Selected item is no longer in inventory.";
		return result;
	}

	if (!inventoryOwner.getInventory().hasItem(upgradeTag)) {
		result.message = "Upgrade part is no longer in inventory.";
		return result;
	}

	try {
		const Item target(itemTag);
		const Item upgrade(upgradeTag);

		if (!isUpgradeableItem(target)) {
			result.message = "This item cannot be upgraded.";
			return result;
		}

		if (!isUpgradeComponent(upgrade)) {
			result.message = "That inventory entry is not an upgrade component.";
			return result;
		}

		const int slotCount = getUpgradeSlotCount(target);
		if (slot < 0 || slot >= slotCount) {
			result.message = "Invalid upgrade slot.";
			return result;
		}

		const Common::UString existing = getAppliedUpgrade(module, itemTag, slot);
		if (!existing.empty()) {
			result.message = "That upgrade slot is already filled.";
			return result;
		}

		module.setGlobalString(upgradeGlobalKey(itemTag, slot), upgradeTag);
		inventoryOwner.getCreatureInfo().removeInventoryItem(upgradeTag, 1);
		result.success = true;
		result.message = "Upgrade installed in slot " + Common::composeString(slot + 1) + ".";
	} catch (Common::Exception &e) {
		result.message = e.what();
	}

	return result;
}

} // End of namespace KotORBase

} // End of namespace Engines
