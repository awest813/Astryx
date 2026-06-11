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
}

void WorkbenchMenu::showItemUpgrades(const Common::UString &itemTag) {
	_selectedItemTag = itemTag;
	setWidgetText("LBL_SLOTNAME", itemTag);
}

void WorkbenchMenu::applyUpgrade(const Common::UString &upgradeTag, int slot) {
	(void)upgradeTag;
	(void)slot;

	if (_selectedItemTag.empty())
		return;

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

	Odyssey::WidgetListBox *lb = getListBox("LB_ITEMS");
	if (!lb)
		return;

	const int index = lb->getSelectedIndex();
	if (index >= 0 && index < (int)_itemTags.size())
		showItemUpgrades(_itemTags[index]);
}

} // End of namespace KotOR

} // End of namespace Engines
