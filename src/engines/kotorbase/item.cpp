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

#include <map>

#include "src/common/string.h"
#include "src/common/strutil.h"

#include "src/aurora/gff3file.h"
#include "src/aurora/2dafile.h"
#include "src/aurora/2dareg.h"

#include "src/engines/aurora/model.h"
#include "src/graphics/aurora/model.h"

#include "src/engines/kotorbase/item.h"

namespace Engines {

namespace KotORBase {

Item::~Item() = default;

Item::Item(const Common::UString &item) : Object(kObjectTypeItem), _stackSize(1) {
	std::unique_ptr<Aurora::GFF3File> uti = std::make_unique<Aurora::GFF3File>(item, Aurora::kFileTypeUTI);

	load(uti->getTopLevel());

	// Inventories are keyed by the template an item was made from
	_templateResRef = item;
}

int Item::getCost() const {
	return _cost;
}

bool Item::hasItemProperty(int propertyType) const {
	for (const ItemPropertyData &prop : _properties) {
		if (prop.type == propertyType)
			return true;
	}
	return false;
}

/** A Damage Bonus property's CostValue is a row of iprp_damagecost: dice
 *  ("numdice" d "die"; their average here) or a flat bonus (the row label). */
static int damageCostAmount(int costValue) {
	try {
		const Aurora::TwoDAFile &twoda = TwoDAReg.get2DA("iprp_damagecost");
		const Aurora::TwoDARow &row = twoda.getRow(static_cast<size_t>(costValue));
		if (!row.empty("numdice") && !row.empty("die"))
			return (row.getInt("numdice") * (row.getInt("die") + 1)) / 2;
		if (!row.empty("label"))
			return row.getInt("label");
	} catch (...) {
	}

	return costValue;
}

int Item::getPropertyBonusSum(int propertyType) const {
	int sum = 0;
	for (const ItemPropertyData &prop : _properties) {
		if (prop.type != propertyType)
			continue;

		if ((propertyType == kItemPropertyDamageBonus) && (prop.costValue > 0))
			sum += damageCostAmount(prop.costValue);
		else
			sum += prop.amount();
	}
	return sum;
}

int Item::getBaseACBonus() const {
	return _acBonus;
}

int Item::getEnhancementBonus() const {
	return getPropertyBonusSum(kItemPropertyEnhancementBonus);
}

int Item::getDamageBonus() const {
	return getPropertyBonusSum(kItemPropertyDamageBonus) + _upgradeDamageBonus;
}

int Item::getAttackBonus() const {
	return getPropertyBonusSum(kItemPropertyAttackBonus) + getEnhancementBonus() + _upgradeAttackBonus;
}

void Item::load(const Aurora::GFF3Struct &gff) {
	// Tag, name and description
	_tag = gff.getString("Tag");
	_name = gff.getString("LocalizedName");
	_description = gff.getString("Description");

	// Base item
	_baseItem = gff.getSint("BaseItem");
	const Aurora::TwoDARow &twoDA = TwoDAReg.get2DA("baseitems").getRow(_baseItem);
	_equipableSlotsMask = twoDA.getInt("equipableslots");
	_itemClass = twoDA.getString("itemclass");
	_weaponWield = static_cast<WeaponWield>(twoDA.getInt("weaponwield"));
	_rangedWeapon = twoDA.getInt("rangedweapon");
	_maxAttackRange = twoDA.getFloat("maxattackrange");
	_numDice = twoDA.getInt("numdice");
	_dieToRoll = twoDA.getInt("dietoroll");
	_acBonus = twoDA.getInt("ACBonus");

	// Model, body and texture variations
	_modelVariation = gff.getSint("ModelVariation");
	_bodyVariation = gff.getSint("BodyVariation");
	_textureVariation = gff.getSint("TextureVar");
	_stackSize = gff.getSint("StackSize", 1);
	_cost = gff.getSint("Cost");

	const Aurora::GFF3List &properties = gff.getList("PropertiesList");
	for (Aurora::GFF3List::const_iterator it = properties.begin(); it != properties.end(); ++it) {
		if (!*it)
			continue;

		ItemPropertyData prop;
		prop.type = (*it)->getUint("PropertyName");
		prop.subtype = (*it)->getUint("Subtype");
		prop.param1 = (*it)->getUint("Param1");
		prop.param1Value = (*it)->getUint("Param1Value");
		prop.costValue = (*it)->getUint("CostValue", 0);
		_properties.push_back(prop);
	}
}

const Common::UString &Item::getName() const {
	return _name;
}

WeaponWield Item::getWeaponWield() const {
	return _weaponWield;
}

float Item::getMaxAttackRange() const {
	return _maxAttackRange;
}

int Item::getNumDice() const {
	return _numDice;
}

int Item::getDieToRoll() const {
	return _dieToRoll;
}

bool Item::isSlotEquipable(InventorySlot slot) const {
	if (slot > 31)
		return false;

	return (_equipableSlotsMask & (1U << slot)) != 0;
}

bool Item::isRangedWeapon() const {
	return _rangedWeapon;
}

int Item::getACBonus() const {
	return _acBonus + getPropertyBonusSum(kItemPropertyACBonus) + _upgradeACBonus;
}

int Item::getBaseItem() const {
	return _baseItem;
}

const Common::UString &Item::getItemClass() const {
	return _itemClass;
}

int Item::getItemPropertyValue(int propertyType, int fallback) const {
	for (const ItemPropertyData &prop : _properties) {
		if (prop.type == propertyType)
			return prop.param1Value;
	}
	return fallback;
}

int Item::getItemPropertySubtype(int propertyType, int fallback) const {
	for (const ItemPropertyData &prop : _properties) {
		if (prop.type == propertyType)
			return prop.subtype;
	}
	return fallback;
}

int Item::getAbilityBonus(Ability ability) const {
	int bonus = 0;
	for (const ItemPropertyData &prop : _properties) {
		if ((prop.type == kItemPropertyAbilityBonus) && (prop.subtype == ability))
			bonus += prop.amount();
	}
	return bonus;
}

int resolveCastSpellSubtype(int subtype, const Aurora::TwoDAFile *subtypeTable) {
	if (!subtypeTable || (subtype < 0) || (static_cast<size_t>(subtype) >= subtypeTable->getRowCount()))
		return subtype;

	const Aurora::TwoDARow &row = subtypeTable->getRow(subtype);
	if (row.empty("spellindex"))
		return subtype;

	return row.getInt("spellindex");
}

/** The subtype 2DA of the CastSpell property, when it isn't spells.2da itself. */
static const Aurora::TwoDAFile *getCastSpellSubtypeTable() {
	try {
		const Aurora::TwoDARow &def = TwoDAReg.get2DA("itempropdef").getRow(kItemPropertyCastSpell);
		const Common::UString &table = def.getString("subtyperesref");
		if (table.empty() || table.equalsIgnoreCase("spells"))
			return nullptr;

		return &TwoDAReg.get2DA(table);
	} catch (...) {
		return nullptr;
	}
}

std::vector<int> Item::getCastSpells() const {
	std::vector<int> spells;

	const Aurora::TwoDAFile *subtypeTable = nullptr;
	bool haveTable = false;

	for (const ItemPropertyData &prop : _properties) {
		if (prop.type != kItemPropertyCastSpell)
			continue;

		if (!haveTable) {
			subtypeTable = getCastSpellSubtypeTable();
			haveTable = true;
		}

		const int spell = resolveCastSpellSubtype(prop.subtype, subtypeTable);
		if (spell >= 0)
			spells.push_back(spell);
	}

	return spells;
}

const std::vector<int> &Item::getConsumableSpells(const Common::UString &resRef) {
	static std::map<Common::UString, std::vector<int>> cache;

	std::map<Common::UString, std::vector<int>>::iterator it = cache.find(resRef);
	if (it != cache.end())
		return it->second;

	std::vector<int> spells;
	try {
		const Item item(resRef);
		if (item._equipableSlotsMask == 0)
			spells = item.getCastSpells();
	} catch (...) {
	}

	return cache.emplace(resRef, spells).first->second;
}

int Item::getStackSize() const {
	return _stackSize;
}

void Item::setStackSize(int size) {
	_stackSize = (size >= 0) ? size : 0;
}

int Item::getBodyVariation() const {
	return _bodyVariation;
}

int Item::getTextureVariation() const {
	return _textureVariation;
}

const Common::UString Item::getIcon() const {
	int variation = (_modelVariation == 0) ? _textureVariation : _modelVariation;
	if (variation == 0)
		variation = 1;
	return Common::String::format("i%s_%03d", _itemClass.c_str(), variation);
}

const Common::UString Item::getModelName() const {
	return Common::String::format("%s_%03d", _itemClass.c_str(), _modelVariation);
}

bool Item::isVisible() const {
	return _model && _model->isVisible();
}

void Item::show() {
	if (_model)
		_model->show();
}

void Item::hide() {
	if (_model)
		_model->hide();
}

void Item::setPosition(float x, float y, float z) {
	Object::setPosition(x, y, z);

	if (_model)
		_model->setPosition(x, y, z);
}

void Item::setUpgradeBonuses(int attack, int damage, int ac) {
	_upgradeAttackBonus = attack;
	_upgradeDamageBonus = damage;
	_upgradeACBonus = ac;
}

void Item::prepareWorldDrop(const Common::UString &templateResRef) {
	_templateResRef = templateResRef;
	_tag = templateResRef + "_" + Common::composeString(getID());
	_usable = true;

	const Common::UString &modelName = getModelName();
	if (modelName.empty())
		return;

	_model.reset(loadModelObject(modelName));
	if (!_model)
		return;

	_model->setTag(_tag);
	_model->setClickable(isClickable());
	_ids.push_back(_model->getID());
}

} // End of namespace KotORBase

} // End of namespace Engines
