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

#include "src/aurora/talkman.h"

#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/creatureinfo.h"

#include "src/engines/kotorbase/gui/gui.h"

namespace Engines {

namespace KotORBase {

static Common::UString classDisplayName(Class charClass) {
	uint32_t strRef = 0;

	switch (charClass) {
	case kClassSoldier:   strRef = 134; break;
	case kClassScout:     strRef = 133; break;
	case kClassScoundrel: strRef = 135; break;
	default:
		break;
	}

	if (strRef)
		return TalkMan.getString(strRef);

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

static void setStatPair(GUI &gui, const char *primaryTag, const char *legacyTag, const Common::UString &text) {
	gui.setWidgetText(primaryTag, text);
	if (legacyTag)
		gui.setWidgetText(legacyTag, text);
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
			setWidgetText("LBL_CLASS" + index, classDisplayName(info.getClassByPosition(i)));
			setWidgetText("LBL_LEVEL" + index, Common::composeString(info.getLevelByPosition(i)));
		} else {
			setWidgetText("LBL_CLASS" + index, "");
			setWidgetText("LBL_LEVEL" + index, "");
		}
	}

	if (numClasses == 1)
		setWidgetText("LBL_CLASS", classDisplayName(info.getClassByPosition(0)));
	else if (numClasses == 0)
		setWidgetText("LBL_CLASS", "");

	const Common::UString vitality = Common::composeString(creature.getCurrentHitPoints())
	                               + "/" + Common::composeString(creature.getMaxHitPoints());
	setStatPair(*this, "LBL_VIT_VAL", "VIT_VAL_LBL", vitality);
	setStatPair(*this, "LBL_DEF_VAL", "DEF_VAL_LBL", Common::composeString(creature.getAC()));

	const int fort = 10 + creature.getSavingThrowBonus(kSavingThrowFortitude);
	const int refl = 10 + creature.getSavingThrowBonus(kSavingThrowReflex);
	const int will = 10 + creature.getSavingThrowBonus(kSavingThrowWill);

	setStatPair(*this, "LBL_FORT_VAL", "FORT_VAL_LBL", Common::composeString(fort));
	setStatPair(*this, "LBL_REFL_VAL", "REFL_VAL_LBL", Common::composeString(refl));
	setStatPair(*this, "LBL_WILL_VAL", "WILL_VAL_LBL", Common::composeString(will));
}

void GUI::populateAbilitiesSheet(Creature &creature) {
	const CreatureInfo &info = creature.getCreatureInfo();

	static const struct {
		Ability ability;
		const char *pointTag;
		const char *legacyTag;
	} kAbilityTags[] = {
		{ kAbilityStrength,     "STR_POINTS_BTN", "BTN_STR_PLUS"  },
		{ kAbilityDexterity,    "DEX_POINTS_BTN", "BTN_DEX_PLUS"  },
		{ kAbilityConstitution, "CON_POINTS_BTN", "BTN_CON_PLUS"  },
		{ kAbilityIntelligence, "INT_POINTS_BTN", "BTN_INT_PLUS"  },
		{ kAbilityWisdom,       "WIS_POINTS_BTN", "BTN_WIS_PLUS"  },
		{ kAbilityCharisma,     "CHA_POINTS_BTN", "BTN_CHA_PLUS"  },
	};

	for (size_t i = 0; i < ARRAYSIZE(kAbilityTags); ++i) {
		const Common::UString score = Common::composeString(info.getAbilityScore(kAbilityTags[i].ability));
		setStatPair(*this, kAbilityTags[i].pointTag, kAbilityTags[i].legacyTag, score);
	}
}

} // End of namespace KotORBase

} // End of namespace Engines
