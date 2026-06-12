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
 *  Shared population logic for KotOR character profile menus.
 */

#include "src/common/util.h"
#include "src/common/strutil.h"

#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/creatureinfo.h"
#include "src/engines/kotorbase/levelup.h"

#include "src/engines/kotorbase/gui/gui.h"
#include "src/engines/kotorbase/gui/guiskilltags.h"

namespace Engines {

namespace KotORBase {

void GUI::setStatPair(const char *primaryTag, const char *legacyTag, const Common::UString &text) {
	setWidgetText(primaryTag, text);
	if (legacyTag)
		setWidgetText(legacyTag, text);
}

void GUI::setStatPairMany(const char *const *tags, size_t count, const Common::UString &text) {
	for (size_t i = 0; i < count; ++i) {
		if (tags[i])
			setWidgetText(tags[i], text);
	}
}

void GUI::populateCharacterSheet(Creature &creature) {
	const CreatureInfo &info = creature.getCreatureInfo();
	const Common::UString levelText = Common::composeString(creature.getHitDice());

	setWidgetText("LBL_NAME", creature.getName());
	setWidgetText("LBL_LEVEL_VAL", levelText);
	setWidgetText("LBL_LEVEL", levelText);

	const int numClasses = info.getNumClasses();
	for (int i = 0; i < 2; ++i) {
		const Common::UString index = Common::composeString(i + 1);
		if (i < numClasses) {
			setWidgetText("LBL_CLASS" + index, getClassDisplayName(info.getClassByPosition(i)));
			setWidgetText("LBL_LEVEL" + index, Common::composeString(info.getLevelByPosition(i)));
		} else {
			setWidgetText("LBL_CLASS" + index, "");
			setWidgetText("LBL_LEVEL" + index, "");
		}
	}

	if (numClasses == 1)
		setWidgetText("LBL_CLASS", getClassDisplayName(info.getClassByPosition(0)));
	else if (numClasses == 0)
		setWidgetText("LBL_CLASS", "");

	const Common::UString vitality = Common::composeString(creature.getCurrentHitPoints())
	                               + "/" + Common::composeString(creature.getMaxHitPoints());
	setStatPair("LBL_VIT_VAL", "VIT_VAL_LBL", vitality);
	setStatPair("LBL_DEF_VAL", "DEF_VAL_LBL", Common::composeString(creature.getAC()));

	const int fort = 10 + creature.getSavingThrowBonus(kSavingThrowFortitude);
	const int refl = 10 + creature.getSavingThrowBonus(kSavingThrowReflex);
	const int will = 10 + creature.getSavingThrowBonus(kSavingThrowWill);

	setStatPair("LBL_FORT_VAL", "FORT_VAL_LBL", Common::composeString(fort));
	setStatPair("LBL_REFL_VAL", "REFL_VAL_LBL", Common::composeString(refl));
	setStatPair("LBL_WILL_VAL", "WILL_VAL_LBL", Common::composeString(will));

	const Common::UString xp = Common::composeString(creature.getCurrentXP());
	setStatPairMany((const char *const[]){ "LBL_XP_VAL", "XP_VAL_LBL", "LBL_XP" }, 3, xp);

	if (info.isJedi()) {
		const Common::UString forcePoints = Common::composeString(creature.getForcePoints())
		                                  + "/" + Common::composeString(creature.getMaxForcePoints());
		setStatPairMany((const char *const[]){ "LBL_FORCE_VAL", "FORCE_VAL_LBL", "FP_VAL_LBL", "LBL_FORCE" }, 4,
			forcePoints);
	}

	setWidgetText("LBL_BAB_VAL", Common::composeString(creature.getBAB()));
	setWidgetText("BAB_VAL_LBL", Common::composeString(creature.getBAB()));
}

void GUI::populateAbilitiesSheet(Creature &creature) {
	const CreatureInfo &info = creature.getCreatureInfo();

	static const struct {
		Ability ability;
		const char *pointTag;
		const char *valTag;
		const char *modTag;
		const char *legacyPointTag;
	} kAbilityTags[] = {
		{ kAbilityStrength,     "STR_POINTS_BTN", "STR_VAL_BTN", "STR_MOD_BTN", "BTN_STR_PLUS"  },
		{ kAbilityDexterity,    "DEX_POINTS_BTN", "DEX_VAL_BTN", "DEX_MOD_BTN", "BTN_DEX_PLUS"  },
		{ kAbilityConstitution, "CON_POINTS_BTN", "CON_VAL_BTN", "CON_MOD_BTN", "BTN_CON_PLUS"  },
		{ kAbilityIntelligence, "INT_POINTS_BTN", "INT_VAL_BTN", "INT_MOD_BTN", "BTN_INT_PLUS"  },
		{ kAbilityWisdom,       "WIS_POINTS_BTN", "WIS_VAL_BTN", "WIS_MOD_BTN", "BTN_WIS_PLUS"  },
		{ kAbilityCharisma,     "CHA_POINTS_BTN", "CHA_VAL_BTN", "CHA_MOD_BTN", "BTN_CHA_PLUS"  },
	};

	for (size_t i = 0; i < ARRAYSIZE(kAbilityTags); ++i) {
		const Common::UString score = Common::composeString(info.getAbilityScore(kAbilityTags[i].ability));
		const Common::UString mod = formatAbilityModifier(info.getAbilityModifier(kAbilityTags[i].ability));

		setStatPairMany((const char *const[]){ kAbilityTags[i].pointTag, kAbilityTags[i].valTag, kAbilityTags[i].legacyPointTag },
			3, score);
		setWidgetText(kAbilityTags[i].modTag, mod);
	}

	for (size_t i = 0; i < kSkillWidgetTagCount; ++i) {
		const Common::UString rank = Common::composeString(info.getSkillRank(kSkillWidgetTags[i].skill));
		setStatPair(kSkillWidgetTags[i].pointTag, kSkillWidgetTags[i].legacyPointTag, rank);
	}
}

} // End of namespace KotORBase

} // End of namespace Engines
