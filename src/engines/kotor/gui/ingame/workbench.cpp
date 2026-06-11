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
#include "src/engines/kotorbase/item.h"
#include "src/engines/kotorbase/itemupgrades.h"
#include "src/engines/kotorbase/module.h"
#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotor/gui/ingame/workbench.h"

namespace Engines {

namespace KotOR {

WorkbenchMenu::WorkbenchMenu(KotORBase::Module &module, ::Engines::Console *console) :
		KotORBase::GUI(console),
		_module(module),
		_selectedItemTag(),
		_selectedUpgradeIndex(-1),
		_selectedSlot(0) {

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
	_upgradeTags.clear();
	_selectedUpgradeIndex = -1;

	KotORBase::Creature *pc = _module.getPC();
	if (!pc)
		return;

	const auto &items = pc->getInventory().getItems();
	for (const auto &entry : items) {
		if (entry.second.count <= 0)
			continue;

		try {
			const KotORBase::Item item(entry.second.tag);
			if (!KotORBase::isUpgradeableItem(item))
				continue;

			_itemTags.push_back(entry.first);

			Common::UString label = item.getName();
			if (entry.second.count > 1)
				label += " (" + Common::composeString(entry.second.count) + ")";
			lb->addItem(label);
		} catch (Common::Exception &e) {
			warning("WorkbenchMenu::fillItemList: %s", e.what());
		}
	}

	lb->refreshItemWidgets();

	if (lb->isEmpty())
		setWidgetText("LBL_MESSAGE", "No upgradeable items in inventory.");
}

void WorkbenchMenu::selectItemByIndex(int index) {
	if (index < 0 || index >= (int)_itemTags.size())
		return;

	showItemUpgrades(_itemTags[index]);
}

void WorkbenchMenu::fillUpgradeList() {
	_upgradeTags.clear();
	_selectedUpgradeIndex = -1;

	Odyssey::WidgetListBox *lb = getListBox("LB_UPGRADES");
	if (!lb)
		lb = getListBox("LB_UPGRADE");
	if (!lb)
		return;

	lb->removeAllItems();

	if (_selectedItemTag.empty())
		return;

	KotORBase::Creature *pc = _module.getPC();
	if (!pc)
		return;

	try {
		const KotORBase::Item target(_selectedItemTag);
		_upgradeTags = KotORBase::getCompatibleUpgradeParts(target, pc->getInventory());

		for (const Common::UString &tag : _upgradeTags) {
			try {
				const KotORBase::Item part(tag);
				lb->addItem(part.getName());
			} catch (...) {
				lb->addItem(tag);
			}
		}
	} catch (Common::Exception &e) {
		setWidgetText("LBL_MESSAGE", e.what());
	}

	lb->refreshItemWidgets();
}

int WorkbenchMenu::findFirstOpenSlot() const {
	if (_selectedItemTag.empty())
		return -1;

	try {
		const KotORBase::Item target(_selectedItemTag);
		const int slotCount = KotORBase::getUpgradeSlotCount(target);
		for (int slot = 0; slot < slotCount; ++slot) {
			if (KotORBase::getAppliedUpgrade(_module, _selectedItemTag, slot).empty())
				return slot;
		}
	} catch (...) {
	}

	return -1;
}

void WorkbenchMenu::showItemUpgrades(const Common::UString &itemTag) {
	_selectedItemTag = itemTag;

	try {
		const KotORBase::Item item(itemTag);
		setWidgetText("LBL_SLOTNAME", item.getName());

		Common::UString status = "Upgrade slots: " + Common::composeString(KotORBase::getUpgradeSlotCount(item));
		const int slotCount = KotORBase::getUpgradeSlotCount(item);
		for (int slot = 0; slot < slotCount; ++slot) {
			const Common::UString applied = KotORBase::getAppliedUpgrade(_module, itemTag, slot);
			if (!applied.empty()) {
				status += "\nSlot " + Common::composeString(slot + 1) + ": " + applied;
			}
		}
		setWidgetText("LBL_MESSAGE", status);
	} catch (Common::Exception &e) {
		setWidgetText("LBL_SLOTNAME", itemTag);
		setWidgetText("LBL_MESSAGE", e.what());
	}

	fillUpgradeList();
}

void WorkbenchMenu::applyUpgrade(const Common::UString &upgradeTag, int slot) {
	if (_selectedItemTag.empty()) {
		setWidgetText("LBL_MESSAGE", "Select an item first.");
		return;
	}

	KotORBase::Creature *pc = _module.getPC();
	if (!pc)
		return;

	const KotORBase::ItemActionResult result =
		KotORBase::applyWorkbenchUpgrade(_module, *pc, _selectedItemTag, upgradeTag, slot);

	if (result.success)
		_module.playSound("fx_workbench_apply");

	setWidgetText("LBL_MESSAGE", result.message);
	if (result.success) {
		showItemUpgrades(_selectedItemTag);
		fillUpgradeList();
	}
}

void WorkbenchMenu::callbackActive(Widget &widget) {
	const Common::UString &tag = widget.getTag();

	if (tag == "BTN_EXIT") {
		_returnCode = 1;
		return;
	}

	if (tag == "BTN_ASSEMBLE") {
		if (_selectedUpgradeIndex < 0 || _selectedUpgradeIndex >= (int)_upgradeTags.size()) {
			setWidgetText("LBL_MESSAGE", "Select an upgrade part first.");
			return;
		}

		const int slot = findFirstOpenSlot();
		if (slot < 0) {
			setWidgetText("LBL_MESSAGE", "All upgrade slots are full.");
			return;
		}

		applyUpgrade(_upgradeTags[_selectedUpgradeIndex], slot);
		return;
	}

	if (tag == "LB_ITEMS" || tag.beginsWith("LB_ITEMS")) {
		Odyssey::WidgetListBox *list = dynamic_cast<Odyssey::WidgetListBox *>(&widget);
		if (!list)
			list = getListBox("LB_ITEMS");
		if (list)
			selectItemByIndex(list->getSelectedIndex());
		return;
	}

	if (tag == "LB_UPGRADES" || tag == "LB_UPGRADE" ||
	    tag.beginsWith("LB_UPGRADES") || tag.beginsWith("LB_UPGRADE")) {
		Odyssey::WidgetListBox *list = dynamic_cast<Odyssey::WidgetListBox *>(&widget);
		if (!list) {
			list = getListBox("LB_UPGRADES");
			if (!list)
				list = getListBox("LB_UPGRADE");
		}
		if (list)
			_selectedUpgradeIndex = list->getSelectedIndex();
		return;
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
