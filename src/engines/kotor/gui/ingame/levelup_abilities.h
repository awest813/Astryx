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
 *  The ability score allocation menu for character level-up.
 */

#ifndef ENGINES_KOTOR_GUI_INGAME_LEVELUP_ABILITIES_H
#define ENGINES_KOTOR_GUI_INGAME_LEVELUP_ABILITIES_H

#include "src/engines/kotorbase/gui/gui.h"
#include "src/engines/kotorbase/creatureinfo.h"

namespace Engines {

class Console;

namespace KotOR {

/** The ability score allocation menu for character level-up.
 *
 *  During level-up, every 4 levels the character gains 1 ability point
 *  to spend in any of the 6 attributes.
 */
class LevelUpAbilitiesMenu : public KotORBase::GUI {
public:
	LevelUpAbilitiesMenu(KotORBase::CreatureInfo &info, Console *console = nullptr);
	~LevelUpAbilitiesMenu();

	void callbackActive(Widget &widget) override;

	bool isAccepted() const { return _accepted; }

private:
	void updateLabels();

	KotORBase::CreatureInfo &_info;

	uint32_t _str, _dex, _con, _intl, _wis, _cha;
	uint32_t _originalStr, _originalDex, _originalCon, _originalIntl, _originalWis, _originalCha;

	int _remainingPoints;
	bool _accepted;
};

} // End of namespace KotOR

} // End of namespace Engines

#endif // ENGINES_KOTOR_GUI_INGAME_LEVELUP_ABILITIES_H
