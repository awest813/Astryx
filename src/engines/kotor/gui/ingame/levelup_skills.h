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

#ifndef ENGINES_KOTOR_GUI_INGAME_LEVELUP_SKILLS_H
#define ENGINES_KOTOR_GUI_INGAME_LEVELUP_SKILLS_H

#include <map>

#include "src/engines/kotorbase/gui/gui.h"
#include "src/engines/kotorbase/creatureinfo.h"

namespace Engines {

class Console;

namespace KotOR {

/** The skill allocation menu for character level-up.
 */
class LevelUpSkillsMenu : public KotORBase::GUI {
public:
	LevelUpSkillsMenu(KotORBase::CreatureInfo &info, Console *console = nullptr);
	~LevelUpSkillsMenu();

	void callbackActive(Widget &widget) override;

	bool isAccepted() const { return _accepted; }

private:
	void updateLabels();
	int computeAvailablePoints() const;

	KotORBase::CreatureInfo &_info;

	uint32_t _ranks[KotORBase::kSkillMAX];
	uint32_t _originalRanks[KotORBase::kSkillMAX];

	int _remainingPoints;
	bool _accepted;
};

} // End of namespace KotOR

} // End of namespace Engines

#endif // ENGINES_KOTOR_GUI_INGAME_LEVELUP_SKILLS_H
