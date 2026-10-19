#include "src/common/util.h"
#include "src/common/string.h"

#include "src/aurora/2dareg.h"
#include "src/aurora/2dafile.h"

#include "src/engines/odyssey/button.h"
#include "src/engines/kotorbase/module.h"
#include "src/engines/kotor/gui/ingame/galaxymap.h"

namespace Engines {

namespace KotOR {

struct PlanetEntry {
	const char *buttonTag;
	const char *label;        ///< Substring of the planet's planetary.2da label.
	int         planetIndex;  ///< Retail PLANET_* constant, if planetary.2da can't be read.
	const char *moduleResRef;
};

// Planet indices are the rows of planetary.2da, which is what the original
// scripts pass to SetPlanetAvailable() / SetPlanetSelectable() (PLANET_*).
static const PlanetEntry kPlanets[] = {
	{ "BTN_DAN", "dantooine",  3, "danm13"     },
	{ "BTN_TAT", "tatooine",   4, "tat_m17aa"  },
	{ "BTN_KAS", "kashyyyk",   5, "kas_m22aa"  },
	{ "BTN_MAN", "manaan",     6, "manm26aa"   },
	{ "BTN_KOR", "korriban",   7, "korr_m33aa" },
	// Late-game destinations (buttons may be absent in GUI until scripts enable them).
	{ "BTN_UNK", "unknown",    9, "unk_m41aa"  },
	{ "BTN_STA", "star",      10, "sta_m45aa"  },
};

/** The planet's row in planetary.2da, falling back to the retail constant. */
static int getPlanetIndex(const PlanetEntry &planet) {
	try {
		const Aurora::TwoDAFile &twoda = TwoDAReg.get2DA("planetary");
		for (size_t i = 0; i < twoda.getRowCount(); ++i)
			if (twoda.getRow(i).getString("label").toLower().contains(planet.label))
				return static_cast<int>(i);
	} catch (...) {
	}

	return planet.planetIndex;
}

GalaxyMapMenu::GalaxyMapMenu(KotORBase::Module &module, ::Engines::Console *console) :
		KotORBase::GUI(console),
		_module(module) {

	load("galaxymap");
	addBackground(KotORBase::kBackgroundTypeMenu);
}

GalaxyMapMenu::~GalaxyMapMenu() {
}

Odyssey::WidgetButton *GalaxyMapMenu::findPlanetButton(const char *tag, int index) {
	// Our tag, or the planetary.2da row ("_<row>") style.
	if (Odyssey::WidgetButton *button = getButton(tag))
		return button;

	return getButton(Common::String::format("_%d", index));
}

void GalaxyMapMenu::show() {
	KotORBase::GUI::show();
	fillPlanets();
}

void GalaxyMapMenu::fillPlanets() {
	bool anyConfigured = false;
	for (size_t i = 0; i < ARRAYSIZE(kPlanets); ++i) {
		if (_module.getPlanetAvailable(getPlanetIndex(kPlanets[i])))
			anyConfigured = true;
	}

	for (size_t i = 0; i < ARRAYSIZE(kPlanets); ++i) {
		const int index = getPlanetIndex(kPlanets[i]);

		Odyssey::WidgetButton *btn = findPlanetButton(kPlanets[i].buttonTag, index);
		if (!btn)
			continue;

		const bool available = anyConfigured ?
		                       _module.getPlanetAvailable(index) : true;
		const bool selectable = anyConfigured ?
		                        _module.getPlanetSelectable(index) : true;

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
		const int index = getPlanetIndex(kPlanets[i]);
		if (tag == kPlanets[i].buttonTag || tag == Common::String::format("_%d", index)) {
			jumpToPlanet(index, kPlanets[i].moduleResRef);
			return;
		}
	}
}

} // End of namespace KotOR

} // End of namespace Engines
