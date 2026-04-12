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
 *  The ingame inventar menu.
 */

#include "src/graphics/graphics.h"

#include "src/engines/odyssey/listbox.h"

#include "src/engines/kotorbase/item.h"
#include "src/engines/kotorbase/gui/inventoryitem.h"

#include "src/engines/kotor/gui/ingame/menu_inv.h"

namespace Engines {

namespace KotOR {

MenuInventory::MenuInventory(KotORBase::Module &module, Console *console) :
		KotORBase::MenuBase(module, console), _category(kCategoryAll) {

	load("inventory");

	Odyssey::WidgetListBox *lbItems = getListBox("LB_ITEMS");
	if (lbItems) {
		lbItems->setItemWidgetFactoryFunction([](Engines::GUI &gui, const Common::UString &tag) { return new KotORBase::WidgetInventoryItem(gui, tag); });
		lbItems->setHideScrollbar(false);
		lbItems->setPadding(6);
		lbItems->setItemBorderColor(0.0f, 0.0f, 0.0f, 0.0f);
		lbItems->setSoundSelectItem("gui_actuse");
		lbItems->createItemWidgets(8);
	}
}

void MenuInventory::show() {
	GUI::show();
	fillItems();
}

void MenuInventory::update() {
	MenuBase::update();
	updatePartyLeader("LBL_PORT");
}

void MenuInventory::fillItems() {
	KotORBase::Inventory &inv = _module->getPC()->getInventory();

	Odyssey::WidgetListBox *lbItems = getListBox("LB_ITEMS");
	if (!lbItems)
		return;

	lbItems->removeAllItems();
	_visibleItems.clear();

	for (const auto &itemPair : inv.getItems()) {
		try {
			KotORBase::Item item(itemPair.second.tag);

			// Filtering
			bool show = false;
			switch (_category) {
			case kCategoryAll: show = true; break;
			case kCategoryWeapons: show = item.isWeapon(); break;
			case kCategoryArmor: show = item.isArmor(); break;
			case kCategoryItems: show = !item.isWeapon() && !item.isArmor(); break;
			case kCategoryMisc: show = item.getBaseItem() == 0; break; // Placeholder misc
			}

			if (!show)
				continue;

			lbItems->addItem(Common::String::format("%s|%s|%u",
			                                         item.getName().c_str(),
			                                         item.getIcon().c_str(),
			                                         itemPair.second.count));

			_visibleItems.push_back(itemPair.second.tag);
		} catch (Common::Exception &e) {
			debug("MenuInventory::fillItems: Failed to load item %s: %s", itemPair.second.tag.c_str(), e.what());
		}
	}

	GfxMan.lockFrame();
	lbItems->refreshItemWidgets();
	GfxMan.unlockFrame();
}

void MenuInventory::callbackActive(Widget &widget) {
	const Common::UString &tag = widget.getTag();

	if (tag == "BTN_EXIT") {
		_returnCode = 1;
		return;
	}

	if (tag == "BTN_CAT_ALL") {
		_category = kCategoryAll;
		fillItems();
		return;
	}
	if (tag == "BTN_CAT_WEAP") {
		_category = kCategoryWeapons;
		fillItems();
		return;
	}
	if (tag == "BTN_CAT_ARMO") {
		_category = kCategoryArmor;
		fillItems();
		return;
	}
	if (tag == "BTN_CAT_ITEM") {
		_category = kCategoryItems;
		fillItems();
		return;
	}

	MenuBase::callbackActive(widget);
}

} // End of namespace KotOR

} // End of namespace Engines
