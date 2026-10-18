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
 *  Shared level-up helpers for KotOR games.
 */

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <utility>
#include <vector>

#include "src/common/util.h"
#include "src/common/strutil.h"

#include "src/aurora/talkman.h"
#include "src/aurora/2dafile.h"
#include "src/aurora/2dareg.h"

#include "src/engines/kotorbase/actionexecutor.h"
#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/creatureinfo.h"
#include "src/engines/kotorbase/gui/chargeninfo.h"
#include "src/engines/kotorbase/levelup.h"

namespace Engines {

namespace KotORBase {

static int totalSkillRanks(const CreatureInfo::Skills &skills) {
	return static_cast<int>(skills.computerUse + skills.demolitions + skills.stealth +
	                        skills.awareness + skills.persuade + skills.repair +
	                        skills.security + skills.treatInjury);
}

static int chargenSkillRank(const CharacterGenerationInfo &info, Skill skill) {
	const CreatureInfo::Skills &s = info.getSkills();
	switch (skill) {
	case kSkillComputerUse:  return static_cast<int>(s.computerUse);
	case kSkillDemolitions:  return static_cast<int>(s.demolitions);
	case kSkillStealth:      return static_cast<int>(s.stealth);
	case kSkillAwareness:    return static_cast<int>(s.awareness);
	case kSkillPersuade:     return static_cast<int>(s.persuade);
	case kSkillRepair:       return static_cast<int>(s.repair);
	case kSkillSecurity:     return static_cast<int>(s.security);
	case kSkillTreatInjury:  return static_cast<int>(s.treatInjury);
	default:                 return 0;
	}
}

static int chargenSkillPointBudget(const CharacterGenerationInfo &info) {
	int base = 4;
	switch (info.getClass()) {
	case kClassScout:     base = 6; break;
	case kClassScoundrel: base = 8; break;
	default:              break;
	}

	const int intScore = static_cast<int>(info.getAbilities().intelligence);
	const int intMod = (intScore - 10) / 2;
	const int total = base + intMod;
	return (total < 1) ? 1 : total;
}

static const Aurora::TwoDARow &gainTableRow(const Aurora::TwoDAFile &twoda, int classLevel);
static Common::UString classTableString(Class charClass, const char *column);

/** The level of @p charClass, 0 when the creature has none (without a warning). */
static int classLevelOf(const CreatureInfo &info, Class charClass) {
	for (int i = 0; i < info.getNumClasses(); ++i)
		if (info.getClassByPosition(i) == charClass)
			return info.getLevelByPosition(i);

	return 0;
}

static int totalClassLevel(const CreatureInfo &info) {
	int level = 0;
	for (int i = 0; i < info.getNumClasses(); ++i)
		level += info.getLevelByPosition(i);

	return level;
}

/** Grant the feats @p charClass gets automatically at its current level. */
static void grantClassFeats(CreatureInfo &info, Class charClass) {
	for (uint32_t feat : classGrantedFeats(charClass, classLevelOf(info, charClass)))
		info.addFeat(feat);
}

int levelUpThresholdFromTable(const Aurora::TwoDAFile &exptable, int currentLevel) {
	if (currentLevel < 1 || exptable.headerToColumn("xp") == Aurora::kFieldIDInvalid)
		return -1;

	// Like KotOR.js: the XP for the next level is in row "currentLevel" (row 0 is level 1).
	const Aurora::TwoDARow &row = gainTableRow(exptable, currentLevel + 1);
	if (row.empty("xp"))
		return -1;

	return MAX<int>(row.getInt("xp"), 0);
}

int levelUpThreshold(int currentLevel) {
	if (currentLevel < 1)
		currentLevel = 1;

	bool haveTable = false;
	int xp = -1;
	try {
		const Aurora::TwoDAFile &exptable = TwoDAReg.get2DA("exptable");
		haveTable = exptable.headerToColumn("xp") != Aurora::kFieldIDInvalid;
		xp = levelUpThresholdFromTable(exptable, currentLevel);
	} catch (...) {
	}

	if (xp >= 0)
		return xp;

	// The table ends at the level cap: no further levels.
	if (haveTable)
		return std::numeric_limits<int>::max();

	// No game data: the d20 progression (which the retail table follows).
	return currentLevel * (currentLevel + 1) / 2 * 1000;
}

int killXPFromTable(const Aurora::TwoDAFile &xptable, int partyLevel, int challengeRating) {
	if (partyLevel < 1 || xptable.getRowCount() == 0)
		return -1;

	// Columns "cr0".."crN"; a higher challenge rating uses the highest column.
	Common::UString column;
	for (int cr = MAX(challengeRating, 0); cr >= 0; --cr) {
		const Common::UString name = "cr" + Common::composeString(cr);
		if (xptable.headerToColumn(name) != Aurora::kFieldIDInvalid) {
			column = name;
			break;
		}
	}

	if (column.empty())
		return -1;

	// KotOR.js reads the row index as the party level. Prefer a column naming it.
	const Common::UString level = Common::composeString(partyLevel);
	const Aurora::TwoDARow *row = nullptr;
	for (const char *levelColumn : { "level", "label" }) {
		if (xptable.headerToColumn(levelColumn) == Aurora::kFieldIDInvalid)
			continue;

		const Aurora::TwoDARow &named = xptable.getRow(levelColumn, level);
		if (!named.empty(levelColumn)) {
			row = &named;
			break;
		}
	}

	if (!row)
		row = &xptable.getRow(MIN<size_t>(static_cast<size_t>(partyLevel), xptable.getRowCount() - 1));

	if (row->empty(column))
		return 0;

	return MAX<int>(row->getInt(column), 0);
}

int killExperience(int partyLevel, int challengeRating) {
	if (challengeRating < 0)
		challengeRating = 0;

	try {
		const int xp = killXPFromTable(TwoDAReg.get2DA("xptable"), MAX(partyLevel, 1), challengeRating);
		if (xp >= 0)
			return xp;
	} catch (...) {
	}

	// No game data.
	return 25 * challengeRating;
}

bool canLevelUp(const Creature &creature) {
	const int level = creature.getHitDice();
	return creature.getCurrentXP() >= levelUpThreshold(level);
}

int classHitDie(Class charClass) {
	switch (progressionClass(charClass)) {
	case kClassScout:
	case kClassJediSentinel:
		return 8;
	case kClassScoundrel:
	case kClassJediConsular:
		return 6;
	case kClassSoldier:
	case kClassJediGuardian:
	default:
		return 10;
	}
}

int hpGainOnLevelUp(Class charClass, int constitutionModifier) {
	const int gain = classHitDie(charClass) / 2 + 1 + constitutionModifier;
	return (gain < 1) ? 1 : gain;
}

void applyLevelUp(Creature &creature) {
	CreatureInfo &info = creature.getCreatureInfo();
	const Class pcClass = info.getLatestClass();

	info.incrementClassLevel(pcClass);
	grantClassFeats(info, pcClass);

	const int hpGain = hpGainOnLevelUp(pcClass,
	                                   info.getAbilityModifier(kAbilityConstitution));
	creature.setMaxHitPoints(creature.getMaxHitPoints() + hpGain);
	creature.setCurrentHitPoints(creature.getMaxHitPoints());

	creature.setMaxForcePoints(creature.computeMaxForcePoints());
	creature.setForcePoints(creature.getMaxForcePoints());

	status("Level up applied for %s. New HitDice: %d", creature.getName().c_str(), creature.getHitDice());
}

void applyJediClass(Creature &creature, Class jediClass) {
	if (!isJediClass(jediClass)) {
		warning("applyJediClass(): invalid Jedi class %d", static_cast<int>(jediClass));
		return;
	}

	CreatureInfo &info = creature.getCreatureInfo();
	info.incrementClassLevel(jediClass);
	grantClassFeats(info, jediClass);

	if (!info.hasFeat(kFeatJediDefense))
		info.addFeat(kFeatJediDefense);

	creature.setMaxForcePoints(creature.computeMaxForcePoints());
	creature.setForcePoints(creature.getMaxForcePoints());
}

bool isClassSkill(Class c, Skill s) {
	c = progressionClass(c);

	switch (c) {
	case kClassScout:
		return (s == kSkillComputerUse || s == kSkillDemolitions ||
		        s == kSkillStealth || s == kSkillAwareness ||
		        s == kSkillRepair || s == kSkillTreatInjury);
	case kClassScoundrel:
		return (s == kSkillDemolitions || s == kSkillStealth ||
		        s == kSkillAwareness || s == kSkillPersuade ||
		        s == kSkillSecurity || s == kSkillTreatInjury);
	case kClassSoldier:
		return (s == kSkillDemolitions || s == kSkillAwareness ||
		        s == kSkillTreatInjury);
	case kClassJediGuardian:
		return (s == kSkillAwareness || s == kSkillPersuade ||
		        s == kSkillTreatInjury);
	case kClassJediSentinel:
		return (s == kSkillComputerUse || s == kSkillDemolitions ||
		        s == kSkillAwareness || s == kSkillPersuade ||
		        s == kSkillSecurity || s == kSkillTreatInjury);
	case kClassJediConsular:
		return (s == kSkillComputerUse || s == kSkillDemolitions ||
		        s == kSkillAwareness || s == kSkillPersuade ||
		        s == kSkillRepair || s == kSkillTreatInjury);
	default:
		return false;
	}
}

int skillPointsPerLevel(const CreatureInfo &info) {
	int base = 1;
	if (info.getNumClasses() > 0) {
		switch (progressionClass(info.getLatestClass())) {
		case kClassScout:           base = 2; break;
		case kClassScoundrel:       base = 3; break;
		case kClassJediGuardian:    base = 1; break;
		case kClassJediConsular:    base = 1; break;
		case kClassJediSentinel:    base = 2; break;
		case kClassExpertDroid:     base = 4; break;
		default:                    base = 1; break;
		}
	}

	const int total = base + info.getAbilityModifier(kAbilityIntelligence);
	return (total < 1) ? 1 : total;
}

bool grantsAbilityIncrease(int currentLevel) {
	return ((currentLevel + 1) % 4) == 0;
}

static void appendClassFeats(std::vector<uint32_t> &feats, Class pcClass,
                            const std::vector<uint32_t> &knownFeats) {
	switch (progressionClass(pcClass)) {
	case kClassSoldier:
		for (uint32_t feat : { kFeatPowerAttack, kFeatFlurry, kFeatCriticalStrike,
		                       kFeatToughness, kFeatConditioning }) {
			bool known = false;
			for (uint32_t k : knownFeats)
				if (k == feat) { known = true; break; }
			if (!known)
				feats.push_back(feat);
		}
		break;
	case kClassScout:
		for (uint32_t feat : { kFeatFlurry, kFeatPowerBlast, kFeatRapidShot,
		                       kFeatSniperShot, kFeatToughness, kFeatConditioning }) {
			bool known = false;
			for (uint32_t k : knownFeats)
				if (k == feat) { known = true; break; }
			if (!known)
				feats.push_back(feat);
		}
		break;
	case kClassScoundrel:
		for (uint32_t feat : { kFeatCriticalStrike, kFeatSniperShot, kFeatRapidShot,
		                       kFeatToughness, kFeatConditioning }) {
			bool known = false;
			for (uint32_t k : knownFeats)
				if (k == feat) { known = true; break; }
			if (!known)
				feats.push_back(feat);
		}
		break;
	case kClassJediGuardian:
		for (uint32_t feat : { kFeatFlurry, kFeatPowerAttack, kFeatJediDefense, kFeatToughness }) {
			bool known = false;
			for (uint32_t k : knownFeats)
				if (k == feat) { known = true; break; }
			if (!known)
				feats.push_back(feat);
		}
		break;
	case kClassJediSentinel:
		for (uint32_t feat : { kFeatFlurry, kFeatCriticalStrike, kFeatJediDefense, kFeatConditioning }) {
			bool known = false;
			for (uint32_t k : knownFeats)
				if (k == feat) { known = true; break; }
			if (!known)
				feats.push_back(feat);
		}
		break;
	case kClassJediConsular:
		for (uint32_t feat : { kFeatJediDefense, kFeatToughness, kFeatConditioning }) {
			bool known = false;
			for (uint32_t k : knownFeats)
				if (k == feat) { known = true; break; }
			if (!known)
				feats.push_back(feat);
		}
		break;
	default:
		for (uint32_t feat : { kFeatPowerAttack, kFeatToughness }) {
			bool known = false;
			for (uint32_t k : knownFeats)
				if (k == feat) { known = true; break; }
			if (!known)
				feats.push_back(feat);
		}
		break;
	}
}

std::vector<uint32_t> getSelectableFeats(Class charClass, const std::vector<uint32_t> &knownFeats) {
	std::vector<uint32_t> feats;
	appendClassFeats(feats, charClass, knownFeats);
	return feats;
}

Common::UString classFeatTableCode(Class charClass) {
	// Like KotOR.js CreatureClass: feat.2da columns are "<featstable>_list" etc.
	Common::UString code = classTableString(charClass, "featstable");
	if (code.empty())
		code = classTableString(charClass, "featgain");

	code.makeLower();
	return code;
}

std::vector<uint32_t> grantedFeatsFromTable(const Aurora::TwoDAFile &feats,
                                            const Common::UString &code, int classLevel) {

	std::vector<uint32_t> granted;

	const Common::UString listColumn    = code + "_list";
	const Common::UString grantedColumn = code + "_granted";
	if (code.empty() || classLevel < 1 || feats.headerToColumn(grantedColumn) == Aurora::kFieldIDInvalid)
		return granted;

	const bool hasList = feats.headerToColumn(listColumn) != Aurora::kFieldIDInvalid;

	for (size_t i = 0; i < feats.getRowCount(); ++i) {
		const Aurora::TwoDARow &row = feats.getRow(i);
		if (row.empty(grantedColumn) || row.getInt(grantedColumn) != classLevel)
			continue;

		// KotOR.js CharGenFeats: only list status 3 ("granted") feats are given out.
		if (hasList && (row.empty(listColumn) || row.getInt(listColumn) != 3))
			continue;

		granted.push_back(static_cast<uint32_t>(i));
	}

	return granted;
}

std::vector<uint32_t> classGrantedFeats(Class charClass, int classLevel) {
	const Common::UString code = classFeatTableCode(charClass);
	if (code.empty() || classLevel < 1)
		return {};

	try {
		return grantedFeatsFromTable(TwoDAReg.get2DA("feat"), code, classLevel);
	} catch (...) {
		return {};
	}
}

static bool containsID(const std::vector<uint32_t> &ids, int32_t id) {
	return (id >= 0) && (std::find(ids.begin(), ids.end(), static_cast<uint32_t>(id)) != ids.end());
}

/** Order @p ranked (rank, id) by rank, followed by @p unranked. */
static std::vector<uint32_t> recommendedFirst(std::vector<std::pair<int32_t, uint32_t>> &ranked,
                                              const std::vector<uint32_t> &unranked) {

	std::stable_sort(ranked.begin(), ranked.end(),
	                 [](const std::pair<int32_t, uint32_t> &a, const std::pair<int32_t, uint32_t> &b) {
		return a.first < b.first;
	});

	std::vector<uint32_t> ordered;
	ordered.reserve(ranked.size() + unranked.size());
	for (const auto &entry : ranked)
		ordered.push_back(entry.second);
	ordered.insert(ordered.end(), unranked.begin(), unranked.end());

	return ordered;
}

/** A positive "recommended" rank in @p column, else 0. */
static int32_t recommendedRank(const Aurora::TwoDARow &row, const Common::UString &column) {
	if (row.empty(column))
		return 0;

	return MAX<int32_t>(row.getInt(column), 0);
}

/** An integer requirement cell; false when the cell is empty. */
static bool requirement(const Aurora::TwoDARow &row, const char *column, int32_t &value) {
	if (row.empty(column))
		return false;

	value = row.getInt(column);
	return true;
}

static bool meetsFeatRequirements(const Aurora::TwoDARow &row, const std::vector<uint32_t> &knownFeats,
                                  const CreatureInfo &next) {

	int32_t value = 0;

	// Both prerequisite feats.
	for (const char *column : { "prereqfeat1", "prereqfeat2" })
		if (requirement(row, column, value) && !containsID(knownFeats, value))
			return false;

	// Any one of the alternative feats.
	bool hasOrReq = false, metOrReq = false;
	for (const char *column : { "orreqfeat0", "orreqfeat1", "orreqfeat2", "orreqfeat3", "orreqfeat4" }) {
		if (!requirement(row, column, value))
			continue;

		hasOrReq = true;
		metOrReq = metOrReq || containsID(knownFeats, value);
	}
	if (hasOrReq && !metOrReq)
		return false;

	if (requirement(row, "mincharlevel", value) && totalClassLevel(next) < value)
		return false;
	if (requirement(row, "minattackbonus", value) && next.getBAB() < value)
		return false;

	static const struct {
		const char *column;
		Ability ability;
	} kMinAbilities[] = {
		{ "minstr", kAbilityStrength     }, { "mindex", kAbilityDexterity },
		{ "mincon", kAbilityConstitution }, { "minint", kAbilityIntelligence },
		{ "minwis", kAbilityWisdom       }, { "mincha", kAbilityCharisma }
	};
	for (const auto &minAbility : kMinAbilities)
		if (requirement(row, minAbility.column, value) && next.getAbilityScore(minAbility.ability) < value)
			return false;

	int32_t levelClass = 0;
	if (requirement(row, "minlevel", value) && requirement(row, "minlevelclass", levelClass) &&
	    classLevelOf(next, static_cast<Class>(levelClass)) < value)
		return false;

	if (requirement(row, "reqskill", value) && (value >= 0) && (value < kSkillMAX)) {
		int32_t ranks = 1;
		requirement(row, "reqskillminranks", ranks);
		if (next.getSkillRank(static_cast<Skill>(value)) < ranks)
			return false;
	}

	return true;
}

std::vector<uint32_t> selectableFeatsFromTable(const Aurora::TwoDAFile &feats, const Common::UString &code,
                                               const std::vector<uint32_t> &knownFeats,
                                               const CreatureInfo &next) {

	const Common::UString listColumn  = code + "_list";
	const Common::UString recomColumn = code + "_recom";
	if (code.empty() || feats.headerToColumn(listColumn) == Aurora::kFieldIDInvalid)
		return {};

	const bool hasConstant = feats.headerToColumn("constant") != Aurora::kFieldIDInvalid;

	std::vector<std::pair<int32_t, uint32_t>> ranked;
	std::vector<uint32_t> unranked;

	for (size_t i = 0; i < feats.getRowCount(); ++i) {
		const Aurora::TwoDARow &row = feats.getRow(i);

		// Unused rows (KotOR.js skips feats without a constant).
		if (hasConstant && row.empty("constant"))
			continue;

		// List status, as KotOR.js CharGenFeats reads it: 0 and 1 are selectable,
		// 2 bonus-only, 3 automatically granted, 4 unavailable.
		if (row.empty(listColumn))
			continue;
		const int32_t status = row.getInt(listColumn);
		if (status != 0 && status != 1)
			continue;

		const uint32_t feat = static_cast<uint32_t>(i);
		if (containsID(knownFeats, static_cast<int32_t>(feat)) || !meetsFeatRequirements(row, knownFeats, next))
			continue;

		const int32_t rank = recommendedRank(row, recomColumn);
		if (rank > 0)
			ranked.emplace_back(rank, feat);
		else
			unranked.push_back(feat);
	}

	return recommendedFirst(ranked, unranked);
}

/** Feats @p next may pick from the game's feat.2da; false without game data. */
static bool readSelectableFeats(Class charClass, const std::vector<uint32_t> &knownFeats,
                                const CreatureInfo &next, std::vector<uint32_t> &feats) {

	const Common::UString code = classFeatTableCode(charClass);
	if (code.empty())
		return false;

	try {
		feats = selectableFeatsFromTable(TwoDAReg.get2DA("feat"), code, knownFeats, next);
		return true;
	} catch (...) {
		return false;
	}
}

std::vector<uint32_t> getSelectableFeats(const CreatureInfo &info) {
	const Class pcClass = info.getNumClasses() > 0 ? info.getLatestClass() : kClassSoldier;

	// Requirements are checked against the character after this level, which
	// also has the feats the level grants automatically.
	CreatureInfo next(info);
	next.incrementClassLevel(pcClass);

	std::vector<uint32_t> known = info.getFeats();
	for (uint32_t feat : classGrantedFeats(pcClass, classLevelOf(next, pcClass)))
		if (!containsID(known, static_cast<int32_t>(feat)))
			known.push_back(feat);

	std::vector<uint32_t> feats;
	if (readSelectableFeats(pcClass, known, next, feats))
		return feats;

	return getSelectableFeats(pcClass, info.getFeats());
}

std::vector<uint32_t> getSelectableFeats(const CharacterGenerationInfo &info,
                                         const std::vector<uint32_t> &knownFeats) {

	const CreatureInfo next(info);

	std::vector<uint32_t> known = knownFeats;
	for (uint32_t feat : classGrantedFeats(info.getClass(), 1))
		if (!containsID(known, static_cast<int32_t>(feat)))
			known.push_back(feat);

	std::vector<uint32_t> feats;
	if (readSelectableFeats(info.getClass(), known, next, feats))
		return feats;

	return getSelectableFeats(info.getClass(), knownFeats);
}

Common::UString forcePowerClassColumn(Class charClass) {
	// spells.2da (KotOR.js TalentSpell): one level column per Force-using class.
	switch (charClass) {
	case kClassJediGuardian:     return "guardian";
	case kClassJediConsular:     return "consular";
	case kClassJediSentinel:     return "sentinel";
	case kClassJediWeaponMaster: return "weaponmaster";
	case kClassJediMaster:       return "jedimaster";
	case kClassJediWatchMan:     return "watchman";
	case kClassSithMarauder:     return "marauder";
	case kClassSithLord:         return "sithlord";
	case kClassSithAssassin:     return "assassin";
	default:                     return "";
	}
}

/** All the underscore-separated spell rows in a "prerequisites" cell are known. */
static bool knowsPrerequisitePowers(const Common::UString &prerequisites, const std::vector<uint32_t> &known) {
	const char *text = prerequisites.c_str();
	while (*text) {
		char *end = nullptr;
		const long id = std::strtol(text, &end, 10);
		if (end == text) {
			++text; // Separator or junk.
			continue;
		}

		if (!containsID(known, static_cast<int32_t>(id)))
			return false;

		text = end;
	}

	return true;
}

std::vector<uint32_t> selectableForcePowersFromTable(const Aurora::TwoDAFile &spells, Class charClass,
                                                     const CreatureInfo &next) {

	const Common::UString classColumn = forcePowerClassColumn(charClass);
	if (classColumn.empty() || spells.headerToColumn(classColumn) == Aurora::kFieldIDInvalid)
		return {};

	const bool hasUserType = spells.headerToColumn("usertype") != Aurora::kFieldIDInvalid;
	const Common::UString recomColumn = (next.getAlignment() < 50) ? "dark_recom" : "light_recom";
	const int characterLevel = totalClassLevel(next);
	const std::vector<uint32_t> &known = next.getForcePowers();

	std::vector<std::pair<int32_t, uint32_t>> ranked;
	std::vector<uint32_t> unranked;

	for (size_t i = 0; i < spells.getRowCount(); ++i) {
		const Aurora::TwoDARow &row = spells.getRow(i);
		if (row.empty(classColumn) || row.getInt(classColumn) > characterLevel)
			continue;

		// Player Force powers only (KotOR.js filters on usertype as well).
		if (hasUserType && (row.empty("usertype") || row.getInt("usertype") != 1))
			continue;

		const uint32_t power = static_cast<uint32_t>(i);
		if (containsID(known, static_cast<int32_t>(power)) || !knowsPrerequisitePowers(row.getString("prerequisites"), known))
			continue;

		const int32_t rank = recommendedRank(row, recomColumn);
		if (rank > 0)
			ranked.emplace_back(rank, power);
		else
			unranked.push_back(power);
	}

	return recommendedFirst(ranked, unranked);
}

Common::UString getFeatDisplayName(uint32_t feat) {
	try {
		const Aurora::TwoDARow &row = TwoDAReg.get2DA("feat").getRow(feat);
		if (!row.empty("name")) {
			const Common::UString &localized = TalkMan.getString(static_cast<uint32_t>(row.getInt("name")));
			if (!localized.empty())
				return localized;
		}

		if (!row.empty("label"))
			return row.getString("label");
	} catch (...) {
	}

	switch (feat) {
	case kFeatPowerAttack:    return "Power Attack";
	case kFeatFlurry:         return "Flurry";
	case kFeatCriticalStrike: return "Critical Strike";
	case kFeatPowerBlast:     return "Power Blast";
	case kFeatRapidShot:      return "Rapid Shot";
	case kFeatSniperShot:     return "Sniper Shot";
	case kFeatConditioning:   return "Conditioning";
	case kFeatToughness:      return "Toughness";
	case kFeatJediDefense:    return "Jedi Defense";
	default:                  return Common::composeString(feat);
	}
}

static int abilityModifier(int score) {
	const int mod = score - 10;
	if (mod >= 0)
		return mod / 2;
	return (mod - 1) / 2;
}

Common::UString getClassDisplayName(Class charClass) {
	uint32_t strRef = 0;

	switch (charClass) {
	case kClassSoldier:       strRef = 134; break;
	case kClassScout:         strRef = 133; break;
	case kClassScoundrel:     strRef = 135; break;
	case kClassJediGuardian:  strRef = 89;  break;
	case kClassJediConsular:  strRef = 90;  break;
	case kClassJediSentinel:  strRef = 91;  break;
	default:
		break;
	}

	if (strRef) {
		const Common::UString &localized = TalkMan.getString(strRef);
		if (!localized.empty())
			return localized;
	}

	switch (charClass) {
	case kClassJediGuardian:     return "Jedi Guardian";
	case kClassJediSentinel:     return "Jedi Sentinel";
	case kClassJediConsular:     return "Jedi Consular";
	case kClassJediWeaponMaster: return "Jedi Weapon Master";
	case kClassJediMaster:       return "Jedi Master";
	case kClassJediWatchMan:     return "Jedi Watchman";
	case kClassSithMarauder:     return "Sith Marauder";
	case kClassSithLord:         return "Sith Lord";
	case kClassSithAssassin:     return "Sith Assassin";
	default:
		return "";
	}
}

Common::UString formatAbilityModifier(int modifier) {
	if (modifier > 0)
		return "+" + Common::composeString(modifier);
	return Common::composeString(modifier);
}

Common::UString getForcePowerDisplayName(uint32_t powerId) {
	const ActionExecutor::SpellInfo *spell = ActionExecutor::getSpellInfo(powerId);
	if (spell && spell->nameStrRef) {
		const Common::UString &localized = TalkMan.getString(spell->nameStrRef);
		if (!localized.empty())
			return localized;
	}

	switch (powerId) {
	case 1: return "Force Heal";
	case 2: return "Force Push";
	case 3: return "Burst of Speed";
	case 4: return "Affect Mind";
	case 5: return "Force Stun";
	case 6: return "Force Armor";
	default:
		if (spell && !spell->label.empty())
			return spell->label;
		return Common::composeString(powerId);
	}
}

CharacterPreviewStats previewStatsAtLevel1(Class charClass, const CreatureInfo::Abilities &abilities) {
	CharacterPreviewStats stats;

	const int conMod = abilityModifier(static_cast<int>(abilities.constitution));
	const int dexMod = abilityModifier(static_cast<int>(abilities.dexterity));
	const int wisMod = abilityModifier(static_cast<int>(abilities.wisdom));

	int hp = classHitDie(charClass) + conMod;
	stats.vitality = (hp < 1) ? 1 : hp;
	stats.defense = 10 + dexMod;
	stats.fortitude = 11 + conMod;
	stats.reflex = 11 + dexMod;
	stats.will = 11 + wisMod;

	return stats;
}

std::vector<uint32_t> getSelectableForcePowers(const CreatureInfo &info) {
	const Class pcClass = info.getNumClasses() > 0 ? info.getLatestClass() : kClassSoldier;

	try {
		const Aurora::TwoDAFile &spells = TwoDAReg.get2DA("spells");

		CreatureInfo next(info);
		next.incrementClassLevel(pcClass);

		return selectableForcePowersFromTable(spells, pcClass, next);
	} catch (...) {
	}

	// No game data: the core powers.
	std::vector<uint32_t> powers;

	// Spell row indices used by ActionExecutor for core powers.
	static const uint32_t kCorePowers[] = { 1, 2, 3, 4, 5, 6 };
	for (uint32_t power : kCorePowers) {
		if (!info.hasForcePower(power))
			powers.push_back(power);
	}

	return powers;
}

/** The row of a per-level gain table for a class level: the row whose level
 *  column names it, else row (classLevel - 1). */
static const Aurora::TwoDARow &gainTableRow(const Aurora::TwoDAFile &twoda, int classLevel) {
	const Common::UString level = Common::composeString(classLevel);
	for (const char *column : { "label", "level" }) {
		if (twoda.headerToColumn(column) == Aurora::kFieldIDInvalid)
			continue;

		const Aurora::TwoDARow &row = twoda.getRow(column, level);
		if (!row.empty(column))
			return row;
	}

	return twoda.getRow(static_cast<size_t>(classLevel - 1));
}

int gainFromTable(const Aurora::TwoDAFile &twoda, const Common::UString &column, int classLevel) {
	if (column.empty() || classLevel < 1 || twoda.headerToColumn(column) == Aurora::kFieldIDInvalid)
		return -1;

	const Aurora::TwoDARow &row = gainTableRow(twoda, classLevel);
	if (row.empty(column))
		return 0;

	return MAX<int>(row.getInt(column), 0);
}

/** gainFromTable() on a game 2DA; -1 when it is unavailable. */
static int readGain(const char *table, const Common::UString &column, int classLevel) {
	if (column.empty() || classLevel < 1)
		return -1;

	try {
		return gainFromTable(TwoDAReg.get2DA(table), column, classLevel);
	} catch (...) {
		return -1;
	}
}

/** A column of the class's classes.2da row, or "" when unavailable. */
static Common::UString classTableString(Class charClass, const char *column) {
	try {
		const Aurora::TwoDARow &row = TwoDAReg.get2DA("classes").getRow(static_cast<size_t>(charClass));
		return row.getString(column);
	} catch (...) {
		return "";
	}
}

int featsGainedAtClassLevel(Class charClass, int classLevel) {
	if (classLevel < 1)
		return 0;

	const Common::UString code = classTableString(charClass, "featgain");
	const int gain = code.empty() ? -1 : readGain("featgain", code + "_reg", classLevel);
	if (gain >= 0)
		return gain;

	// No game data: one feat per level.
	return 1;
}

int forcePowersGainedAtClassLevel(Class charClass, int classLevel) {
	if (classLevel < 1)
		return 0;

	// The column is named by the class's spell gain table, or its feat gain code.
	for (const char *column : { "spellgaintable", "featgain" }) {
		const Common::UString code = classTableString(charClass, column);
		const int gain = readGain("classpowergain", code, classLevel);
		if (gain >= 0)
			return gain;
	}

	// No game data: one power per Jedi level.
	return isJediClass(charClass) ? 1 : 0;
}

static Class nextLevelClass(const CreatureInfo &info) {
	return (info.getNumClasses() > 0) ? info.getLatestClass() : kClassSoldier;
}

int featsForNextLevel(const CreatureInfo &info) {
	const Class charClass = nextLevelClass(info);
	return featsGainedAtClassLevel(charClass, info.getClassLevel(charClass) + 1);
}

int forcePowersForNextLevel(const CreatureInfo &info) {
	const Class charClass = nextLevelClass(info);
	return forcePowersGainedAtClassLevel(charClass, info.getClassLevel(charClass) + 1);
}

std::vector<uint32_t> completeLevelUpPicks(const std::vector<uint32_t> &chosen,
                                           const std::vector<uint32_t> &available, int count) {

	std::vector<uint32_t> picks;
	auto canPick = [&](uint32_t id) {
		return std::find(available.begin(), available.end(), id) != available.end() &&
		       std::find(picks.begin(), picks.end(), id) == picks.end();
	};

	for (uint32_t id : chosen)
		if (static_cast<int>(picks.size()) < count && canPick(id))
			picks.push_back(id);

	for (uint32_t id : available)
		if (static_cast<int>(picks.size()) < count && canPick(id))
			picks.push_back(id);

	return picks;
}

void applyDefaultChargenBuild(CharacterGenerationInfo &info) {
	// Unspent level 1 feat picks go to the recommended feats.
	const int picks = featsGainedAtClassLevel(info.getClass(), 1) - static_cast<int>(info.getFeats().size());
	if (picks > 0)
		for (uint32_t feat : completeLevelUpPicks({}, getSelectableFeats(info, info.getFeats()), picks))
			info.addFeat(feat);

	// The feats the class grants at level 1 (proficiencies and the like).
	for (uint32_t feat : classGrantedFeats(info.getClass(), 1))
		if (std::find(info.getFeats().begin(), info.getFeats().end(), feat) == info.getFeats().end())
			info.addFeat(feat);

	if (totalSkillRanks(info.getSkills()) > 0)
		return;

	int points = chargenSkillPointBudget(info);
	const Class pcClass = info.getClass();

	while (points > 0) {
		Skill bestSkill = kSkillComputerUse;
		int bestRank = -1;
		bool bestIsClassSkill = false;

		for (int i = 0; i < kSkillMAX; ++i) {
			const Skill skill = static_cast<Skill>(i);
			const int rank = chargenSkillRank(info, skill);
			const bool classSkill = isClassSkill(pcClass, skill);

			if (rank > bestRank || (rank == bestRank && classSkill && !bestIsClassSkill)) {
				bestRank = rank;
				bestSkill = skill;
				bestIsClassSkill = classSkill;
			}
		}

		const int cost = isClassSkill(pcClass, bestSkill) ? 1 : 2;
		if (points < cost)
			break;

		info.setSkillRank(bestSkill, chargenSkillRank(info, bestSkill) + 1);
		points -= cost;
	}
}

void autoLevelUp(Creature &creature) {
	CreatureInfo &info = creature.getCreatureInfo();
	const Class pcClass = info.getLatestClass();

	int points = skillPointsPerLevel(info);
	while (points > 0) {
		Skill bestSkill = kSkillComputerUse;
		int bestRank = -1;

		for (int i = 0; i < kSkillMAX; ++i) {
			const Skill skill = static_cast<Skill>(i);
			const int rank = info.getSkillRank(skill);
			if (rank > bestRank) {
				bestRank = rank;
				bestSkill = skill;
			}
		}

		const int cost = isClassSkill(pcClass, bestSkill) ? 1 : 2;
		if (points < cost)
			break;

		info.setSkillRank(bestSkill, info.getSkillRank(bestSkill) + 1);
		points -= cost;
	}

	// Take the recommended (first) feats and powers the level grants.
	for (uint32_t feat : completeLevelUpPicks({}, getSelectableFeats(info), featsForNextLevel(info)))
		info.addFeat(feat);

	for (uint32_t power : completeLevelUpPicks({}, getSelectableForcePowers(info), forcePowersForNextLevel(info)))
		info.addForcePower(power);

	applyLevelUp(creature);
}

} // End of namespace KotORBase

} // End of namespace Engines
