#include "src/engines/odyssey/button.h"
#include "src/engines/odyssey/listbox.h"
#include "src/engines/kotorbase/module.h"
#include "src/engines/kotor/gui/ingame/workbench.h"

namespace Engines {

namespace KotOR {

WorkbenchMenu::WorkbenchMenu(KotORBase::Module &module, ::Engines::Console *console) :
		KotORBase::GUI(console),
		_module(module) {

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
	Odyssey::WidgetListbox *lb = getListBox("LB_ITEMS");
	if (!lb) return;

	lb->clear();
	// Populate with upgradable items from player inventory
	// Placeholder:
	lb->addItem("Lightsaber", "LBL_ITEM_1");
	lb->addItem("Blaster Pistol", "LBL_ITEM_2");
}

void WorkbenchMenu::showItemUpgrades(const Common::UString &itemTag) {
	// Show currently installed upgrades and empty slots
	// [Crystal 1] [Crystal 2] [Lens] [Emitter]
}

void WorkbenchMenu::applyUpgrade(const Common::UString &upgradeTag, int slot) {
	// 1. Logic to modify item properties
	// 2. Play success sound
	_module.playSound("fx_workbench_apply");
}

void WorkbenchMenu::callbackActive(Widget &widget) {
	const Common::UString &tag = widget.getTag();

	if (tag == "BTN_EXIT") {
		_returnCode = 1;
		return;
	}

	if (tag == "BTN_ASSEMBLE") {
		applyUpgrade("selected_upgrade", 0);
	}
}

} // End of namespace KotOR

} // End of namespace Engines
