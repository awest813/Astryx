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
 *  The skill allocation menu for character level-up.
 */

#include "src/common/strutil.h"

#include "src/engines/odyssey/button.h"
#include "src/engines/odyssey/label.h"

#include "src/engines/kotorbase/levelup.h"

#include "src/engines/kotor/gui/ingame/levelup_skills.h"

namespace Engines {

namespace KotOR {

struct SkillWidgetTags {
	const char *pointTag;
	const char *plusTag;
	const char *minusTag;
	const char *legacyPointTag;
	const char *legacyPlusTag;
	const char *legacyMinusTag;
};

static const SkillWidgetTags kSkillTags[] = {
	{ "COMPUTER_USE_POINTS_BTN", "COM_PLUS_BTN", "COM_MINUS_BTN", "LBL_COMP_USE",    "BTN_COMP_USE_PLUS",    "BTN_COMP_USE_MINUS"    },
	{ "DEMOLITIONS_POINTS_BTN",  "DEM_PLUS_BTN", "DEM_MINUS_BTN", "LBL_DEMOLITIONS", "BTN_DEMOLITIONS_PLUS", "BTN_DEMOLITIONS_MINUS" },
	{ "STEALTH_POINTS_BTN",      "STE_PLUS_BTN", "STE_MINUS_BTN", "LBL_STEALTH",     "BTN_STEALTH_PLUS",     "BTN_STEALTH_MINUS"     },
	{ "AWARENESS_POINTS_BTN",    "AWA_PLUS_BTN", "AWA_MINUS_BTN", "LBL_AWARENESS",   "BTN_AWARENESS_PLUS",   "BTN_AWARENESS_MINUS"   },
	{ "PERSUADE_POINTS_BTN",     "PER_PLUS_BTN", "PER_MINUS_BTN", "LBL_PERSUADE",    "BTN_PERSUADE_PLUS",    "BTN_PERSUADE_MINUS"    },
	{ "REPAIR_POINTS_BTN",       "REP_PLUS_BTN", "REP_MINUS_BTN", "LBL_REPAIR",      "BTN_REPAIR_PLUS",      "BTN_REPAIR_MINUS"      },
	{ "SECURITY_POINTS_BTN",     "SEC_PLUS_BTN", "SEC_MINUS_BTN", "LBL_SECURITY",    "BTN_SECURITY_PLUS",    "BTN_SECURITY_MINUS"    },
	{ "TREAT_INJURY_POINTS_BTN", "TRE_PLUS_BTN", "TRE_MINUS_BTN", "LBL_TREAT_INJ",   "BTN_TREAT_INJ_PLUS",   "BTN_TREAT_INJ_MINUS"   },
};

LevelUpSkillsMenu::LevelUpSkillsMenu(KotORBase::CreatureInfo &info, Console *console) :
		KotORBase::GUI(console),
		_info(info),
		_accepted(false) {

	try {
		load("skchrgen");
	} catch (...) {
		load("skilitems");
	}

	addBackground(KotORBase::kBackgroundTypeMenu);

	for (int i = 0; i < KotORBase::kSkillMAX; ++i)
		_ranks[i] = _originalRanks[i] = info.getSkillRank(static_cast<KotORBase::Skill>(i));

	_remainingPoints = computeAvailablePoints();

	updateLabels();
}

LevelUpSkillsMenu::~LevelUpSkillsMenu() {
}

int LevelUpSkillsMenu::computeAvailablePoints() const {
	int base = 1;
	if (_info.getNumClasses() > 0) {
		switch (_info.getLatestClass()) {
		case KotORBase::kClassScout:         base = 2; break;
		case KotORBase::kClassScoundrel:     base = 3; break;
		case KotORBase::kClassJediSentinel:  base = 2; break;
		case KotORBase::kClassExpertDroid:   base = 4; break;
		default: base = 1; break;
		}
	}

	const int total = base + _info.getAbilityModifier(KotORBase::kAbilityIntelligence);
	return (total < 1) ? 1 : total;
}

void LevelUpSkillsMenu::updateLabels() {
	for (int i = 0; i < KotORBase::kSkillMAX; ++i) {
		const Common::UString rank = Common::composeString(_ranks[i]);
		setWidgetText(kSkillTags[i].pointTag, rank);
		setWidgetText(kSkillTags[i].legacyPointTag, rank);
	}

	const Common::UString remaining = Common::composeString(_remainingPoints);
	setWidgetText("REMAINING_SELECTIONS_LBL", remaining);
	setWidgetText("SELECTIONS_REMAINING_LBL", remaining);
}

void LevelUpSkillsMenu::callbackActive(Widget &widget) {
	const Common::UString &tag = widget.getTag();
	const KotORBase::Class pcClass = _info.getLatestClass();

	for (int i = 0; i < KotORBase::kSkillMAX; ++i) {
		if (tag == kSkillTags[i].plusTag || tag == kSkillTags[i].legacyPlusTag) {
			const int cost = KotORBase::isClassSkill(pcClass, static_cast<KotORBase::Skill>(i)) ? 1 : 2;

			if (_remainingPoints >= cost) {
				_ranks[i]++;
				_remainingPoints -= cost;
				updateLabels();
			}
			return;
		}
		if (tag == kSkillTags[i].minusTag || tag == kSkillTags[i].legacyMinusTag) {
			if (_ranks[i] > _originalRanks[i]) {
				const int cost = KotORBase::isClassSkill(pcClass, static_cast<KotORBase::Skill>(i)) ? 1 : 2;

				_ranks[i]--;
				_remainingPoints += cost;
				updateLabels();
			}
			return;
		}
	}

	if (tag == "BTN_BACK") {
		_returnCode = 1;
		return;
	}

	if (tag == "BTN_ACCEPT") {
		if (_remainingPoints == 0) {
			for (int i = 0; i < KotORBase::kSkillMAX; ++i)
				_info.setSkillRank(static_cast<KotORBase::Skill>(i), _ranks[i]);

			_accepted = true;
			_returnCode = 1;
		}
		return;
	}
}

} // End of namespace KotOR

} // End of namespace Engines
