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
#include <vector>

#include "src/common/util.h"

#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/creatureinfo.h"
#include "src/engines/kotorbase/levelup.h"

namespace Engines {

namespace KotORBase {

int levelUpThreshold(int currentLevel) {
	if (currentLevel < 1)
		currentLevel = 1;

	return currentLevel * (currentLevel + 1) / 2 * 1000;
}

bool canLevelUp(const Creature &creature) {
	const int level = creature.getHitDice();
	return creature.getCurrentXP() >= levelUpThreshold(level);
}

int classHitDie(Class charClass) {
	switch (charClass) {
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

	const int hpGain = hpGainOnLevelUp(pcClass,
	                                   info.getAbilityModifier(kAbilityConstitution));
	creature.setMaxHitPoints(creature.getMaxHitPoints() + hpGain);
	creature.setCurrentHitPoints(creature.getMaxHitPoints());

	creature.setMaxForcePoints(creature.computeMaxForcePoints());
	creature.setForcePoints(creature.getMaxForcePoints());

	status("Level up applied for %s. New HitDice: %d", creature.getName().c_str(), creature.getHitDice());
}

bool isClassSkill(Class c, Skill s) {
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

static int skillPointsPerLevel(const CreatureInfo &info) {
	int base = 1;
	if (info.getNumClasses() > 0) {
		switch (info.getLatestClass()) {
		case kClassScout:        base = 2; break;
		case kClassScoundrel:    base = 3; break;
		case kClassJediSentinel: base = 2; break;
		case kClassExpertDroid:  base = 4; break;
		default:                 base = 1; break;
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
	switch (pcClass) {
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

std::vector<uint32_t> getSelectableFeats(const CreatureInfo &info) {
	const Class pcClass = info.getNumClasses() > 0 ? info.getLatestClass() : kClassSoldier;
	return getSelectableFeats(pcClass, info.getFeats());
}

static int abilityModifier(int score) {
	const int mod = score - 10;
	if (mod >= 0)
		return mod / 2;
	return (mod - 1) / 2;
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
	std::vector<uint32_t> powers;

	// Spell row indices used by ActionExecutor for core powers.
	static const uint32_t kCorePowers[] = { 1, 2, 3, 4, 5, 6 };
	for (uint32_t power : kCorePowers) {
		if (!info.hasForcePower(power))
			powers.push_back(power);
	}

	return powers;
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

	// Grant a sensible default combat feat when the auto path is used.
	if (!info.hasFeat(kFeatPowerAttack))
		info.addFeat(kFeatPowerAttack);
	else if (!info.hasFeat(kFeatFlurry))
		info.addFeat(kFeatFlurry);

	if (info.isJedi() && info.getForcePowers().empty())
		info.addForcePower(1); // Force Heal

	applyLevelUp(creature);
}

} // End of namespace KotORBase

} // End of namespace Engines
