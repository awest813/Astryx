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
 *  Store object in KotOR games.
 */

#include "src/aurora/gff3file.h"
#include "src/engines/kotorbase/store.h"
#include "src/engines/kotorbase/item.h"
#include "src/engines/kotorbase/creature.h"

namespace Engines {

namespace KotORBase {

Store::Store() : Object(), _markUp(100), _markDown(50) {
}

Store::~Store() {
}

Inventory &Store::getInventory() {
	return _inventory;
}

const Inventory &Store::getInventory() const {
	return _inventory;
}

int Store::getMarkUp() const {
	return _markUp;
}

void Store::setMarkUp(int markUp) {
	_markUp = markUp;
}

int Store::getMarkDown() const {
	return _markDown;
}

void Store::setMarkDown(int markDown) {
	_markDown = markDown;
}

int Store::getBuyPrice(const Item &item, const Creature &pc) const {
	int cost = item.getCost();
	
	// Default KOTOR logic: MarkUp / 100.
	// Persuade bonus: -1% price per rank of persuade (max 20%).
	int persuadeRank = pc.getSkillRank(kSkillPersuade);
	int discount = std::min(persuadeRank, 20);
	
	int price = (cost * (_markUp - discount)) / 100;
	return std::max(price, 1);
}

int Store::getSellPrice(const Item &item, const Creature &pc) const {
	int cost = item.getCost();
	
	// Default KOTOR logic: MarkDown / 100.
	// Persuade bonus: +1% price per rank of persuade (max 20%).
	int persuadeRank = pc.getSkillRank(kSkillPersuade);
	int bonus = std::min(persuadeRank, 20);
	
	int price = (cost * (_markDown + bonus)) / 100;
	return std::max(price, 1);
}

void Store::saveState(Aurora::GFF3Struct &gff) const {
	gff.setSint("MarkUp", _markUp);
	gff.setSint("MarkDown", _markDown);

	Aurora::GFF3List &invList = gff.getList("ItemList");
	_inventory.save(invList);
}

void Store::loadState(const Aurora::GFF3Struct &gff) {
	_markUp = gff.getSint("MarkUp", 100);
	_markDown = gff.getSint("MarkDown", 50);

	if (gff.hasField("ItemList")) {
		_inventory.read(gff.getList("ItemList"));
	}
}

} // End of namespace KotORBase

} // End of namespace Engines
