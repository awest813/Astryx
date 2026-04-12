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

#include <algorithm>

#include "src/common/debug.h"
#include "src/aurora/gff3file.h"
#include "src/common/util.h"
#include "src/common/error.h"

#include "src/aurora/gff3file.h"

#include "src/engines/kotorbase/creatureinfo.h"

#include "src/engines/kotorbase/gui/chargeninfo.h"

namespace Engines {

namespace KotORBase {

CreatureInfo::ClassLevel::ClassLevel() {
}

CreatureInfo::ClassLevel::ClassLevel(const ClassLevel &other) {
	operator=(other);
}

CreatureInfo::ClassLevel &CreatureInfo::ClassLevel::operator=(const ClassLevel &other) {
	characterClass = other.characterClass;
	level = other.level;

	return *this;
}

CreatureInfo::Skills &CreatureInfo::Skills::operator=(const Skills &other) {
	computerUse = other.computerUse;
	demolitions = other.demolitions;
	stealth = other.stealth;
	awareness = other.awareness;
	persuade = other.persuade;
	repair = other.repair;
	security = other.security;
	treatInjury = other.treatInjury;

	return *this;
}

CreatureInfo::Abilities &CreatureInfo::Abilities::operator=(const Abilities &other) {
	strength = other.strength;
	dexterity = other.dexterity;
	constitution = other.constitution;
	intelligence = other.intelligence;
	wisdom = other.wisdom;
	charisma = other.charisma;

	return *this;
}

CreatureInfo::CreatureInfo() {
}

CreatureInfo::CreatureInfo(const Aurora::GFF3Struct &gff) {
	loadClassLevels(gff);
	loadSkills(gff);
	loadAbilities(gff);
	loadFeats(gff);

	_alignment = gff.getSint("GoodEvil", 50);
}

CreatureInfo::CreatureInfo(const CharacterGenerationInfo &info) {
	_levels.resize(1);
	_levels[0].characterClass = info.getClass();
	_levels[0].level = 1;
	_abilities = info.getAbilities();
	_skills = info.getSkills();
}

CreatureInfo &CreatureInfo::operator=(const CreatureInfo &other) {
	_levels = other._levels;
	_skills = other._skills;
	_abilities = other._abilities;
	_inventory = other._inventory;
	_equipment = other._equipment;
	_feats = other._feats;
	_forcePointsCurrent = other._forcePointsCurrent;
	_forcePointsMax     = other._forcePointsMax;
	_alignment          = other._alignment;

	return *this;
}

void CreatureInfo::save(Aurora::GFF3Struct &gff) const {
	saveAbilities(gff);
	saveSkills(gff);
	
	gff.setUint("CurrentFP", _forcePointsCurrent);
	gff.setUint("MaxFP", _forcePointsMax);
	gff.setSint("GoodEvil", _alignment);

	// Save Feats
	Aurora::GFF3List &featList = gff.getList("FeatList");
	for (auto feat : _feats) {
		Aurora::GFF3Struct &f = featList.addStruct(0);
		f.setUint("Feat", feat);
	}

	// Save Force Powers
	Aurora::GFF3List &powerList = gff.getList("PowerList");
	for (auto power : _forcePowers) {
		Aurora::GFF3Struct &p = powerList.addStruct(0);
		p.setUint("Power", power);
	}

	// Save Class Levels
	Aurora::GFF3List &classList = gff.getList("ClassList");
	for (auto const &cl : _levels) {
		Aurora::GFF3Struct &c = classList.addStruct(0);
		c.setSint("Class", (int)cl.characterClass);
		c.setSint("ClassLevel", cl.level);
	}

	Aurora::GFF3List &invList = gff.getList("ItemList");
	_inventory.save(invList);
}

void CreatureInfo::read(const Aurora::GFF3Struct &gff) {
	loadAbilities(gff);
	loadSkills(gff);
	loadFeats(gff);
	loadForcePowers(gff);
	loadClassLevels(gff);

	_alignment = gff.getSint("GoodEvil", 50);
	_forcePointsCurrent = gff.getUint("CurrentFP");
	_forcePointsMax = gff.getUint("MaxFP");

	if (gff.hasField("ItemList")) {
		_inventory.read(gff.getList("ItemList"));
	}
}

void CreatureInfo::saveAbilities(Aurora::GFF3Struct &gff) const {
	gff.setUint("Str", _abilities.strength);
	gff.setUint("Dex", _abilities.dexterity);
	gff.setUint("Con", _abilities.constitution);
	gff.setUint("Int", _abilities.intelligence);
	gff.setUint("Wis", _abilities.wisdom);
	gff.setUint("Cha", _abilities.charisma);
}

void CreatureInfo::saveSkills(Aurora::GFF3Struct &gff) const {
	gff.setUint("SkillRank_Computer", _skills.computerUse);
	gff.setUint("SkillRank_Demolition", _skills.demolitions);
	gff.setUint("SkillRank_Stealth", _skills.stealth);
	gff.setUint("SkillRank_Awareness", _skills.awareness);
	gff.setUint("SkillRank_Persuade", _skills.persuade);
	gff.setUint("SkillRank_Repair", _skills.repair);
	gff.setUint("SkillRank_Security", _skills.security);
	gff.setUint("SkillRank_TreatInjury", _skills.treatInjury);
}

void CreatureInfo::loadForcePowers(const Aurora::GFF3Struct &gff) {
	_forcePowers.clear();
	if (gff.hasField("PowerList")) {
		Aurora::GFF3List powerList = gff.getList("PowerList");
		for (const auto &power : powerList) {
			_forcePowers.push_back(power->getUint("Power"));
		}
	}
}

int CreatureInfo::getClassLevel(Class charClass) const {
	auto it = std::find_if(_levels.begin(), _levels.end(), [&](const ClassLevel &cl) {
		return cl.characterClass == charClass;
	});

	if (it == _levels.end()) {
		warning("CreatureInfo::getClassLevel(): Invalid character class \"%d\"", charClass);
		return 0;
	}

	return it->level;
}

Class CreatureInfo::getClassByPosition(int position) const {
	if (position >= static_cast<int>(_levels.size())) {
		warning("CreatureInfo::getClassByPosition(): Invalid position \"%d\"", position);
		return kClassInvalid;
	}

	return _levels[position].characterClass;
}

int CreatureInfo::getLevelByPosition(int position) const {
	if (position >= static_cast<int>(_levels.size())) {
		warning("CreatureInfo::getLevelByPosition(): Invalid position \"%d\"", position);
		return 0;
	}

	return _levels[position].level;
}

int CreatureInfo::getNumClasses() const {
	return static_cast<int>(_levels.size());
}

int CreatureInfo::getBAB() const {
	// KOTOR d20 BAB progressions (per class level):
	//   Full (1:1)  — Soldier, CombatDroid
	//   3/4         — Scout, all Jedi classes, ExpertDroid, TechSpecialist, BountyHunter,
	//                  JediWeaponMaster, JediMaster, JediWatchMan, SithMarauder, SithLord,
	//                  SithAssassin
	//   1/2         — Scoundrel, Minion

	int bab = 0;
	for (const auto &cl : _levels) {
		const int lv = cl.level;
		switch (cl.characterClass) {
			case kClassSoldier:
			case kClassCombatDroid:
				bab += lv;
				break;
			case kClassScout:
			case kClassJediGuardian:
			case kClassJediConsular:
			case kClassJediSentinel:
			case kClassExpertDroid:
			case kClassTechSpecialist:
			case kClassBountyHunter:
			case kClassJediWeaponMaster:
			case kClassJediMaster:
			case kClassJediWatchMan:
			case kClassSithMarauder:
			case kClassSithLord:
			case kClassSithAssassin:
				bab += (lv * 3) / 4;
				break;
			case kClassScoundrel:
			case kClassMinion:
			default:
				bab += lv / 2;
				break;
		}
	}
	return bab;
}

void CreatureInfo::incrementClassLevel(Class charClass) {
	auto it = std::find_if(_levels.begin(), _levels.end(), [&](const ClassLevel &cl) {
		return cl.characterClass == charClass;
	});

	if (it != _levels.end()) {
		it->level += 1;
	} else {
		ClassLevel newLevel;
		newLevel.characterClass = charClass;
		newLevel.level = 1;
		_levels.push_back(newLevel);
	}
}

Class CreatureInfo::getLatestClass() const {
	if (_levels.empty())
		return kClassInvalid;
	return _levels.back().characterClass;
}

int CreatureInfo::getAbilityScore(Ability ability) const {
	switch (ability) {
		case kAbilityStrength:
			return _abilities.strength;
		case kAbilityDexterity:
			return _abilities.dexterity;
		case kAbilityConstitution:
			return _abilities.constitution;
		case kAbilityIntelligence:
			return _abilities.intelligence;
		case kAbilityWisdom:
			return _abilities.wisdom;
		case kAbilityCharisma:
			return _abilities.charisma;
		default:
			warning("CreatureInfo::getAbilityScore(): Invalid ability \"%d\"", ability);
			return 0;
	}
}

int CreatureInfo::getAbilityModifier(Ability ability) const {
	int score;

	switch (ability) {
		case kAbilityStrength:
			score = _abilities.strength;
			break;
		case kAbilityDexterity:
			score = _abilities.dexterity;
			break;
		case kAbilityConstitution:
			score = _abilities.constitution;
			break;
		case kAbilityIntelligence:
			score = _abilities.intelligence;
			break;
		case kAbilityWisdom:
			score = _abilities.wisdom;
			break;
		case kAbilityCharisma:
			score = _abilities.charisma;
			break;
		default:
			return -1;
	}

	const int delta = score - 10;
	if (delta >= 0)
		return delta / 2;

	// D20/SRD uses floor((score - 10) / 2), not truncation toward zero.
	return -(((-delta) + 1) / 2);
}

void CreatureInfo::setAbilityScore(Ability ability, uint32_t score) {
	switch (ability) {
		case kAbilityStrength:
			_abilities.strength = score;
			break;
		case kAbilityDexterity:
			_abilities.dexterity = score;
			break;
		case kAbilityConstitution:
			_abilities.constitution = score;
			break;
		case kAbilityIntelligence:
			_abilities.intelligence = score;
			break;
		case kAbilityWisdom:
			_abilities.wisdom = score;
			break;
		case kAbilityCharisma:
			_abilities.charisma = score;
			break;
		default:
			warning("CreatureInfo::setAbilityScore(): Invalid ability \"%d\"", ability);
			break;
	}
}

int CreatureInfo::getSkillRank(Skill skill) const {
	switch (skill) {
		case kSkillComputerUse:
			return _skills.computerUse;
		case kSkillDemolitions:
			return _skills.demolitions;
		case kSkillStealth:
			return _skills.stealth;
		case kSkillAwareness:
			return _skills.awareness;
		case kSkillPersuade:
			return _skills.persuade;
		case kSkillRepair:
			return _skills.repair;
		case kSkillSecurity:
			return _skills.security;
		case kSkillTreatInjury:
			return _skills.treatInjury;
		default:
			warning("CreatureInfo::getSkillRank(): Invalid skill \"%d\"", skill);
			return -1;
	}
}

void CreatureInfo::setSkillRank(Skill skill, uint32_t rank) {
	switch (skill) {
		case kSkillComputerUse:
			_skills.computerUse = rank;
			break;
		case kSkillDemolitions:
			_skills.demolitions = rank;
			break;
		case kSkillStealth:
			_skills.stealth = rank;
			break;
		case kSkillAwareness:
			_skills.awareness = rank;
			break;
		case kSkillPersuade:
			_skills.persuade = rank;
			break;
		case kSkillRepair:
			_skills.repair = rank;
			break;
		case kSkillSecurity:
			_skills.security = rank;
			break;
		case kSkillTreatInjury:
			_skills.treatInjury = rank;
			break;
		default:
			warning("CreatureInfo::setSkillRank(): Invalid skill \"%d\"", skill);
			break;
	}
}

uint32_t CreatureInfo::getForcePoints() const {
	return _forcePointsCurrent;
}

uint32_t CreatureInfo::getMaxForcePoints() const {
	return _forcePointsMax;
}

void CreatureInfo::setForcePoints(uint32_t fp) {
	_forcePointsCurrent = fp;
}

void CreatureInfo::setMaxForcePoints(uint32_t fp) {
	_forcePointsMax = fp;
}

void CreatureInfo::addFeat(uint32_t feat) {
	if (hasFeat(feat))
		return;

	_feats.push_back(feat);
}

bool CreatureInfo::hasFeat(uint32_t feat) const {
	for (std::vector<uint32_t>::const_iterator f = _feats.begin(); f != _feats.end(); ++f) {
		if (*f == feat)
			return true;
	}

	return false;
}

const std::vector<uint32_t> &CreatureInfo::getFeats() const {
	return _feats;
}

void CreatureInfo::addForcePower(uint32_t power) {
	if (hasForcePower(power))
		return;

	_forcePowers.push_back(power);
}

bool CreatureInfo::hasForcePower(uint32_t power) const {
	for (auto p : _forcePowers) {
		if (p == power)
			return true;
	}
	return false;
}

const std::vector<uint32_t> &CreatureInfo::getForcePowers() const {
	return _forcePowers;
}

void CreatureInfo::loadClassLevels(const Aurora::GFF3Struct &gff) {
	_levels.clear();

	if (gff.hasField("ClassList")) {
		Aurora::GFF3List classList = gff.getList("ClassList");
		for (const auto &charClass : classList) {
			ClassLevel classLevel;
			classLevel.characterClass = Class(charClass->getSint("Class"));
			classLevel.level = charClass->getSint("ClassLevel");
			_levels.push_back(classLevel);
		}
	}
}

void CreatureInfo::loadSkills(const Aurora::GFF3Struct &gff) {
	if (gff.hasField("SkillList")) {
		Aurora::GFF3List skillList = gff.getList("SkillList");
		int index = 0;
		for (const auto &skill : skillList) {
			setSkillRank(Skill(index++), skill->getUint("Rank"));
		}
	}
}

void CreatureInfo::loadAbilities(const Aurora::GFF3Struct &gff) {
	_abilities.strength     = gff.getUint("Str");
	_abilities.dexterity    = gff.getUint("Dex");
	_abilities.constitution = gff.getUint("Con");
	_abilities.intelligence = gff.getUint("Int");
	_abilities.wisdom       = gff.getUint("Wis");
	_abilities.charisma     = gff.getUint("Cha");
}

void CreatureInfo::loadFeats(const Aurora::GFF3Struct &gff) {
	_feats.clear();

	if (!gff.hasField("FeatList"))
		return;

	const Aurora::GFF3List &feats = gff.getList("FeatList");
	for (Aurora::GFF3List::const_iterator f = feats.begin(); f != feats.end(); ++f) {
		const Aurora::GFF3Struct &feat = **f;
		addFeat(feat.getUint("Feat"));
	}
}

Inventory &CreatureInfo::getInventory() {
	return _inventory;
}

void CreatureInfo::addInventoryItem(const Common::UString &tag, int count) {
	_inventory.addItem(tag, count);
}

void CreatureInfo::removeInventoryItem(const Common::UString &tag, int count) {
	_inventory.removeItem(tag, count);
}

const Common::UString &CreatureInfo::getEquippedItem(InventorySlot slot) const {
	auto it = _equipment.find(slot);
	if (it == _equipment.end())
		throw Common::Exception("CreatureInfo::getEquippedItem(): Inventory slot \"%d\" is empty", slot);

	return it->second;
}

bool CreatureInfo::isInventorySlotEquipped(InventorySlot slot) const {
	return _equipment.find(slot) != _equipment.end();
}

void CreatureInfo::equipItem(const Common::UString &tag, InventorySlot slot) {
	if (_equipment.find(slot) != _equipment.end())
		throw Common::Exception("CreatureInfo::equipItem(): Inventory slot \"%d\" is not empty", slot);

	_equipment.insert(std::make_pair(slot, tag));
}

void CreatureInfo::unequipInventorySlot(InventorySlot slot) {
	_equipment.erase(slot);
}

int CreatureInfo::getAlignment() const {
	return _alignment;
}

void CreatureInfo::setAlignment(int alignment) {
	_alignment = std::clamp(alignment, 0, 100);
}

void CreatureInfo::adjustAlignment(int shift) {
	setAlignment(_alignment + shift);
}

} // End of namespace KotORBase

} // End of namespace Engines
