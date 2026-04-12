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

#ifndef ENGINES_KOTORBASE_STORE_H
#define ENGINES_KOTORBASE_STORE_H

#include "src/engines/kotorbase/object.h"
#include "src/engines/kotorbase/inventory.h"

namespace Engines {

namespace KotORBase {

class Store : public Object {
public:
	Store();
	~Store();

	ObjectType getType() const override { return kObjectTypeStore; }

	Inventory &getInventory();
	const Inventory &getInventory() const;

	int getMarkUp() const;
	void setMarkUp(int markUp);

	int getMarkDown() const;
	void setMarkDown(int markDown);

	/** Calculate the price at which the store sells an item to the PC. */
	int getBuyPrice(const Item &item, const Creature &pc) const;
	/** Calculate the price at which the store buys an item from the PC. */
	int getSellPrice(const Item &item, const Creature &pc) const;

	void saveState(Aurora::GFF3Struct &gff) const;
	void loadState(const Aurora::GFF3Struct &gff);

private:
	Inventory _inventory;

	int _markUp;   ///< Purchase price multiplier (default 100%).
	int _markDown; ///< Sale price multiplier (default 50%).
};

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_STORE_H
