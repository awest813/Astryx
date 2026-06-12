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

#ifndef ENGINES_KOTORBASE_ITEMACTIONS_H
#define ENGINES_KOTORBASE_ITEMACTIONS_H

#include "src/common/ustring.h"

#include "src/engines/kotorbase/types.h"

namespace Engines {

namespace KotORBase {

class Creature;
class Item;
class Module;

struct ItemActionResult {
	bool success { false };
	bool needsTargeting { false };
	Common::UString message;
};

bool isEquipableItem(const Item &item);
bool isUsableConsumable(const Item &item);
InventorySlot findEquipSlot(const Item &item, const Creature &creature);

int grenadeDamageForItem(const Item &item);
bool grenadeHasFriendlyFire(const Item &item);

ItemActionResult throwGrenadeAt(Creature &inventoryOwner, const Common::UString &tag, Module &module,
                                float x, float y, float z);

ItemActionResult useInventoryItem(Creature &target, Creature &inventoryOwner, const Common::UString &tag,
                                  Module *module = nullptr);
ItemActionResult equipInventoryItem(Creature &target, Creature &inventoryOwner, const Common::UString &tag,
                                    Module *module = nullptr);
ItemActionResult dropInventoryItem(Creature &inventoryOwner, const Common::UString &tag, int count = 1,
                                   Module *module = nullptr, float dropX = 0.0f, float dropY = 0.0f,
                                   float dropZ = 0.0f, bool useFixedDropPosition = false);

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_ITEMACTIONS_H
