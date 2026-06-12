#include "src/common/util.h"

#include "src/engines/odyssey/button.h"
#include "src/engines/kotorbase/module.h"
#include "src/engines/kotor/gui/ingame/galaxymap.h"

namespace Engines {

namespace KotOR {

struct PlanetEntry {
	const char *buttonTag;
	int         planetIndex;
	const char *moduleResRef;
};

static const PlanetEntry kPlanets[] = {
	{ "BTN_TAT", 1, "tat_m17aa" },
	{ "BTN_KAS", 2, "kas_m22aa" },
	{ "BTN_MAN", 3, "manm26aa"  },
	{ "BTN_KOR", 4, "korr_m33aa" },
	{ "BTN_DAN", 5, "danm13"    },
};

GalaxyMapMenu::GalaxyMapMenu(KotORBase::Module &module, ::Engines::Console *console) :
		KotORBase::GUI(console),
		_module(module) {

	load("galaxymap");
	addBackground(KotORBase::kBackgroundTypeMenu);
}

GalaxyMapMenu::~GalaxyMapMenu() {
}

void GalaxyMapMenu::show() {
	KotORBase::GUI::show();
	fillPlanets();
}

void GalaxyMapMenu::fillPlanets() {
	bool anyConfigured = false;
	for (size_t i = 0; i < ARRAYSIZE(kPlanets); ++i) {
		if (_module.getPlanetAvailable(kPlanets[i].planetIndex))
			anyConfigured = true;
	}

	for (size_t i = 0; i < ARRAYSIZE(kPlanets); ++i) {
		Odyssey::WidgetButton *btn = getButton(kPlanets[i].buttonTag);
		if (!btn)
			continue;

		const bool available = anyConfigured ?
		                       _module.getPlanetAvailable(kPlanets[i].planetIndex) : true;
		const bool selectable = anyConfigured ?
		                        _module.getPlanetSelectable(kPlanets[i].planetIndex) : true;

		if (!available) {
			btn->hide();
			continue;
		}

		btn->show();
		btn->setDisabled(!selectable);
	}
}

void GalaxyMapMenu::jumpToPlanet(int planetIndex, const Common::UString &targetResRef) {
	if (!_module.getPlanetAvailable(planetIndex) || !_module.getPlanetSelectable(planetIndex))
		return;

	_module.setSelectedPlanet(planetIndex);
	_module.playMovie("Hyperspace");
	_module.load(targetResRef);
	_returnCode = 2;
}

void GalaxyMapMenu::callbackActive(Widget &widget) {
	const Common::UString &tag = widget.getTag();

	if (tag == "BTN_EXIT") {
		_returnCode = 1;
		return;
	}

	for (size_t i = 0; i < ARRAYSIZE(kPlanets); ++i) {
		if (tag == kPlanets[i].buttonTag) {
			jumpToPlanet(kPlanets[i].planetIndex, kPlanets[i].moduleResRef);
			return;
		}
	}
}

} // End of namespace KotOR

} // End of namespace Engines
