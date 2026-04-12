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

#include "src/common/configman.h"
#include "src/common/filepath.h"
#include "src/common/filelist.h"

#include "src/engines/odyssey/listbox.h"
#include "src/engines/odyssey/label.h"

#include "src/engines/kotor/game.h"
#include "src/engines/kotor/gui/main/load.h"

namespace Engines {
namespace KotOR {

LoadScreen::LoadScreen(Game &game) : KotORBase::GUI(game.getModule(), "loadscreen"), _game(game) {
}

LoadScreen::~LoadScreen() {
}

void LoadScreen::show() {
	GUI::show();
	updateSaveList();
}

void LoadScreen::hide() {
	GUI::hide();
}

void LoadScreen::updateSaveList() {
	Odyssey::WidgetListBox *lbSaves = getListBox("LB_SAVES");
	if (!lbSaves) return;

	lbSaves->removeAllItems();
	_saveSlots.clear();

	// In a real implementation we'd use ConfigMan.getGameDataDir() + "/saves"
	Common::UString savePath = "saves";
	Common::FileList saves(savePath, "", true);

	for (auto const& save : saves) {
		if (save.isDirectory) {
			_saveSlots.push_back(save.name);
			lbSaves->addItem(save.name);
		}
	}
}

void LoadScreen::callbackActive(Widget &widget) {
	const Common::UString &tag = widget.getTag();

	if (tag == "BTN_CANCEL") {
		_game.getModule().showMenu(); // Return to main menu
	} else if (tag == "BTN_LOAD") {
		loadSelected();
	}
}

void LoadScreen::loadSelected() {
	Odyssey::WidgetListBox *lbSaves = getListBox("LB_SAVES");
	int index = lbSaves->getSelectedIndex();
	if (index < 0 || index >= (int)_saveSlots.size())
		return;

	Common::UString slot = _saveSlots[index];
	// _game.loadGame(slot); // TODO: Implement Game::loadGame
}

} // End of namespace KotOR
} // End of namespace Engines
