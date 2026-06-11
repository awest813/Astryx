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

#ifndef ENGINES_KOTORBASE_LEVELUP_H
#define ENGINES_KOTORBASE_LEVELUP_H

#include <vector>
#include <cstdint>

#include "src/engines/kotorbase/creatureinfo.h"
#include "src/engines/kotorbase/types.h"

namespace Engines {

namespace KotORBase {

class Creature;
class CreatureInfo;

/** XP required to reach the next level from @p currentLevel (1-based). */
int levelUpThreshold(int currentLevel);

/** True when @p creature has enough XP for another class level. */
bool canLevelUp(const Creature &creature);

/** Hit die for a base class (d6 / d8 / d10). */
int classHitDie(Class charClass);

/** HP gained on level-up: floor(hitDie/2) + 1 + CON mod, minimum 1. */
int hpGainOnLevelUp(Class charClass, int constitutionModifier);

/** Apply a completed level-up (class level, HP, force points). */
void applyLevelUp(Creature &creature);

/** Auto-distribute skill points and a default feat when manual GUI is skipped. */
void autoLevelUp(Creature &creature);

/** True when levelling from @p currentLevel grants an ability score increase. */
bool grantsAbilityIncrease(int currentLevel);

/** Feats the creature may pick on the next level (excludes ones already known). */
std::vector<uint32_t> getSelectableFeats(const CreatureInfo &info);

/** Feats available during character creation for a class. */
std::vector<uint32_t> getSelectableFeats(Class charClass, const std::vector<uint32_t> &knownFeats);

/** Force powers a Jedi may pick on the next level (excludes known powers). */
std::vector<uint32_t> getSelectableForcePowers(const CreatureInfo &info);

/** True when @p skill is a class skill for @p charClass. */
bool isClassSkill(Class charClass, Skill skill);

/** Derived combat numbers shown on the character creation summary panel. */
struct CharacterPreviewStats {
	int vitality { 0 };
	int defense { 0 };
	int fortitude { 0 };
	int reflex { 0 };
	int will { 0 };
};

/** Preview HP and saving throws at level 1 from ability scores. */
CharacterPreviewStats previewStatsAtLevel1(Class charClass, const CreatureInfo::Abilities &abilities);

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_LEVELUP_H
