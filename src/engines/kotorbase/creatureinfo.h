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
 *  Abstract creature information in KotOR games.
 */

#ifndef ENGINES_KOTORBASE_CREATUREINFO_H
#define ENGINES_KOTORBASE_CREATUREINFO_H

#include <vector>

#include "src/common/types.h"

#include "src/engines/kotorbase/types.h"
#include "src/engines/kotorbase/inventory.h"

namespace Aurora {
	class GFF3Struct;
}

namespace Engines {

namespace KotORBase {

class CharacterGenerationInfo;

class CreatureInfo {
public:
	CreatureInfo();
	CreatureInfo(const Aurora::GFF3Struct &gff);
	CreatureInfo(const CharacterGenerationInfo &info);

	CreatureInfo &operator=(const CreatureInfo &other);

	void save(Aurora::GFF3Struct &gff) const;
	void read(const Aurora::GFF3Struct &gff);

	// Class levels

	struct ClassLevel {
		Class characterClass;
		int level;

		ClassLevel();
		ClassLevel(const ClassLevel &other);

		ClassLevel &operator=(const ClassLevel &other);
	};

	int getClassLevel(Class charClass) const;
	Class getClassByPosition(int position) const;
	int getLevelByPosition(int position) const;
	int getNumClasses() const;
	/** Increment the level of the given class by 1 (adds a new ClassLevel entry if needed). */
	void incrementClassLevel(Class charClass);

	/** Return the class that was most recently added/leveled. */
	Class getLatestClass() const;

	/** Compute the total Base Attack Bonus from all class levels (KOTOR d20 rules). */
	int getBAB() const;

	/** Compute the total saving throw bonus for the given category (Class base + Ability mod). */
	int getSavingThrowBonus(SavingThrow type) const;

	/** Return true if the creature has any Jedi class levels. */
	bool isJedi() const;

	// Abilities

	struct Abilities {
		uint32_t strength { 0 };
		uint32_t dexterity { 0 };
		uint32_t constitution { 0 };
		uint32_t intelligence { 0 };
		uint32_t wisdom { 0 };
		uint32_t charisma { 0 };

		Abilities &operator=(const Abilities &other);
	};

	int getAbilityScore(Ability ability) const;
	int getAbilityModifier(Ability ability) const;

	void setAbilityScore(Ability ability, uint32_t score);

	// Skills

	struct Skills {
		uint32_t computerUse { 0 };
		uint32_t demolitions { 0 };
		uint32_t stealth { 0 };
		uint32_t awareness { 0 };
		uint32_t persuade { 0 };
		uint32_t repair { 0 };
		uint32_t security { 0 };
		uint32_t treatInjury { 0 };

		Skills &operator=(const Skills &other);
	};

	int getSkillRank(Skill skill) const;

	void setSkillRank(Skill skill, uint32_t rank);

	// Force Points

	uint32_t getForcePoints() const;
	uint32_t getMaxForcePoints() const;
	void setForcePoints(uint32_t fp);
	void setMaxForcePoints(uint32_t fp);

	// Feats

	void addFeat(uint32_t feat);
	bool hasFeat(uint32_t feat) const;
	int  getFeatRank(uint32_t feat) const;
	const std::vector<uint32_t> &getFeats() const;

	// Force Powers

	void addForcePower(uint32_t power);
	bool hasForcePower(uint32_t power) const;
	const std::vector<uint32_t> &getForcePowers() const;

	// Inventory

	Inventory &getInventory();

	void addInventoryItem(const Common::UString &tag, int count = 1);
	void removeInventoryItem(const Common::UString &tag, int count = 1);

	// Equipment

	const Common::UString &getEquippedItem(InventorySlot slot) const;

	bool isInventorySlotEquipped(InventorySlot slot) const;

	void equipItem(const Common::UString &tag, InventorySlot slot);
	void unequipInventorySlot(InventorySlot slot);

	// Alignment
	int getAlignment() const;
	void setAlignment(int alignment);
	void adjustAlignment(int shift);

private:
	std::vector<ClassLevel> _levels;
	Abilities _abilities;
	Skills _skills;
	Inventory _inventory;
	std::map<InventorySlot, Common::UString> _equipment;
	std::vector<uint32_t> _feats;
	std::vector<uint32_t> _forcePowers;

	uint32_t _forcePointsCurrent { 0 };
	uint32_t _forcePointsMax     { 0 };
	int      _alignment          { 50 }; // Neutral by default

	void saveAbilities(Aurora::GFF3Struct &gff) const;
	void loadAbilities(const Aurora::GFF3Struct &gff);

	void saveSkills(Aurora::GFF3Struct &gff) const;
	void loadSkills(const Aurora::GFF3Struct &gff);

	void loadClassLevels(const Aurora::GFF3Struct &gff);
	void loadFeats(const Aurora::GFF3Struct &gff);
	void loadForcePowers(const Aurora::GFF3Struct &gff);
};

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_CREATUREINFO_H
