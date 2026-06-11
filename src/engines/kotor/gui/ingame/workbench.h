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
 *  Workbench upgrade menu for Star Wars: Knights of the Old Republic.
 */

#ifndef ENGINES_KOTOR_GUI_INGAME_WORKBENCH_H
#define ENGINES_KOTOR_GUI_INGAME_WORKBENCH_H

#include <vector>

#include "src/common/ustring.h"

#include "src/engines/kotorbase/gui/gui.h"

namespace Engines {

namespace KotORBase {
	class Module;
}

namespace KotOR {

class WorkbenchMenu : public KotORBase::GUI {
public:
	WorkbenchMenu(KotORBase::Module &module, ::Engines::Console *console = 0);
	~WorkbenchMenu();

	void show() override;

protected:
	void callbackActive(Widget &widget);
	void callbackKeyInput(const Events::Key &key, const Events::EventType &type);

private:
	KotORBase::Module &_module;
	Common::UString _selectedItemTag;
	std::vector<Common::UString> _itemTags;
	std::vector<Common::UString> _upgradeTags;
	int _selectedUpgradeIndex;
	int _selectedSlot;

	void fillItemList();
	void fillUpgradeList();
	void showItemUpgrades(const Common::UString &itemTag);
	void applyUpgrade(const Common::UString &upgradeTag, int slot);
	void selectItemByIndex(int index);
	int findFirstOpenSlot() const;
};

} // End of namespace KotOR

} // End of namespace Engines

#endif // ENGINES_KOTOR_GUI_INGAME_WORKBENCH_H
