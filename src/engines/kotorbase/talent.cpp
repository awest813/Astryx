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
 *  Talent engine type for KotOR games.
 */

#include "src/common/error.h"

#include "src/aurora/2dafile.h"
#include "src/aurora/2dareg.h"

#include "src/engines/kotorbase/talent.h"
#include "src/engines/kotorbase/action.h"
#include "src/engines/kotorbase/creature.h"

namespace Engines {

namespace KotORBase {

Talent::Talent(TalentType type, int id, const Common::UString &item) :
		_type(type),
		_id(id),
		_item(item) {
}

bool Talent::isValid() const {
	return ((_type == kTalentTypeSpell) ||
	        (_type == kTalentTypeFeat)  ||
	        (_type == kTalentTypeSkill)) && (_id >= 0);
}

TalentType Talent::getType() const {
	return _type;
}

int Talent::getID() const {
	return _id;
}

int Talent::getCategory() const {
	if (!isValid())
		return -1;

	const Aurora::TwoDAFile *table = TalentTables::fromGameData().forType(_type);
	return table ? getTalentCategory(*table, _id) : -1;
}

const Common::UString &Talent::getItem() const {
	return _item;
}

Aurora::NWScript::EngineType *Talent::clone() const {
	return new Talent(_type, _id, _item);
}


static const Aurora::TwoDAFile *loadTalentTable(const char *name) {
	try {
		return &TwoDAReg.get2DA(name);
	} catch (...) {
		return nullptr;
	}
}

TalentTables TalentTables::fromGameData() {
	TalentTables tables;
	tables.spells = loadTalentTable("spells");
	tables.feats  = loadTalentTable("feat");
	tables.skills = loadTalentTable("skills");
	return tables;
}

const Aurora::TwoDAFile *TalentTables::forType(TalentType type) const {
	switch (type) {
		case kTalentTypeSpell:
			return spells;
		case kTalentTypeFeat:
			return feats;
		case kTalentTypeSkill:
			return skills;
		default:
			return nullptr;
	}
}

int getTalentCategory(const Aurora::TwoDAFile &table, int id) {
	if ((id < 0) || (static_cast<size_t>(id) >= table.getRowCount()))
		return -1;

	const Aurora::TwoDARow &row = table.getRow(id);
	if (row.empty("category"))
		return -1;

	return row.getInt("category");
}

int getTalentMaxCR(const Aurora::TwoDAFile &table, int id) {
	if ((id < 0) || (static_cast<size_t>(id) >= table.getRowCount()))
		return 0;

	const Aurora::TwoDARow &row = table.getRow(id);
	return row.empty("maxcr") ? 0 : row.getInt("maxcr");
}

bool isTalentInCategory(int talentCategory, int category) {
	return (talentCategory >= 0) && ((talentCategory & category) == category);
}

int getForcePowerCost(int baseCost, const Common::UString &goodEvil, int casterAlignment) {
	if (baseCost <= 0)
		return 0;

	// GetAlignmentGoodEvil() bands: light side from 70, dark side up to 30.
	const bool lightCaster = casterAlignment >= 70;
	const bool darkCaster  = casterAlignment <= 30;

	const bool lightPower = goodEvil.beginsWith("G") || goodEvil.beginsWith("g");
	const bool darkPower  = goodEvil.beginsWith("E") || goodEvil.beginsWith("e");

	if ((lightPower && darkCaster) || (darkPower && lightCaster))
		return (baseCost * 3) / 2;

	return baseCost;
}

static void addTalent(std::vector<TalentEntry> &talents, const TalentTables &tables,
                      TalentType type, int id, const Common::UString &item = Common::UString()) {
	const Aurora::TwoDAFile *table = tables.forType(type);
	if (!table)
		return;

	TalentEntry entry;
	entry.type     = type;
	entry.id       = id;
	entry.category = getTalentCategory(*table, id);
	entry.maxCR    = getTalentMaxCR(*table, id);
	entry.item     = item;

	if (entry.category < 0)
		return;

	talents.push_back(entry);
}

std::vector<TalentEntry> getCreatureTalents(const Creature &creature, const TalentTables &tables) {
	std::vector<TalentEntry> talents;

	const CreatureInfo &info = creature.getCreatureInfo();

	// Known Force powers the creature has the Force points for.
	for (uint32_t power : info.getForcePowers()) {
		if (tables.spells && (power < tables.spells->getRowCount())) {
			const Aurora::TwoDARow &row = tables.spells->getRow(power);
			const int cost = getForcePowerCost(row.empty("forcepoints") ? 0 : row.getInt("forcepoints"),
			                                   row.getString("goodevil"), creature.getAlignment());
			if (cost > creature.getForcePoints())
				continue;
		}

		addTalent(talents, tables, kTalentTypeSpell, static_cast<int>(power));
	}

	// Powers of equipped items (armbands) and spells of usable items cost no Force points.
	for (const Creature::ItemPower &power : creature.getEquippedItemPowers())
		addTalent(talents, tables, kTalentTypeSpell, power.spellId, power.item);
	for (const Creature::ItemPower &power : creature.getInventoryItemPowers())
		addTalent(talents, tables, kTalentTypeSpell, power.spellId, power.item);

	for (uint32_t feat : info.getFeats())
		addTalent(talents, tables, kTalentTypeFeat, static_cast<int>(feat));

	for (int skill = kSkillComputerUse; skill < kSkillMAX; ++skill)
		if (info.getSkillRank(static_cast<Skill>(skill)) > 0)
			addTalent(talents, tables, kTalentTypeSkill, skill);

	return talents;
}

static bool isTalentExcluded(const TalentEntry &talent, int excludeType, int excludeId) {
	if ((excludeType < 0) || (talent.type != excludeType))
		return false;

	return (excludeId == kTalentExcludeAllOfType) || (talent.id == excludeId);
}

const TalentEntry *findBestTalent(const std::vector<TalentEntry> &talents, int category, int crMax,
                                  int excludeType, int excludeId) {
	const TalentEntry *best = nullptr;

	for (const TalentEntry &talent : talents) {
		if (!isTalentInCategory(talent.category, category) || (talent.maxCR > crMax))
			continue;
		if (isTalentExcluded(talent, excludeType, excludeId))
			continue;

		if (!best || (talent.maxCR > best->maxCR))
			best = &talent;
	}

	return best;
}

std::vector<const TalentEntry *> findTalentsInCategory(const std::vector<TalentEntry> &talents,
                                                       int category) {
	std::vector<const TalentEntry *> found;

	for (const TalentEntry &talent : talents)
		if (isTalentInCategory(talent.category, category))
			found.push_back(&talent);

	return found;
}

/** The item power casting @p spell (from @p item, if not empty), or nullptr. */
static const Creature::ItemPower *findItemPower(const std::vector<Creature::ItemPower> &powers,
                                                int spell, const Common::UString &item) {
	for (const Creature::ItemPower &power : powers)
		if ((power.spellId == spell) && (item.empty() || (power.item == item)))
			return &power;

	return nullptr;
}

bool creatureHasTalent(const Creature &creature, const Talent &talent) {
	if (!talent.isValid())
		return false;

	const int id = talent.getID();

	switch (talent.getType()) {
		case kTalentTypeSpell:
			if (talent.getItem().empty() && creature.hasForcePower(static_cast<uint32_t>(id)))
				return true;

			return findItemPower(creature.getEquippedItemPowers(), id, talent.getItem()) ||
			       findItemPower(creature.getInventoryItemPowers(), id, talent.getItem());

		case kTalentTypeFeat:
			return creature.hasFeat(static_cast<uint32_t>(id));

		case kTalentTypeSkill:
			if ((id < kSkillComputerUse) || (id >= kSkillMAX))
				return false;

			return creature.getCreatureInfo().getSkillRank(static_cast<Skill>(id)) > 0;

		default:
			return false;
	}
}

Action makeTalentSpellAction(const Creature &caster, const Talent &talent, Object *target) {
	Action action(kActionCastSpell);
	action.object   = target;
	action.actionID = talent.getID();

	if (talent.getItem().empty() && caster.hasForcePower(static_cast<uint32_t>(talent.getID())))
		return action;

	// An item's spell: free, from the equipment first, then using up a consumable.
	const Creature::ItemPower *power = findItemPower(caster.getEquippedItemPowers(), talent.getID(), talent.getItem());
	if (power) {
		action.cheat = true;
		action.item  = power->item;
		return action;
	}

	const std::vector<Creature::ItemPower> consumables = caster.getInventoryItemPowers();
	power = findItemPower(consumables, talent.getID(), talent.getItem());
	if (power) {
		action.cheat       = true;
		action.item        = power->item;
		action.consumeItem = true;
	}

	return action;
}

} // End of namespace KotORBase

} // End of namespace Engines
