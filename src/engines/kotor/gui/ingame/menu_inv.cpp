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

#include "src/engines/odyssey/button.h"
#include "src/engines/odyssey/listbox.h"
#include "src/engines/odyssey/label.h"

#include "src/engines/kotorbase/area.h"
#include "src/engines/kotorbase/item.h"
#include "src/engines/kotorbase/itemactions.h"
#include "src/engines/kotorbase/gui/inventoryitem.h"
#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/module.h"
#include "src/engines/kotorbase/inventory.h"

#include "src/engines/kotor/gui/ingame/menu_inv.h"

namespace Engines {

namespace KotOR {

MenuInventory::MenuInventory(KotORBase::Module &module, Console *console) :
		KotORBase::MenuBase(module, console),
		_category(kCategoryAll),
		_selectedIndex(-1) {

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

	Odyssey::WidgetListBox *lbItems = getListBox("LB_ITEMS");
	if (!lbItems)
		return;

	const int selected = lbItems->getSelectedIndex();
	if (selected != _selectedIndex) {
		_selectedIndex = selected;
		showItemDescription(_selectedIndex);
		updateActionButtons();
	}
}

void MenuInventory::fillItems() {
	KotORBase::Creature *pc = _module->getPC();
	if (!pc)
		return;

	KotORBase::Inventory &inv = pc->getInventory();

	Odyssey::WidgetListBox *lbItems = getListBox("LB_ITEMS");
	if (!lbItems)
		return;

	lbItems->removeAllItems();
	_visibleItems.clear();
	_selectedIndex = -1;

	for (const auto &itemPair : inv.getItems()) {
		try {
			KotORBase::Item item(itemPair.second.tag);

			const bool isWeapon = item.isSlotEquipable(KotORBase::kInventorySlotRightWeapon) ||
			                      item.isSlotEquipable(KotORBase::kInventorySlotLeftWeapon);
			const bool isArmor = item.isSlotEquipable(KotORBase::kInventorySlotBody);

			bool show = false;
			switch (_category) {
			case kCategoryAll: show = true; break;
			case kCategoryWeapons: show = isWeapon; break;
			case kCategoryArmor: show = isArmor; break;
			case kCategoryItems: show = !isWeapon && !isArmor; break;
			case kCategoryMisc: show = !isWeapon && !isArmor; break;
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

	if (_visibleItems.empty()) {
		setWidgetText("LBL_DESC", TalkMan.getString(400).empty() ?
		              "No items in this category." : TalkMan.getString(400));
	} else if (lbItems->getSelectedIndex() < 0) {
		lbItems->selectItem(0);
		_selectedIndex = 0;
		showItemDescription(0);
	}

	updateActionButtons();
}

void MenuInventory::showItemDescription(int index) {
	if (index < 0 || index >= (int)_visibleItems.size()) {
		setWidgetText("LBL_DESC", "");
		return;
	}

	try {
		KotORBase::Item item(_visibleItems[index]);
		const Common::UString &desc = item.getDescription();
		setWidgetText("LBL_DESC", desc.empty() ? item.getName() : desc);

		if (Odyssey::WidgetListBox *lbDesc = getListBox("LB_DESC")) {
			lbDesc->removeAllItems();
			lbDesc->addItem(desc.empty() ? item.getName() : desc);
			lbDesc->refreshItemWidgets();
		}
	} catch (Common::Exception &e) {
		warning("MenuInventory::showItemDescription: %s", e.what());
	}
}

void MenuInventory::setStatusMessage(const Common::UString &message) {
	if (!message.empty())
		setWidgetText("LBL_DESC", message);
}

void MenuInventory::updateActionButtons() {
	const bool hasSelection = _selectedIndex >= 0 && _selectedIndex < (int)_visibleItems.size();
	bool canUse = false;
	bool canEquip = false;

	if (hasSelection) {
		try {
			const KotORBase::Item item(_visibleItems[_selectedIndex]);
			canEquip = KotORBase::isEquipableItem(item);
			canUse = KotORBase::isUsableConsumable(item) || canEquip;
		} catch (...) {
		}
	}

	if (Odyssey::WidgetButton *useBtn = getButton("BTN_USEITEM")) {
		useBtn->setDisabled(!canUse);
		useBtn->setInvisible(!canUse);
		if (canUse)
			useBtn->show();
		else
			useBtn->hide();
	}

	if (Odyssey::WidgetButton *legacyUseBtn = getButton("BTN_USE")) {
		legacyUseBtn->setDisabled(!canUse);
		legacyUseBtn->setInvisible(!canUse);
		if (canUse)
			legacyUseBtn->show();
		else
			legacyUseBtn->hide();
	}

	if (Odyssey::WidgetButton *equipBtn = getButton("BTN_EQUIP")) {
		equipBtn->setDisabled(!canEquip);
		equipBtn->setInvisible(!canEquip);
		if (canEquip)
			equipBtn->show();
		else
			equipBtn->hide();
	}

	if (Odyssey::WidgetButton *dropBtn = getButton("BTN_DROP")) {
		dropBtn->setDisabled(!hasSelection);
		dropBtn->setInvisible(!hasSelection);
		if (hasSelection)
			dropBtn->show();
		else
			dropBtn->hide();
	}
}

bool MenuInventory::performUseSelectedItem() {
	if (_selectedIndex < 0 || _selectedIndex >= (int)_visibleItems.size())
		return false;

	KotORBase::Creature *pc = _module->getPC();
	KotORBase::Creature *leader = _module->getPartyLeader();
	if (!pc || !leader)
		return false;

	const KotORBase::ItemActionResult result =
		KotORBase::useInventoryItem(*leader, *pc, _visibleItems[_selectedIndex]);

	setStatusMessage(result.message);
	if (result.success && _module->getCurrentArea())
		_module->getCurrentArea()->addToObjectMap(leader);

	fillItems();
	return result.success;
}

bool MenuInventory::performEquipSelectedItem() {
	if (_selectedIndex < 0 || _selectedIndex >= (int)_visibleItems.size())
		return false;

	KotORBase::Creature *pc = _module->getPC();
	KotORBase::Creature *leader = _module->getPartyLeader();
	if (!pc || !leader)
		return false;

	const KotORBase::ItemActionResult result =
		KotORBase::equipInventoryItem(*leader, *pc, _visibleItems[_selectedIndex]);

	setStatusMessage(result.message);
	if (result.success && _module->getCurrentArea())
		_module->getCurrentArea()->addToObjectMap(leader);

	fillItems();
	return result.success;
}

bool MenuInventory::performDropSelectedItem() {
	if (_selectedIndex < 0 || _selectedIndex >= (int)_visibleItems.size())
		return false;

	KotORBase::Creature *pc = _module->getPC();
	if (!pc)
		return false;

	const KotORBase::ItemActionResult result =
		KotORBase::dropInventoryItem(*pc, _visibleItems[_selectedIndex], 1);

	setStatusMessage(result.message);
	if (result.success)
		fillItems();

	return result.success;
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
	if (tag == "BTN_CAT_MISC" || tag == "BTN_QUESTITEMS") {
		_category = kCategoryMisc;
		fillItems();
		return;
	}

	if (tag == "BTN_USEITEM" || tag == "BTN_USE") {
		performUseSelectedItem();
		return;
	}

	if (tag == "BTN_EQUIP") {
		performEquipSelectedItem();
		return;
	}

	if (tag == "BTN_DROP" || tag == "BTN_DESTROY") {
		performDropSelectedItem();
		return;
	}

	if (tag == "LB_ITEMS" || tag.beginsWith("LB_ITEMS")) {
		Odyssey::WidgetListBox *list = dynamic_cast<Odyssey::WidgetListBox *>(&widget);
		if (!list)
			list = getListBox("LB_ITEMS");
		if (list) {
			_selectedIndex = list->getSelectedIndex();
			showItemDescription(_selectedIndex);
			updateActionButtons();
		}
		return;
	}

	MenuBase::callbackActive(widget);
}

void MenuInventory::callbackKeyInput(const Events::Key &key, const Events::EventType &type) {
	if (type != Events::kEventKeyDown)
		return;

	Odyssey::WidgetListBox *lbItems = getListBox("LB_ITEMS");
	if (!lbItems)
		return;

	switch (key) {
	case Events::kKeyUp:
		lbItems->selectPreviousItem();
		_selectedIndex = lbItems->getSelectedIndex();
		showItemDescription(_selectedIndex);
		updateActionButtons();
		break;
	case Events::kKeyDown:
		lbItems->selectNextItem();
		_selectedIndex = lbItems->getSelectedIndex();
		showItemDescription(_selectedIndex);
		updateActionButtons();
		break;
	case Events::kKeyReturn:
		performUseSelectedItem();
		break;
	default:
		break;
	}
}

} // End of namespace KotOR

} // End of namespace Engines
