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
 *  Load Game screen.
 */

#ifndef ENGINES_KOTOR_GUI_MAIN_LOAD_H
#define ENGINES_KOTOR_GUI_MAIN_LOAD_H

#include <vector>
#include "src/engines/kotorbase/gui/gui.h"

namespace Engines {
namespace KotOR {

class Game;

class LoadScreen : public KotORBase::GUI {
public:
	LoadScreen(Game &game);
	~LoadScreen();

	void show() override;
	void hide() override;

	void callbackActive(Widget &widget) override;

private:
	Game &_game;
	std::vector<Common::UString> _saveSlots;

	void updateSaveList();
	void loadSelected();
};

} // End of namespace KotOR
} // End of namespace Engines

#endif
