#include "src/common/util.h"
#include "src/engines/kotorbase/module.h"
#include "src/engines/kotor/encounters_end.h"

namespace Engines {

namespace KotOR {

void performEndarSpireOpening(KotORBase::Module &module) {
	debug("Orchestrating Endar Spire Opening...");

	// 1. Initial Movies (The Star Wars hook)
	module.playMovie("swlogo");
	module.playMovie("openingcrawl");

	// 2. Transition to Endar Spire Interior
	module.replaceModule("end_m01aa");

	// 3. Stinger & Impact
	// We call this after a slight delay in a real runner, but for now we chain it
	module.playMusicStinger("mus_bat_ship");
	module.shakeCamera(4.0f, 0.8f); // Long initial tremor

	// 4. Initial Journal Entry
	module.addJournalQuestEntry("k_main_quest", 5); // Escape the Endar Spire
}

void performTraskEncounter(KotORBase::Module &module) {
	debug("Spawning Trask Ulgo Tutorial...");

	// This is typically called when the player clicks the door in the starting room.
	// Trask runs up and initiates dialogue.
	module.shakeCamera(1.5f, 0.4f); // Secondary explosion
	module.playMusicStinger("mus_vfx_explosion");
	
	// Trask dialogue hook usually follows automatically via SignalEncounter handlers
}

void performSithBoarding(KotORBase::Module &module) {
	debug("Orchestrating Sith Boarding Action...");

	// 1. Zoom to the airlock door
	module.cameraTransitionToTarget("wp_sector_2_airlock", 2.5f);

	// 2. Timed tremors
	module.shakeCamera(0.8f, 0.6f);
	module.playMusicStinger("mus_vfx_impact");

	// 3. Dialogue alert: "They're slicing through the door!"
	module.playMusicStinger("mus_bat_sith");
}

} // End of namespace KotOR

} // End of namespace Engines
