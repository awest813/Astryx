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
 *  The character level-up GUI.
 */

#ifndef ENGINES_KOTOR_GUI_INGAME_LEVELUP_H
#define ENGINES_KOTOR_GUI_INGAME_LEVELUP_H

#include "src/engines/kotorbase/gui/gui.h"

namespace Engines {

namespace KotORBase {
	class Module;
	class Creature;
}

namespace KotOR {

/** The character level-up GUI.
 *
 *  Loads the "levelpnl" GUI resource.
 */
class LevelUpGUI : public KotORBase::GUI {
public:
	LevelUpGUI(KotORBase::Module &module, KotORBase::Creature &pc, ::Engines::Console *console = 0);
	~LevelUpGUI();

	void callbackActive(Widget &widget) override;
	void callbackRun() override;

	void showAbilities();
	void showSkills();
	void showFeats();
	void showForcePowers();

private:
	void finalizeLevelUp();

	KotORBase::Module   &_module;
	KotORBase::Creature &_pc;

	int _step;
};

} // End of namespace KotOR

} // End of namespace Engines

#endif // ENGINES_KOTOR_GUI_INGAME_LEVELUP_H
