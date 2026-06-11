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

#include "src/common/strutil.h"

#include "src/events/events.h"

#include "src/engines/odyssey/button.h"
#include "src/engines/odyssey/listbox.h"
#include "src/engines/kotorbase/module.h"
#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotor/gui/ingame/workbench.h"

namespace Engines {

namespace KotOR {

WorkbenchMenu::WorkbenchMenu(KotORBase::Module &module, ::Engines::Console *console) :
		KotORBase::GUI(console),
		_module(module),
		_selectedItemTag() {

	load("workbench");
	addBackground(KotORBase::kBackgroundTypeMenu);
}

WorkbenchMenu::~WorkbenchMenu() {
}

void WorkbenchMenu::show() {
	KotORBase::GUI::show();
	fillItemList();
}

void WorkbenchMenu::fillItemList() {
	Odyssey::WidgetListBox *lb = getListBox("LB_ITEMS");
	if (!lb)
		return;

	lb->removeAllItems();
	_selectedItemTag.clear();
	_itemTags.clear();

	KotORBase::Creature *pc = _module.getPC();
	if (!pc)
		return;

	const auto &items = pc->getInventory().getItems();
	for (const auto &entry : items) {
		if (entry.second.count <= 0)
			continue;

		_itemTags.push_back(entry.first);

		Common::UString label = entry.first;
		if (entry.second.count > 1)
			label += " (" + Common::composeString(entry.second.count) + ")";
		lb->addItem(label);
	}

	lb->refreshItemWidgets();
}

void WorkbenchMenu::selectItemByIndex(int index) {
	if (index < 0 || index >= (int)_itemTags.size())
		return;

	showItemUpgrades(_itemTags[index]);
}

void WorkbenchMenu::showItemUpgrades(const Common::UString &itemTag) {
	_selectedItemTag = itemTag;
	setWidgetText("LBL_SLOTNAME", itemTag);
}

void WorkbenchMenu::applyUpgrade(const Common::UString &upgradeTag, int slot) {
	(void)upgradeTag;
	(void)slot;

	if (_selectedItemTag.empty()) {
		setWidgetText("LBL_MESSAGE", "Select an item first.");
		return;
	}

	_module.playSound("fx_workbench_apply");
	setWidgetText("LBL_MESSAGE", "Upgrade applied to " + _selectedItemTag);
}

void WorkbenchMenu::callbackActive(Widget &widget) {
	const Common::UString &tag = widget.getTag();

	if (tag == "BTN_EXIT") {
		_returnCode = 1;
		return;
	}

	if (tag == "BTN_ASSEMBLE") {
		applyUpgrade("selected_upgrade", 0);
		return;
	}

	if (tag == "LB_ITEMS" || tag.beginsWith("LB_ITEMS")) {
		Odyssey::WidgetListBox *list = dynamic_cast<Odyssey::WidgetListBox *>(&widget);
		if (!list)
			list = getListBox("LB_ITEMS");
		if (list)
			selectItemByIndex(list->getSelectedIndex());
	}
}

void WorkbenchMenu::callbackKeyInput(const Events::Key &key, const Events::EventType &type) {
	Odyssey::WidgetListBox *lb = getListBox("LB_ITEMS");
	if (!lb || type != Events::kEventKeyDown)
		return;

	switch (key) {
	case Events::kKeyUp:
		lb->selectPreviousItem();
		selectItemByIndex(lb->getSelectedIndex());
		break;
	case Events::kKeyDown:
		lb->selectNextItem();
		selectItemByIndex(lb->getSelectedIndex());
		break;
	default:
		break;
	}
}

} // End of namespace KotOR

} // End of namespace Engines
