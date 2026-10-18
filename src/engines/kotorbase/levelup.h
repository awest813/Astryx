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

#include "src/common/ustring.h"

#include "src/engines/kotorbase/creatureinfo.h"
#include "src/engines/kotorbase/types.h"

namespace Aurora {
	class TwoDAFile;
}

namespace Engines {

namespace KotORBase {

class CharacterGenerationInfo;
class Creature;
class CreatureInfo;

/** XP required to reach the next level from @p currentLevel (1-based): exptable.2da,
 *  else the d20 progression. Past the end of exptable.2da (level cap), INT_MAX. */
int levelUpThreshold(int currentLevel);

/** exptable.2da's "xp" for level @p currentLevel + 1: the row whose "level" column
 *  names that level, else row @p currentLevel. -1 when the table has no such row. */
int levelUpThresholdFromTable(const Aurora::TwoDAFile &exptable, int currentLevel);

/** True when @p creature has enough XP for another class level. */
bool canLevelUp(const Creature &creature);

/** XP for killing a creature of challenge rating @p challengeRating, from
 *  xptable.2da: column "cr<N>" (clamped to the highest one), row @p partyLevel
 *  (or the row whose "level"/"label" column names it; clamped to the last row).
 *  -1 when the table has no CR columns. */
int killXPFromTable(const Aurora::TwoDAFile &xptable, int partyLevel, int challengeRating);

/** XP the party earns for a kill: xptable.2da, else 25 x the challenge rating. */
int killExperience(int partyLevel, int challengeRating);

/** Hit die for a base class (d6 / d8 / d10). */
int classHitDie(Class charClass);

/** HP gained on level-up: floor(hitDie/2) + 1 + CON mod, minimum 1. */
int hpGainOnLevelUp(Class charClass, int constitutionModifier);

/** Apply a completed level-up (class level, HP, force points). */
void applyLevelUp(Creature &creature);

/** Story/script Jedi conversion: add class level, Jedi Defense, and refresh FP pool. */
void applyJediClass(Creature &creature, Class jediClass);

/** Fill in default feat and skill picks for quick character creation. */
void applyDefaultChargenBuild(CharacterGenerationInfo &info);

/** Auto-distribute skill points and a default feat when manual GUI is skipped. */
void autoLevelUp(Creature &creature);

/** True when levelling from @p currentLevel grants an ability score increase. */
bool grantsAbilityIncrease(int currentLevel);

/** Feats the creature may pick on the next level (excludes ones already known),
 *  recommended ones first. From feat.2da, else a small built-in list. */
std::vector<uint32_t> getSelectableFeats(const CreatureInfo &info);

/** Feats available during character creation (level 1 of the chosen class),
 *  excluding @p knownFeats; recommended ones first. */
std::vector<uint32_t> getSelectableFeats(const CharacterGenerationInfo &info,
                                         const std::vector<uint32_t> &knownFeats);

/** The built-in feat list for a class, used without feat.2da. */
std::vector<uint32_t> getSelectableFeats(Class charClass, const std::vector<uint32_t> &knownFeats);

/** Force powers a Jedi may pick on the next level (excludes known powers),
 *  recommended ones first. From spells.2da, else the six core powers. */
std::vector<uint32_t> getSelectableForcePowers(const CreatureInfo &info);

/** The feat.2da column prefix of a class: classes.2da "featstable", lower-cased
 *  (e.g. "sol", "jcn"). Empty without game data. */
Common::UString classFeatTableCode(Class charClass);

/** Feats feat.2da grants a class automatically ("<code>_list" 3) on reaching
 *  @p classLevel ("<code>_granted"). */
std::vector<uint32_t> grantedFeatsFromTable(const Aurora::TwoDAFile &feats,
                                            const Common::UString &code, int classLevel);

/** grantedFeatsFromTable() on the game's feat.2da; empty without game data. */
std::vector<uint32_t> classGrantedFeats(Class charClass, int classLevel);

/** Feats a class may pick from feat.2da: "<code>_list" 0 or 1, not in @p knownFeats,
 *  with all requirements (prereqfeat1/2, orreqfeat0-4, mincharlevel, minattackbonus,
 *  min<ability>, minlevel/minlevelclass, reqskill) met by @p next, the character as it
 *  will be after the level. Feats with a "<code>_recom" rank come first, by rank. */
std::vector<uint32_t> selectableFeatsFromTable(const Aurora::TwoDAFile &feats, const Common::UString &code,
                                               const std::vector<uint32_t> &knownFeats,
                                               const CreatureInfo &next);

/** The spells.2da column holding the level a Jedi class may learn a power at
 *  ("guardian", "consular", "sentinel", ...); empty for non-Force classes. */
Common::UString forcePowerClassColumn(Class charClass);

/** Force powers @p charClass may learn from spells.2da: player powers ("usertype" 1)
 *  whose class column is at most @p next's character level, not yet known, with all
 *  "prerequisites" (underscore-separated spell rows) known. Powers ranked by
 *  "light_recom" ("dark_recom" for dark side characters) come first. */
std::vector<uint32_t> selectableForcePowersFromTable(const Aurora::TwoDAFile &spells, Class charClass,
                                                     const CreatureInfo &next);

/** Localized (or 2DA label) name of a feat.2da row. */
Common::UString getFeatDisplayName(uint32_t feat);

/** A class level's entry in a per-level gain table (featgain.2da, classpowergain.2da):
 *  the row whose "label"/"level" column holds @p classLevel, else row classLevel - 1.
 *  Returns -1 when the column doesn't exist. */
int gainFromTable(const Aurora::TwoDAFile &twoda, const Common::UString &column, int classLevel);

/** Feats @p charClass gains on reaching @p classLevel (featgain.2da "<code>_reg"). */
int featsGainedAtClassLevel(Class charClass, int classLevel);

/** Force powers @p charClass gains on reaching @p classLevel (classpowergain.2da). */
int forcePowersGainedAtClassLevel(Class charClass, int classLevel);

/** Feats the next level-up of @p info grants. */
int featsForNextLevel(const CreatureInfo &info);

/** Force powers the next level-up of @p info grants. */
int forcePowersForNextLevel(const CreatureInfo &info);

/** The final picks for a level-up step: @p chosen (only entries in @p available,
 *  without duplicates, at most @p count), topped up with the first remaining
 *  entries of @p available until @p count are picked or none remain. */
std::vector<uint32_t> completeLevelUpPicks(const std::vector<uint32_t> &chosen,
                                           const std::vector<uint32_t> &available, int count);

/** True when @p skill is a class skill for @p charClass. */
bool isClassSkill(Class charClass, Skill skill);

/** Skill points granted on the next level-up for @p info. */
int skillPointsPerLevel(const CreatureInfo &info);

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

/** Localized (or fallback English) label for a base/Jedi class. */
Common::UString getClassDisplayName(Class charClass);

/** Localized (or fallback English) label for a Force power spell row. */
Common::UString getForcePowerDisplayName(uint32_t powerId);

/** Format an ability modifier for GUI display (e.g. "+3", "-1", "0"). */
Common::UString formatAbilityModifier(int modifier);

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_LEVELUP_H
