#ifndef ENGINES_KOTOR_GUI_INGAME_WORKBENCH_H
#define ENGINES_KOTOR_GUI_INGAME_WORKBENCH_H

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

private:
	KotORBase::Module &_module;

	void fillItemList();
	void showItemUpgrades(const Common::UString &itemTag);
	void applyUpgrade(const Common::UString &upgradeTag, int slot);
};

} // End of namespace KotOR

} // End of namespace Engines

#endif // ENGINES_KOTOR_GUI_INGAME_WORKBENCH_H
