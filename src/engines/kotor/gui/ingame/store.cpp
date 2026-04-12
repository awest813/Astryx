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
 *  The ingame store GUI.
 */

#include "src/graphics/graphics.h"
#include "src/engines/odyssey/listbox.h"
#include "src/engines/odyssey/label.h"

#include "src/engines/kotorbase/module.h"
#include "src/engines/kotorbase/store.h"
#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/gui/inventoryitem.h"

#include "src/engines/kotor/gui/ingame/store.h"

namespace Engines {

namespace KotOR {

StoreGUI::StoreGUI(KotORBase::Module &module, KotORBase::Store &store, ::Engines::Console *console) :
		KotORBase::GUI(console),
		_module(module),
		_store(store) {

	load("storepnl");

	addBackground(KotORBase::kBackgroundTypeMenu);

	Odyssey::WidgetListBox *lbStore = getListBox("LB_STORE_ITEMS");
	if (lbStore) {
		lbStore->setItemWidgetFactoryFunction([](Engines::GUI &gui, const Common::UString &tag) { return new KotORBase::WidgetInventoryItem(gui, tag); });
		lbStore->createItemWidgets(8);
	}

	Odyssey::WidgetListBox *lbPlayer = getListBox("LB_PLAYER_ITEMS");
	if (lbPlayer) {
		lbPlayer->setItemWidgetFactoryFunction([](Engines::GUI &gui, const Common::UString &tag) { return new KotORBase::WidgetInventoryItem(gui, tag); });
		lbPlayer->createItemWidgets(8);
	}

	fillStoreInventory();
	fillPlayerInventory();
	
	updatePrice();
}

void StoreGUI::updatePrice() {
	Odyssey::WidgetLabel *lblPrice = getLabel("LBL_TRANSACTION_PRICE");
	if (!lblPrice) return;

	Odyssey::WidgetListBox *lbStore = getListBox("LB_STORE_ITEMS");
	Odyssey::WidgetListBox *lbPlayer = getListBox("LB_PLAYER_ITEMS");

	KotORBase::Creature *pc = _module.getPC();
	if (!pc) return;

	int selectedStoreIdx = lbStore->getSelectedIndex();
	int selectedPlayerIdx = lbPlayer->getSelectedIndex();

	// Charisma-based price modifiers (1.0 +/- 0.05 per mod point)
	float chaMod = pc->getAbilityModifier(KotORBase::kAbilityCharisma) * 0.05f;
	float buyMultiplier = 1.0f - chaMod;
	float sellMultiplier = 1.0f + chaMod;

	if (selectedStoreIdx >= 0) {
		// Calculate Buy Price
		auto items = _store.getInventory().getItems();
		auto it = items.begin();
		std::advance(it, selectedStoreIdx);
		KotORBase::Item item(it->second.tag);
		
		int basePrice = _store.getBuyPrice(item, *pc);
		int finalPrice = static_cast<int>(basePrice * buyMultiplier);
		
		lblPrice->setText(Common::String::format("Buy: %d Credits (Cha Disc: %d%%)", 
		                  finalPrice, static_cast<int>(chaMod * 100)));
		lblPrice->setTextColor(0.0f, 0.8f, 1.0f, 1.0f); // Teal
	} else if (selectedPlayerIdx >= 0) {
		// Calculate Sell Price
		auto items = pc->getInventory().getItems();
		auto it = items.begin();
		std::advance(it, selectedPlayerIdx);
		KotORBase::Item item(it->second.tag);

		int basePrice = _store.getSellPrice(item, *pc);
		int finalPrice = static_cast<int>(basePrice * sellMultiplier);

		lblPrice->setText(Common::String::format("Sell: %d Credits (Cha Bonus: %d%%)", 
		                  finalPrice, static_cast<int>(chaMod * 100)));
		lblPrice->setTextColor(1.0f, 0.6f, 0.0f, 1.0f); // Orange
	} else {
		lblPrice->setText("Select an item to trade");
		lblPrice->setTextColor(0.7f, 0.7f, 0.7f, 1.0f); // Grey
	}
}

StoreGUI::~StoreGUI() {
}

void StoreGUI::callbackActive(::Engines::Widget &widget) {
	const Common::UString &tag = widget.getTag();

	if (tag == "BTN_EXIT") {
		_returnCode = 1;
		return;
	}

	if (tag == "BTN_BUY") {
		Odyssey::WidgetListBox *lbStore = getListBox("LB_STORE_ITEMS");
		int selectedIdx = lbStore->getSelectedIndex();
		if (selectedIdx < 0) return;

		KotORBase::Creature *pc = _module.getPC();
		auto items = _store.getInventory().getItems();
		auto it = items.begin();
		std::advance(it, selectedIdx);
		KotORBase::Item item(it->second.tag);

		float buyMultiplier = 1.0f - (pc->getAbilityModifier(KotORBase::kAbilityCharisma) * 0.05f);
		int price = static_cast<int>(_store.getBuyPrice(item, *pc) * buyMultiplier);

		if (pc->getInventory().getGold() >= (uint32_t)price) {
			pc->getInventory().addGold(-price);
			_store.getInventory().addGold(price);

			pc->getInventory().addItem(it->second.tag, 1);
			_store.getInventory().removeItem(it->second.tag, 1);

			fillStoreInventory();
			fillPlayerInventory();
			updatePrice();
		}
		return;
	}

	if (tag == "BTN_SELL") {
		Odyssey::WidgetListBox *lbPlayer = getListBox("LB_PLAYER_ITEMS");
		int selectedIdx = lbPlayer->getSelectedIndex();
		if (selectedIdx < 0) return;

		KotORBase::Creature *pc = _module.getPC();
		auto items = pc->getInventory().getItems();
		auto it = items.begin();
		std::advance(it, selectedIdx);
		KotORBase::Item item(it->second.tag);

		float sellMultiplier = 1.0f + (pc->getAbilityModifier(KotORBase::kAbilityCharisma) * 0.05f);
		int price = static_cast<int>(_store.getSellPrice(item, *pc) * sellMultiplier);

		_store.getInventory().addGold(-price);
		pc->getInventory().addGold(price);

		_store.getInventory().addItem(it->second.tag, 1);
		pc->getInventory().removeItem(it->second.tag, 1);

		fillStoreInventory();
		fillPlayerInventory();
		updatePrice();
		return;
	}
}

void StoreGUI::fillStoreInventory() {
	Odyssey::WidgetListBox *lbStore = getListBox("LB_STORE_ITEMS");
	if (!lbStore) return;

	lbStore->removeAllItems();
	for (const auto &group : _store.getInventory().getItems()) {
		try {
			KotORBase::Item item(group.second.tag);
			lbStore->addItem(Common::String::format("%s|%s|%u",
				item.getName().c_str(), item.getIcon().c_str(), group.second.count));
		} catch (...) {}
	}
	
	Odyssey::WidgetLabel *lblStoreCredits = getLabel("LBL_STORE_CREDITS");
	if (lblStoreCredits)
		lblStoreCredits->setText(Common::String::format("%u", _store.getInventory().getGold()));

	lbStore->refreshItemWidgets();
}

void StoreGUI::fillPlayerInventory() {
	Odyssey::WidgetListBox *lbPlayer = getListBox("LB_PLAYER_ITEMS");
	if (!lbPlayer) return;

	KotORBase::Creature *pc = _module.getPC();
	if (!pc) return;

	lbPlayer->removeAllItems();
	for (const auto &group : pc->getInventory().getItems()) {
		try {
			KotORBase::Item item(group.second.tag);
			lbPlayer->addItem(Common::String::format("%s|%s|%u",
				item.getName().c_str(), item.getIcon().c_str(), group.second.count));
		} catch (...) {}
	}

	Odyssey::WidgetLabel *lblPCCredits = getLabel("LBL_PC_CREDITS");
	if (lblPCCredits)
		lblPCCredits->setText(Common::String::format("%u", pc->getInventory().getGold()));

	lbPlayer->refreshItemWidgets();
}

void StoreGUI::callbackRun() {
	Odyssey::WidgetListBox *lbStore = getListBox("LB_STORE_ITEMS");
	Odyssey::WidgetListBox *lbPlayer = getListBox("LB_PLAYER_ITEMS");

	// Mutual exclusivity: if you select an item in one grid, deselect the other.
	if (lbStore->isHovered() || lbPlayer->isHovered()) {
		if (lbStore->isHovered() && lbPlayer->getSelectedIndex() >= 0) {
			lbPlayer->selectItem(-1);
		} else if (lbPlayer->isHovered() && lbStore->getSelectedIndex() >= 0) {
			lbStore->selectItem(-1);
		}
	}

	updatePrice();
}

} // End of namespace KotOR

} // End of namespace Engines
