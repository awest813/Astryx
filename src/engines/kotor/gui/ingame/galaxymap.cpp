#include "src/engines/odyssey/button.h"
#include "src/engines/kotorbase/module.h"
#include "src/engines/kotor/gui/ingame/galaxymap.h"

namespace Engines {

namespace KotOR {

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
	// Reveal planets based on game progress
	// For now, reveal all core planets
	static const char * const kPlanets[] = {
		"BTN_TAT", "BTN_KAS", "BTN_MAN", "BTN_KOR", "BTN_DAN"
	};

	for (int i = 0; i < 5; ++i) {
		Odyssey::WidgetButton *btn = getButton(kPlanets[i]);
		if (btn) btn->show();
	}
}

void GalaxyMapMenu::jumpToPlanet(const Common::UString &planetID) {
	Common::UString targetResRef;
	
	if (planetID == "TAT") targetResRef = "tat_m17aa"; // Tatooine Anchorhead
	else if (planetID == "KAS") targetResRef = "kas_m22aa"; // Kashyyyk Czerka Landing
	else if (planetID == "MAN") targetResRef = "manm26aa"; // Manaan Ahto East
	else if (planetID == "KOR") targetResRef = "korr_m33aa"; // Korriban Dreshdae
	else if (planetID == "DAN") targetResRef = "danm13";    // Dantooine Enclave

	if (!targetResRef.empty()) {
		_module.replaceModule(targetResRef);
		_returnCode = 2;
	}
}

void GalaxyMapMenu::callbackActive(Widget &widget) {
	const Common::UString &tag = widget.getTag();
	
	if (tag == "BTN_EXIT") {
		_returnCode = 1;
		return;
	}

	if (tag.startsWith("BTN_")) {
		jumpToPlanet(tag.substr(4));
	}
}

} // End of namespace KotOR

} // End of namespace Engines
