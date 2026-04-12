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

#include "src/engines/kotor/gui/ingame/levelup_skills.h"

namespace Engines {

namespace KotOR {

struct SkillWidgetTags {
	const char *pointTag;
	const char *plusTag;
	const char *minusTag;
};

static const SkillWidgetTags kSkillTags[] = {
	{ "COMPUTER_USE_POINTS_BTN", "COM_PLUS_BTN", "COM_MINUS_BTN" }, // kSkillComputerUse
	{ "DEMOLITIONS_POINTS_BTN",  "DEM_PLUS_BTN", "DEM_MINUS_BTN" }, // kSkillDemolitions
	{ "STEALTH_POINTS_BTN",      "STE_PLUS_BTN", "STE_MINUS_BTN" }, // kSkillStealth
	{ "AWARENESS_POINTS_BTN",    "AWA_PLUS_BTN", "AWA_MINUS_BTN" }, // kSkillAwareness
	{ "PERSUADE_POINTS_BTN",     "PER_PLUS_BTN", "PER_MINUS_BTN" }, // kSkillPersuade
	{ "REPAIR_POINTS_BTN",       "REP_PLUS_BTN", "REP_MINUS_BTN" }, // kSkillRepair
	{ "SECURITY_POINTS_BTN",     "SEC_PLUS_BTN", "SEC_MINUS_BTN" }, // kSkillSecurity
	{ "TREAT_INJURY_POINTS_BTN", "TRE_PLUS_BTN", "TRE_MINUS_BTN" }, // kSkillTreatInjury
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
	// Base points per level based on the class being levelled (assume first class for now).
	int base = 1;
	if (_info.getNumClasses() > 0) {
		KotORBase::Class pcClass = _info.getClassByPosition(0);
		switch (pcClass) {
			case KotORBase::kClassScout:         base = 2; break;
			case KotORBase::kClassScoundrel:     base = 3; break;
			case KotORBase::kClassJediSentinel:  base = 2; break;
			case KotORBase::kClassExpertDroid:   base = 4; break;
			default: base = 1; break;
		}
	}

	int intMod = _info.getAbilityModifier(KotORBase::kAbilityIntelligence);
	int total = base + intMod;

	// Minimum 1 point per level.
	return (total < 1) ? 1 : total;
}

void LevelUpSkillsMenu::updateLabels() {
	auto setWidgetText = [this](const char *tag, const Common::UString &text) {
		Odyssey::WidgetLabel *lbl = getLabel(tag);
		if (lbl) {
			lbl->setText(text);
			return;
		}
		Odyssey::WidgetButton *btn = getButton(tag);
		if (btn)
			btn->setText(text);
	};

	for (int i = 0; i < KotORBase::kSkillMAX; ++i) {
		setWidgetText(kSkillTags[i].pointTag, Common::composeString(_ranks[i]));
	}

	setWidgetText("REMAINING_SELECTIONS_LBL", Common::composeString(_remainingPoints));
}

void LevelUpSkillsMenu::callbackActive(Widget &widget) {
	const Common::UString &tag = widget.getTag();

	for (int i = 0; i < KotORBase::kSkillMAX; ++i) {
		if (tag == kSkillTags[i].plusTag) {
			KotORBase::Class pcClass = _info.getLatestClass();
			int cost = isClassSkill(pcClass, static_cast<KotORBase::Skill>(i)) ? 1 : 2;

			if (_remainingPoints >= cost) {
				_ranks[i]++;
				_remainingPoints -= cost;
				updateLabels();
			}
			return;
		}
		if (tag == kSkillTags[i].minusTag) {
			if (_ranks[i] > _originalRanks[i]) {
				KotORBase::Class pcClass = _info.getLatestClass();
				int cost = isClassSkill(pcClass, static_cast<KotORBase::Skill>(i)) ? 1 : 2;

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

bool LevelUpSkillsMenu::isClassSkill(KotORBase::Class c, KotORBase::Skill s) const {
	switch (c) {
		case KotORBase::kClassScout:
			return (s == KotORBase::kSkillComputerUse || s == KotORBase::kSkillDemolitions || 
			        s == KotORBase::kSkillStealth || s == KotORBase::kSkillAwareness || 
			        s == KotORBase::kSkillRepair || s == KotORBase::kSkillTreatInjury);
		case KotORBase::kClassScoundrel:
			return (s == KotORBase::kSkillDemolitions || s == KotORBase::kSkillStealth || 
			        s == KotORBase::kSkillAwareness || s == KotORBase::kSkillPersuade || 
			        s == KotORBase::kSkillSecurity || s == KotORBase::kSkillTreatInjury);
		case KotORBase::kClassSoldier:
			return (s == KotORBase::kSkillDemolitions || s == KotORBase::kSkillAwareness || 
			        s == KotORBase::kSkillTreatInjury);
		case KotORBase::kClassJediGuardian:
			return (s == KotORBase::kSkillAwareness || s == KotORBase::kSkillPersuade || 
			        s == KotORBase::kSkillTreatInjury);
		case KotORBase::kClassJediSentinel:
			return (s == KotORBase::kSkillComputerUse || s == KotORBase::kSkillDemolitions || 
			        s == KotORBase::kSkillAwareness || s == KotORBase::kSkillPersuade || 
			        s == KotORBase::kSkillSecurity || s == KotORBase::kSkillTreatInjury);
		case KotORBase::kClassJediConsular:
			return (s == KotORBase::kSkillComputerUse || s == KotORBase::kSkillDemolitions || 
			        s == KotORBase::kSkillAwareness || s == KotORBase::kSkillPersuade || 
			        s == KotORBase::kSkillRepair || s == KotORBase::kSkillTreatInjury);
		default:
			return false;
	}
}

} // End of namespace KotOR

} // End of namespace Engines
