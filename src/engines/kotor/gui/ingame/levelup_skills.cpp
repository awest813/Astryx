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
#include "src/engines/kotorbase/gui/guiskilltags.h"

#include "src/engines/kotor/gui/ingame/levelup_skills.h"

namespace Engines {

namespace KotOR {

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
	return KotORBase::skillPointsPerLevel(_info);
}

void LevelUpSkillsMenu::updateLabels() {
	for (size_t i = 0; i < KotORBase::kSkillWidgetTagCount; ++i) {
		const Common::UString rank = Common::composeString(_ranks[KotORBase::kSkillWidgetTags[i].skill]);
		setWidgetText(KotORBase::kSkillWidgetTags[i].pointTag, rank);
		setWidgetText(KotORBase::kSkillWidgetTags[i].legacyPointTag, rank);
	}

	const Common::UString remaining = Common::composeString(_remainingPoints);
	setWidgetText("REMAINING_SELECTIONS_LBL", remaining);
	setWidgetText("SELECTIONS_REMAINING_LBL", remaining);
}

void LevelUpSkillsMenu::callbackActive(Widget &widget) {
	const Common::UString &tag = widget.getTag();
	const KotORBase::Class pcClass = _info.getLatestClass();

	for (size_t i = 0; i < KotORBase::kSkillWidgetTagCount; ++i) {
		const KotORBase::Skill skill = KotORBase::kSkillWidgetTags[i].skill;
		if (tag == KotORBase::kSkillWidgetTags[i].plusTag || tag == KotORBase::kSkillWidgetTags[i].legacyPlusTag) {
			const int cost = KotORBase::isClassSkill(pcClass, skill) ? 1 : 2;

			if (_remainingPoints >= cost) {
				_ranks[skill]++;
				_remainingPoints -= cost;
				updateLabels();
			}
			return;
		}
		if (tag == KotORBase::kSkillWidgetTags[i].minusTag || tag == KotORBase::kSkillWidgetTags[i].legacyMinusTag) {
			if (_ranks[skill] > _originalRanks[skill]) {
				const int cost = KotORBase::isClassSkill(pcClass, skill) ? 1 : 2;

				_ranks[skill]--;
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
