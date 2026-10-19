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

#ifndef ENGINES_KOTORBASE_TALENT_H
#define ENGINES_KOTORBASE_TALENT_H

#include <vector>

#include "src/common/ustring.h"

#include "src/aurora/nwscript/enginetype.h"

namespace Aurora {
	class TwoDAFile;
}

namespace Engines {

namespace KotORBase {

class Creature;
class Object;
struct Action;

/** TALENT_TYPE_* from nwscript.nss. */
enum TalentType {
	kTalentTypeInvalid = -1,
	kTalentTypeSpell   =  0, ///< TALENT_TYPE_FORCE / TALENT_TYPE_SPELL.
	kTalentTypeFeat    =  1, ///< TALENT_TYPE_FEAT.
	kTalentTypeSkill   =  2  ///< TALENT_TYPE_SKILL.
};

/** TALENT_EXCLUDE_ALL_OF_TYPE: as nExcludeId, ignore every talent of nExcludeType. */
static const int kTalentExcludeAllOfType = -1;

class Talent : public Aurora::NWScript::EngineType {
public:
	/** @param item Template of the item that grants this talent (an item's Force power). */
	Talent(TalentType type = kTalentTypeInvalid, int id = -1,
	       const Common::UString &item = Common::UString());

	bool isValid() const;

	TalentType getType() const;
	int getID() const;
	/** The "category" bitmask from spells.2da, feat.2da or skills.2da, or -1. */
	int getCategory() const;
	/** Template of the item that grants this talent, empty for the creature's own talents. */
	const Common::UString &getItem() const;

	Aurora::NWScript::EngineType *clone() const;

private:
	TalentType _type;
	int _id;
	Common::UString _item;
};

/** A talent a creature can use, with its 2DA category and challenge rating. */
struct TalentEntry {
	TalentType type { kTalentTypeInvalid };
	int id { -1 };
	int category { -1 };
	int maxCR { 0 };
	Common::UString item; ///< Granting item's template, empty for the creature's own talents.
};

/** The 2DAs describing talents. A missing table means no talent of that type has a category. */
struct TalentTables {
	const Aurora::TwoDAFile *spells { nullptr };
	const Aurora::TwoDAFile *feats { nullptr };
	const Aurora::TwoDAFile *skills { nullptr };

	/** spells.2da, feat.2da and skills.2da from the game data, where they can be loaded. */
	static TalentTables fromGameData();

	const Aurora::TwoDAFile *forType(TalentType type) const;
};

/** The row's "category" bitmask (hex in the 2DA, e.g. 0x1104), or -1 when it has none. */
int getTalentCategory(const Aurora::TwoDAFile &table, int id);
/** The row's "maxcr" (challenge rating of the talent), or 0. */
int getTalentMaxCR(const Aurora::TwoDAFile &table, int id);

/** Does a talent of @p talentCategory belong to the requested TALENT_CATEGORY_* bitmask? */
bool isTalentInCategory(int talentCategory, int category);

/** Force points a power costs, given its spells.2da "goodevil" ('G', 'E' or '-') and the
 *  caster's alignment (0 dark .. 100 light). Powers of the side opposite to the
 *  caster's cost half again as much. */
int getForcePowerCost(int baseCost, const Common::UString &goodEvil, int casterAlignment);

/** A power that reaches as far as needed: it needs no walk to the target. */
static const float kSpellRangeUnlimited = -1.0f;

/** How far (m) a power with the spells.2da "range" code @p code reaches.
 *  The codes are S(hort), M(edium), L(ong), (thro)W, P(ersonal) and T(ouch);
 *  short, medium and long are the "primaryrange" of the SpellRngShrt/Med/Long
 *  rows of @p ranges (ranges.2da), falling back to KotOR.js' distances without
 *  the table. Personal and touch powers, as in KotOR.js, and unknown codes are
 *  kSpellRangeUnlimited. */
float getSpellRangeFromTable(const Common::UString &code, const Aurora::TwoDAFile *ranges);
/** The spells.2da "range" code of power @p id, or an empty string. */
Common::UString getSpellRangeCode(const Aurora::TwoDAFile &spells, int id);

/** Every talent @p creature can use right now: known Force powers it has the Force
 *  points for, Force powers of equipped items, spells of usable inventory items,
 *  feats and trained skills, each with its category and challenge rating. */
std::vector<TalentEntry> getCreatureTalents(const Creature &creature, const TalentTables &tables);

/** GetCreatureTalentBest: the talent in @p category whose challenge rating is the
 *  closest to @p crMax without going over. Talents of type @p excludeType (all of
 *  them for kTalentExcludeAllOfType, otherwise only id @p excludeId) are skipped.
 *  Returns nullptr when nothing qualifies. */
const TalentEntry *findBestTalent(const std::vector<TalentEntry> &talents, int category, int crMax,
                                  int excludeType = -1, int excludeId = -1);

/** All talents of @p category, for GetCreatureTalentRandom. */
std::vector<const TalentEntry *> findTalentsInCategory(const std::vector<TalentEntry> &talents,
                                                       int category);

/** Does @p creature have @p talent (GetCreatureHasTalent)? A spell talent counts
 *  when the creature knows the power or one of its items casts it. */
bool creatureHasTalent(const Creature &creature, const Talent &talent);

/** The kActionCastSpell that uses spell talent @p talent on @p target. A power the
 *  caster knows costs Force points; an item's spell is free (bCheat) and uses up
 *  the item when it comes from the inventory rather than the equipment. */
Action makeTalentSpellAction(const Creature &caster, const Talent &talent, Object *target);

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_TALENT_H
