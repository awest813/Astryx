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

#include "src/aurora/talkman.h"

#include "src/graphics/graphics.h"

#include "src/engines/odyssey/listbox.h"
#include "src/engines/odyssey/label.h"

#include "src/engines/kotorbase/item.h"
#include "src/engines/kotorbase/gui/inventoryitem.h"
#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/module.h"
#include "src/engines/kotorbase/inventory.h"

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

			bool isWeapon = item.isSlotEquipable(KotORBase::kInventorySlotRightWeapon) || item.isSlotEquipable(KotORBase::kInventorySlotLeftWeapon);
			bool isArmor = item.isSlotEquipable(KotORBase::kInventorySlotBody);

			// Filtering
			bool show = false;
			switch (_category) {
			case kCategoryAll: show = true; break;
			case kCategoryWeapons: show = isWeapon; break;
			case kCategoryArmor: show = isArmor; break;
			case kCategoryItems: show = !isWeapon && !isArmor; break;
			case kCategoryMisc:
				show = !isWeapon && !isArmor;
				break;
			}

			if (!show)
				continue;

			lbItems->addItem(Common::String::format("%s|%s|%u",
			                                         item.getName().c_str(),
			                                         item.getIcon().c_str(),
			                                         itemPair.second.count));

			_visibleItems.push_back(itemPair.second.tag);
		} catch (Common::Exception &e) {
			warning("MenuInventory::fillItems: Failed to load item %s: %s", itemPair.second.tag.c_str(), e.what());
		}
	}

	GfxMan.lockFrame();
	lbItems->refreshItemWidgets();
	GfxMan.unlockFrame();

	if (_visibleItems.empty())
		setWidgetText("LBL_DESC", TalkMan.getString(400).empty() ?
		              "No items in this category." : TalkMan.getString(400));
}

void MenuInventory::showItemDescription(int index) {
	if (index < 0 || index >= (int)_visibleItems.size())
		return;

	try {
		KotORBase::Item item(_visibleItems[index]);
		const Common::UString &desc = item.getDescription();
		setWidgetText("LBL_DESC", desc.empty() ? item.getName() : desc);

		Odyssey::WidgetListBox *lbDesc = getListBox("LB_DESC");
		if (lbDesc) {
			lbDesc->removeAllItems();
			lbDesc->addItem(desc.empty() ? item.getName() : desc);
			lbDesc->refreshItemWidgets();
		}
	} catch (Common::Exception &e) {
		warning("MenuInventory::showItemDescription: %s", e.what());
	}
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
	if (tag == "BTN_CAT_MISC") {
		_category = kCategoryMisc;
		fillItems();
		return;
	}

	if (tag == "LB_ITEMS" || tag.beginsWith("LB_ITEMS")) {
		Odyssey::WidgetListBox *list = dynamic_cast<Odyssey::WidgetListBox *>(&widget);
		if (!list)
			list = getListBox("LB_ITEMS");
		if (list)
			showItemDescription(list->getSelectedIndex());
		return;
	}

	MenuBase::callbackActive(widget);
}

} // End of namespace KotOR

} // End of namespace Engines
