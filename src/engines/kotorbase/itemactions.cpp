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
 *  Inventory item use, equip, and drop helpers for KotOR games.
 */

#include "src/common/exception.h"

#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/effect.h"
#include "src/engines/kotorbase/item.h"
#include "src/engines/kotorbase/itemactions.h"

namespace Engines {

namespace KotORBase {

static bool itemClassIs(const Item &item, const char *itemClass) {
	return item.getItemClass().equalsIgnoreCase(itemClass);
}

static int healAmountForItem(const Item &item) {
	if (item.hasItemProperty(kItemPropertyCastSpell))
		return item.getItemPropertyValue(kItemPropertyCastSpell, 15);

	if (itemClassIs(item, "medpac"))
		return 30;
	if (itemClassIs(item, "medical"))
		return 50;
	if (itemClassIs(item, "medstation"))
		return 80;

	return 0;
}

static int forceHealAmountForItem(const Item &item) {
	if (itemClassIs(item, "stim"))
		return 10;
	return 0;
}

bool isEquipableItem(const Item &item) {
	static const InventorySlot kSlots[] = {
		kInventorySlotHead, kInventorySlotBody, kInventorySlotHands,
		kInventorySlotRightArm, kInventorySlotLeftArm, kInventorySlotImplant,
		kInventorySlotBelt, kInventorySlotRightWeapon, kInventorySlotLeftWeapon
	};

	for (InventorySlot slot : kSlots) {
		if (item.isSlotEquipable(slot))
			return true;
	}

	return false;
}

bool isUsableConsumable(const Item &item) {
	if (healAmountForItem(item) > 0 || forceHealAmountForItem(item) > 0)
		return true;

	if (itemClassIs(item, "grenade") || itemClassIs(item, "droidrepair") ||
	    itemClassIs(item, "repair") || itemClassIs(item, "spice"))
		return true;

	return item.hasItemProperty(kItemPropertyCastSpell);
}

InventorySlot findEquipSlot(const Item &item, const Creature &creature) {
	static const InventorySlot kPriority[] = {
		kInventorySlotRightWeapon,
		kInventorySlotLeftWeapon,
		kInventorySlotBody,
		kInventorySlotHead,
		kInventorySlotHands,
		kInventorySlotRightArm,
		kInventorySlotLeftArm,
		kInventorySlotImplant,
		kInventorySlotBelt
	};

	for (InventorySlot slot : kPriority) {
		if (!item.isSlotEquipable(slot))
			continue;

		if (slot == kInventorySlotLeftWeapon) {
			const Item *rightWeapon = creature.getEquipedItem(kInventorySlotRightWeapon);
			if (!rightWeapon || item.getWeaponWield() != rightWeapon->getWeaponWield())
				continue;
		}

		return slot;
	}

	return kInventorySlotInvalid;
}

ItemActionResult useInventoryItem(Creature &target, Creature &inventoryOwner, const Common::UString &tag) {
	ItemActionResult result;

	if (!inventoryOwner.getInventory().hasItem(tag)) {
		result.message = "Item is not in inventory.";
		return result;
	}

	try {
		Item item(tag);

		const int heal = healAmountForItem(item);
		if (heal > 0) {
			if (target.getCurrentHitPoints() >= target.getMaxHitPoints()) {
				result.message = "Vitality is already at maximum.";
				return result;
			}

			target.applyEffect(Effect(kKotOREffectHeal, heal));
			inventoryOwner.getCreatureInfo().removeInventoryItem(tag, 1);
			result.success = true;
			result.message = "Item used.";
			return result;
		}

		const int forceHeal = forceHealAmountForItem(item);
		if (forceHeal > 0) {
			if (!target.getCreatureInfo().isJedi()) {
				result.message = "Only Force users can use this item.";
				return result;
			}

			if (target.getForcePoints() >= target.getMaxForcePoints()) {
				result.message = "Force points are already at maximum.";
				return result;
			}

			int forcePoints = target.getForcePoints() + forceHeal;
			if (forcePoints > target.getMaxForcePoints())
				forcePoints = target.getMaxForcePoints();
			target.setForcePoints(forcePoints);
			inventoryOwner.getCreatureInfo().removeInventoryItem(tag, 1);
			result.success = true;
			result.message = "Item used.";
			return result;
		}

		if (isEquipableItem(item))
			return equipInventoryItem(target, inventoryOwner, tag);

		result.message = "This item cannot be used.";
	} catch (Common::Exception &e) {
		result.message = e.what();
	}

	return result;
}

ItemActionResult equipInventoryItem(Creature &target, Creature &inventoryOwner, const Common::UString &tag) {
	ItemActionResult result;

	if (!inventoryOwner.getInventory().hasItem(tag)) {
		result.message = "Item is not in inventory.";
		return result;
	}

	try {
		Item item(tag);
		const InventorySlot slot = findEquipSlot(item, target);
		if (slot == kInventorySlotInvalid) {
			result.message = "This item cannot be equipped.";
			return result;
		}

		target.equipItem(tag, slot, inventoryOwner.getCreatureInfo());
		result.success = true;
		result.message = "Item equipped.";
	} catch (Common::Exception &e) {
		result.message = e.what();
	}

	return result;
}

ItemActionResult dropInventoryItem(Creature &inventoryOwner, const Common::UString &tag, int count) {
	ItemActionResult result;

	if (!inventoryOwner.getInventory().hasItem(tag)) {
		result.message = "Item is not in inventory.";
		return result;
	}

	if (count < 1)
		count = 1;

	inventoryOwner.getCreatureInfo().removeInventoryItem(tag, count);
	result.success = true;
	result.message = "Item dropped.";
	return result;
}

} // End of namespace KotORBase

} // End of namespace Engines
