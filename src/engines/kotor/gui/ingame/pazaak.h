#ifndef ENGINES_KOTOR_GUI_INGAME_PAZAAK_H
#define ENGINES_KOTOR_GUI_INGAME_PAZAAK_H

#include <vector>
#include "src/engines/kotorbase/gui/gui.h"
#include "src/engines/kotor/pazaak.h"

namespace Engines {
namespace KotOR {

class PazaakGUI : public KotORBase::GUI {
public:
	PazaakGUI(PazaakEngine &engine, Console *console = nullptr);
	~PazaakGUI();

	void callbackActive(Widget &widget) override;
	void callbackRun() override;

private:
	void updateUI();

	PazaakEngine &_engine;
};

} // End of namespace KotOR
} // End of namespace Engines

#endif
