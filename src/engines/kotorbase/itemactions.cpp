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

#include <cfloat>

#include "src/common/exception.h"

#include "src/engines/kotorbase/actionexecutor.h"
#include "src/engines/kotorbase/area.h"
#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/effect.h"
#include "src/engines/kotorbase/item.h"
#include "src/engines/kotorbase/itemactions.h"
#include "src/engines/kotorbase/itemupgrades.h"
#include "src/engines/kotorbase/module.h"

namespace Engines {

namespace KotORBase {

static bool itemClassIs(const Item &item, const char *itemClass) {
	return item.getItemClass().equalsIgnoreCase(itemClass);
}

static int healAmountForSpell(uint32_t spellId, int propertyValue) {
	if (propertyValue > 0)
		return propertyValue;

	switch (spellId) {
	case 1:  return 15;
	case 37: return 25;
	case 38: return 40;
	case 39: return 60;
	default:
		break;
	}

	const ActionExecutor::SpellInfo *spell = ActionExecutor::getSpellInfo(spellId);
	if (spell) {
		Common::UString label = spell->label;
		label.toLower();
		if (label.contains("heal"))
			return 20;
	}

	return 0;
}

static int healAmountForItem(const Item &item) {
	if (item.hasItemProperty(kItemPropertyCastSpell)) {
		const int propertyValue = item.getItemPropertyValue(kItemPropertyCastSpell, 0);
		const int spellId = item.getItemPropertySubtype(kItemPropertyCastSpell, 0);
		const int spellHeal = healAmountForSpell(spellId, propertyValue);
		if (spellHeal > 0)
			return spellHeal;
	}

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

int grenadeDamageForItem(const Item &item) {
	int dice = item.getNumDice();
	int die = item.getDieToRoll();
	if (dice <= 0)
		dice = 4;
	if (die <= 0)
		die = 6;

	return dice * die;
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

static ItemActionResult useGrenade(Creature &target, Creature &inventoryOwner,
                                   const Item &item, const Common::UString &tag, Module *module) {
	ItemActionResult result;

	if (!module) {
		result.message = "Grenades cannot be used here.";
		return result;
	}

	Area *area = module->getCurrentArea();
	if (!area) {
		result.message = "Grenades cannot be used here.";
		return result;
	}

	const int damage = grenadeDamageForItem(item);
	int targetsHit = 0;

	for (Creature *creature : area->getCreatures()) {
		if (!creature || creature->isDead() || creature->isPC())
			continue;
		if (module->isObjectPartyMember(creature))
			continue;
		if (target.getDistanceTo(creature) > 8.0f)
			continue;

		creature->applyEffect(Effect(kKotOREffectDamage, damage));
		++targetsHit;
	}

	module->playSound("exp_generic");
	inventoryOwner.getCreatureInfo().removeInventoryItem(tag, 1);
	result.success = true;
	result.message = targetsHit > 0 ?
	                 "Grenade detonated." :
	                 "Grenade detonated, but nothing was in range.";
	return result;
}

ItemActionResult useInventoryItem(Creature &target, Creature &inventoryOwner, const Common::UString &tag,
                                  Module *module) {
	ItemActionResult result;

	if (!inventoryOwner.getInventory().hasItem(tag)) {
		result.message = "Item is not in inventory.";
		return result;
	}

	try {
		Item item(tag);

		if (itemClassIs(item, "grenade"))
			return useGrenade(target, inventoryOwner, item, tag, module);

		const int heal = healAmountForItem(item);
		if (heal > 0) {
			if (target.getCurrentHitPoints() >= target.getMaxHitPoints()) {
				result.message = "Vitality is already at maximum.";
				return result;
			}

			target.applyEffect(Effect(kKotOREffectHeal, heal));
			inventoryOwner.getCreatureInfo().removeInventoryItem(tag, 1);
			if (module)
				module->playSound("gui_actuse");
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
			if (module)
				module->playSound("gui_actuse");
			result.success = true;
			result.message = "Item used.";
			return result;
		}

		if (isEquipableItem(item))
			return equipInventoryItem(target, inventoryOwner, tag, module);

		result.message = "This item cannot be used.";
	} catch (Common::Exception &e) {
		result.message = e.what();
	}

	return result;
}

ItemActionResult equipInventoryItem(Creature &target, Creature &inventoryOwner, const Common::UString &tag,
                                    Module *module) {
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
		if (module)
			refreshCreatureEquipmentUpgrades(target, *module);
		result.success = true;
		result.message = "Item equipped.";
	} catch (Common::Exception &e) {
		result.message = e.what();
	}

	return result;
}

ItemActionResult dropInventoryItem(Creature &inventoryOwner, const Common::UString &tag, int count,
                                   Module *module, float dropX, float dropY, float dropZ,
                                   bool useFixedDropPosition) {
	ItemActionResult result;

	if (!inventoryOwner.getInventory().hasItem(tag)) {
		result.message = "Item is not in inventory.";
		return result;
	}

	if (count < 1)
		count = 1;

	inventoryOwner.getCreatureInfo().removeInventoryItem(tag, count);
	const bool itemFullyRemoved = !inventoryOwner.getInventory().hasItem(tag);

	if (module) {
		module->playSound("gui_actuse");
		Area *area = module->getCurrentArea();
		if (area) {
			float x = dropX;
			float y = dropY;
			float z = dropZ;

			if (!useFixedDropPosition) {
				Creature *leader = module->getPartyLeader();
				if (leader)
					leader->getPosition(x, y, z);
			}

			const float elevation = area->evaluateElevation(x, y);
			if (elevation != FLT_MIN)
				z = elevation;

			area->spawnDroppedItem(tag, x, y, z);
			module->setGlobalString("DROP_LAST_TAG", tag);
			module->setGlobalNumber("DROP_LAST_X", static_cast<int>(x));
			module->setGlobalNumber("DROP_LAST_Y", static_cast<int>(y));
			module->setGlobalNumber("DROP_LAST_Z", static_cast<int>(z));
		}

		if (itemFullyRemoved)
			clearAppliedUpgrades(*module, tag);
	}

	result.success = true;
	result.message = "Item dropped.";
	return result;
}

} // End of namespace KotORBase

} // End of namespace Engines
