#include "src/common/configman.h"

#include "src/engines/kotorbase/module.h"

#include "src/engines/kotor/encounters_end.h"

namespace Engines {

namespace KotOR {

void performEndarSpireOpening(KotORBase::Module &module) {
	if (module.isLoaded())
		return;

	Common::UString firstModule = ConfigMan.getString("KOTOR_startModule", "end_m01aa");
	if (firstModule.empty())
		firstModule = "end_m01aa";

	module.load(firstModule);
}

} // End of namespace KotOR

} // End of namespace Engines
