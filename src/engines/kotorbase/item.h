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
 *  Inventory item in KotOR games.
 */

#ifndef ENGINES_KOTORBASE_ITEM_H
#define ENGINES_KOTORBASE_ITEM_H

#include <memory>
#include <vector>

#include "src/graphics/aurora/types.h"

#include "src/engines/kotorbase/types.h"

#include "src/engines/kotorbase/object.h"

namespace Aurora {
	class TwoDAFile;
}

namespace Engines {

namespace KotORBase {

/** The spells.2da row a CastSpell item property's subtype names. @p subtypeTable is
 *  the property's subtype 2DA when it is an iprp_spells.2da style table (with a
 *  "spellindex" column), or nullptr when the subtype is the spells.2da row itself. */
int resolveCastSpellSubtype(int subtype, const Aurora::TwoDAFile *subtypeTable);

class Item : public Object {
public:
	Item(const Common::UString &item);
	~Item() override;

	// Basic properties

	const Common::UString &getName() const;
	WeaponWield getWeaponWield() const;
	float getMaxAttackRange() const;
	int getNumDice() const;
	int getDieToRoll() const;

	bool isSlotEquipable(InventorySlot slot) const;
	bool isRangedWeapon() const;

	int getACBonus() const;
	int getBaseACBonus() const;
	int getEnhancementBonus() const;
	int getDamageBonus() const;
	int getAttackBonus() const;
	int getBaseItem() const;
	int getCost() const;
	const Common::UString &getItemClass() const;

	bool hasItemProperty(int propertyType) const;
	/** Returns Param1Value for the first property of @p propertyType, or @p fallback. */
	int getItemPropertyValue(int propertyType, int fallback = 0) const;
	/** Returns Subtype for the first property of @p propertyType, or @p fallback. */
	int getItemPropertySubtype(int propertyType, int fallback = 0) const;

	/** Bonus the item's Ability Bonus properties give to @p ability. */
	int getAbilityBonus(Ability ability) const;
	/** The spells.2da rows the item's CastSpell properties cast. */
	std::vector<int> getCastSpells() const;
	/** The spells the consumable item template @p resRef (medpac, stim, grenade...) casts
	 *  when used, cached. Empty for equipment and when the template can't be loaded. */
	static const std::vector<int> &getConsumableSpells(const Common::UString &resRef);

	int  getStackSize() const;
	void setStackSize(int size);

	// Visual properties

	int getBodyVariation() const;
	int getTextureVariation() const;
	const Common::UString getIcon() const;
	const Common::UString getModelName() const;

	bool isVisible() const override;
	void show() override;
	void hide() override;
	void setPosition(float x, float y, float z) override;

	/** Load the world model and mark this item as a clickable ground pickup. */
	void prepareWorldDrop(const Common::UString &templateResRef);

	void setUpgradeBonuses(int attack, int damage, int ac);

private:
	struct ItemPropertyData {
		int type { 0 };
		int subtype { 0 };
		int param1 { 0 };
		int param1Value { 0 };
		int costValue { 0 };

		/** The bonus a property gives: its cost table value (iprp_bonuscost rows are
		 *  the bonus itself), or Param1Value for properties that store it there. */
		int amount() const { return (costValue > 0) ? costValue : param1Value; }
	};

	int _baseItem;
	Common::UString _itemClass;
	int32_t _equipableSlotsMask;
	WeaponWield _weaponWield;
	bool _rangedWeapon;
	float _maxAttackRange;
	int _numDice;
	int _dieToRoll;

	int _modelVariation;
	int _bodyVariation;
	int _textureVariation;
	int _acBonus;
	int _stackSize; ///< Stack count (grenades, medpacs, etc. — default 1).
	int _cost;
	int _upgradeAttackBonus { 0 };
	int _upgradeDamageBonus { 0 };
	int _upgradeACBonus { 0 };

	std::vector<ItemPropertyData> _properties;
	std::unique_ptr<Graphics::Aurora::Model> _model;

	int getPropertyBonusSum(int propertyType) const;

	void load(const Aurora::GFF3Struct &gff);
};

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_ITEM_H
