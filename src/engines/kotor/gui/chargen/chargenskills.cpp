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
 *  The skill allocation menu for custom character creation.
 */

#include "src/common/strutil.h"

#include "src/engines/odyssey/button.h"
#include "src/engines/odyssey/label.h"

#include "src/engines/kotorbase/gui/chargeninfo.h"
#include "src/engines/kotorbase/creatureinfo.h"
#include "src/engines/kotorbase/gui/guiskilltags.h"

#include "src/engines/kotor/gui/chargen/chargenskills.h"

namespace Engines {

namespace KotOR {

// Per-level skill point base values by class (before Int modifier).
static const int kSkillPointsBySoldier   = 4;
static const int kSkillPointsByScout     = 6;
static const int kSkillPointsByScoundrel = 8;

CharacterGenerationSkillsMenu::CharacterGenerationSkillsMenu(
		KotORBase::CharacterGenerationInfo &info,
		Console *console) :
		CharacterGenerationBaseMenu(info, console) {

	// KotOR 1 uses "skchrgen". Keep a fallback for alternate data layouts.
	try {
		load("skchrgen");
	} catch (...) {
		load("skilitems");
	}

	addBackground(KotORBase::kBackgroundTypeMenu);

	// Initialise working ranks from current info.
	const KotORBase::CreatureInfo::Skills &s = info.getSkills();
	_ranks[KotORBase::kSkillComputerUse] = s.computerUse;
	_ranks[KotORBase::kSkillDemolitions] = s.demolitions;
	_ranks[KotORBase::kSkillStealth]     = s.stealth;
	_ranks[KotORBase::kSkillAwareness]   = s.awareness;
	_ranks[KotORBase::kSkillPersuade]    = s.persuade;
	_ranks[KotORBase::kSkillRepair]      = s.repair;
	_ranks[KotORBase::kSkillSecurity]    = s.security;
	_ranks[KotORBase::kSkillTreatInjury] = s.treatInjury;

	// Compute how many points are still available.
	int total = computeSkillPoints();
	int spent = 0;
	for (int i = 0; i < KotORBase::kSkillMAX; ++i)
		spent += static_cast<int>(_ranks[i]);
	_remainingPoints = total - spent;
	if (_remainingPoints < 0)
		_remainingPoints = 0;

	updateLabels();
}

int CharacterGenerationSkillsMenu::computeSkillPoints() const {
	int base = kSkillPointsBySoldier;
	switch (_info.getClass()) {
		case KotORBase::kClassScout:     base = kSkillPointsByScout;     break;
		case KotORBase::kClassScoundrel: base = kSkillPointsByScoundrel; break;
		default: break;
	}

	// Intelligence modifier: (score - 10) / 2, rounded down.
	int intScore = static_cast<int>(_info.getAbilities().intelligence);
	int intMod   = (intScore - 10) / 2;

	int total = base + intMod;
	return (total < 1) ? 1 : total;
}

void CharacterGenerationSkillsMenu::updateLabels() {
	for (size_t i = 0; i < KotORBase::kSkillWidgetTagCount; ++i) {
		const Common::UString rank = Common::composeString(_ranks[KotORBase::kSkillWidgetTags[i].skill]);
		setWidgetText(KotORBase::kSkillWidgetTags[i].pointTag, rank);
		setWidgetText(KotORBase::kSkillWidgetTags[i].legacyPointTag, rank);
	}

	setWidgetText("REMAINING_SELECTIONS_LBL", Common::composeString(_remainingPoints));
	setWidgetText("SELECTIONS_REMAINING_LBL", Common::composeString(_remainingPoints));
}

void CharacterGenerationSkillsMenu::callbackActive(Widget &widget) {
	const Common::UString &tag = widget.getTag();

	for (size_t i = 0; i < KotORBase::kSkillWidgetTagCount; ++i) {
		const KotORBase::Skill skill = KotORBase::kSkillWidgetTags[i].skill;
		if ((tag == KotORBase::kSkillWidgetTags[i].plusTag) || (tag == KotORBase::kSkillWidgetTags[i].legacyPlusTag)) {
			if (_remainingPoints > 0) {
				++_ranks[skill];
				--_remainingPoints;
				updateLabels();
			}
			return;
		}
		if ((tag == KotORBase::kSkillWidgetTags[i].minusTag) || (tag == KotORBase::kSkillWidgetTags[i].legacyMinusTag)) {
			if (_ranks[skill] > 0) {
				--_ranks[skill];
				++_remainingPoints;
				updateLabels();
			}
			return;
		}
	}

	if (tag == "BTN_RECOMMENDED") {
		const KotORBase::CreatureInfo::Skills &s = _info.getSkills();
		_ranks[KotORBase::kSkillComputerUse] = s.computerUse;
		_ranks[KotORBase::kSkillDemolitions] = s.demolitions;
		_ranks[KotORBase::kSkillStealth]     = s.stealth;
		_ranks[KotORBase::kSkillAwareness]   = s.awareness;
		_ranks[KotORBase::kSkillPersuade]    = s.persuade;
		_ranks[KotORBase::kSkillRepair]      = s.repair;
		_ranks[KotORBase::kSkillSecurity]    = s.security;
		_ranks[KotORBase::kSkillTreatInjury] = s.treatInjury;

		int total = computeSkillPoints();
		int spent = 0;
		for (int i = 0; i < KotORBase::kSkillMAX; ++i)
			spent += static_cast<int>(_ranks[i]);
		_remainingPoints = total - spent;
		if (_remainingPoints < 0)
			_remainingPoints = 0;

		updateLabels();
		return;
	}

	if (tag == "BTN_BACK") {
		_returnCode = 1;
		return;
	}

	if (tag == "BTN_ACCEPT") {
		_info.setSkillRank(KotORBase::kSkillComputerUse, _ranks[KotORBase::kSkillComputerUse]);
		_info.setSkillRank(KotORBase::kSkillDemolitions, _ranks[KotORBase::kSkillDemolitions]);
		_info.setSkillRank(KotORBase::kSkillStealth,     _ranks[KotORBase::kSkillStealth]);
		_info.setSkillRank(KotORBase::kSkillAwareness,   _ranks[KotORBase::kSkillAwareness]);
		_info.setSkillRank(KotORBase::kSkillPersuade,    _ranks[KotORBase::kSkillPersuade]);
		_info.setSkillRank(KotORBase::kSkillRepair,      _ranks[KotORBase::kSkillRepair]);
		_info.setSkillRank(KotORBase::kSkillSecurity,    _ranks[KotORBase::kSkillSecurity]);
		_info.setSkillRank(KotORBase::kSkillTreatInjury, _ranks[KotORBase::kSkillTreatInjury]);
		accept();
		_returnCode = 1;
		return;
	}
}

} // End of namespace KotOR

} // End of namespace Engines
