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
 *  The ingame container inventory menu.
 */

#include "src/aurora/talkman.h"

#include "src/engines/odyssey/panel.h"
#include "src/engines/odyssey/scrollbar.h"
#include "src/engines/odyssey/listbox.h"
#include "src/engines/odyssey/label.h"

#include "src/engines/kotorbase/item.h"

#include "src/engines/kotorbase/gui/inventoryitem.h"

#include "src/engines/kotor/gui/ingame/container.h"

namespace Engines {

namespace KotOR {

ContainerMenu::ContainerMenu(Console *console) : KotORBase::GUI(console) {

	load("container");

	Odyssey::WidgetPanel *guiPanel = getPanel("TGuiPanel");
	guiPanel->setPosition(-guiPanel->getWidth()/2, -guiPanel->getHeight()/2, 0);

	Odyssey::WidgetListBox *lbItems = getListBox("LB_ITEMS");
	lbItems->setItemWidgetFactoryFunction([](Engines::GUI &gui, const Common::UString &tag) { return new KotORBase::WidgetInventoryItem(gui, tag); });
	lbItems->setPadding(18);
	lbItems->createItemWidgets(3);
}

void ContainerMenu::bindInventories(KotORBase::Inventory &container, KotORBase::Inventory &party) {
	_container = &container;
	_party = &party;
}

void ContainerMenu::fillFromInventory(const KotORBase::Inventory &inv) {
	_itemTags.clear();

	if (inv.getItems().empty())
		setWidgetText("LBL_MESSAGE", TalkMan.getString(394));

	Odyssey::WidgetListBox *lbItems = getListBox("LB_ITEMS");
	lbItems->removeAllItems();

	const std::map<Common::UString, KotORBase::Inventory::ItemGroup> &invItems = inv.getItems();
	for (std::map<Common::UString, KotORBase::Inventory::ItemGroup>::const_iterator i = invItems.begin();
			i != invItems.end(); ++i) {
		try {
			KotORBase::Item item(i->second.tag);
			_itemTags.push_back(i->first);
			lbItems->addItem(Common::String::format("%s|%s|%u",
			                                         item.getName().c_str(),
			                                         item.getIcon().c_str(),
			                                         i->second.count));
		} catch (Common::Exception &e) {
			e.add("Failed to load item %s", i->second.tag.c_str());
			Common::printException(e, "WARNING: ");
		}
	}

	lbItems->refreshItemWidgets();
}

void ContainerMenu::takeSelectedItem() {
	if (!_container || !_party)
		return;

	Odyssey::WidgetListBox *lbItems = getListBox("LB_ITEMS");
	if (!lbItems)
		return;

	const int index = lbItems->getSelectedIndex();
	if (index < 0 || index >= (int)_itemTags.size())
		return;

	const Common::UString &tag = _itemTags[index];
	const auto &items = _container->getItems();
	const auto it = items.find(tag);
	if (it == items.end())
		return;

	const int count = it->second.count > 0 ? it->second.count : 1;
	_party->addItem(tag, count);
	_container->removeItem(tag, count);

	fillFromInventory(*_container);

	if (!lbItems->isEmpty())
		lbItems->selectItem(index < (int)_itemTags.size() ? index : (int)_itemTags.size() - 1);
}

void ContainerMenu::takeAllItems() {
	if (!_container || !_party)
		return;

	const std::map<Common::UString, KotORBase::Inventory::ItemGroup> items = _container->getItems();
	for (const auto &entry : items)
		_party->addItem(entry.first, entry.second.count);

	_container->removeAllItems();
	fillFromInventory(*_container);
}

void ContainerMenu::callbackActive(Widget &widget) {
	const Common::UString &tag = widget.getTag();

	if (tag == "BTN_OK") {
		takeAllItems();
		_returnCode = 1;
		return;
	}

	if (tag == "BTN_CANCEL") {
		_returnCode = kReturnCodeAbort;
		return;
	}

	if (tag == "LB_ITEMS" || tag.beginsWith("LB_ITEMS")) {
		takeSelectedItem();
		return;
	}
}

void ContainerMenu::callbackKeyInput(const Events::Key &key, const Events::EventType &type) {
	if (type == Events::kEventKeyDown) {
		switch (key) {
			case Events::kKeyUp:
				getListBox("LB_ITEMS")->selectPreviousItem();
				break;
			case Events::kKeyDown:
				getListBox("LB_ITEMS")->selectNextItem();
				break;
			case Events::kKeyReturn:
				takeSelectedItem();
				break;
			default:
				break;
		}
	}
}

} // End of namespace KotOR

} // End of namespace Engines
