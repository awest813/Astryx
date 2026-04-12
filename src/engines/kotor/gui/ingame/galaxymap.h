#ifndef ENGINES_KOTOR_GUI_INGAME_GALAXYMAP_H
#define ENGINES_KOTOR_GUI_INGAME_GALAXYMAP_H

#include "src/engines/kotorbase/gui/gui.h"

namespace Engines {

namespace KotORBase {
	class Module;
}

namespace KotOR {

class GalaxyMapMenu : public KotORBase::GUI {
public:
	GalaxyMapMenu(KotORBase::Module &module, ::Engines::Console *console = 0);
	~GalaxyMapMenu();

	void show();

protected:
	void callbackActive(Widget &widget);

private:
	KotORBase::Module &_module;

	void fillPlanets();
	void jumpToPlanet(const Common::UString &planetID);
};

} // End of namespace KotOR

} // End of namespace Engines

#endif // ENGINES_KOTOR_GUI_INGAME_GALAXYMAP_H
