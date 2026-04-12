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

	// 2. Cinematic Lockdown
	module.setCutsceneMode(true);
	module.setPlayerInputEnabled(false);

	// 3. Transition to Endar Spire Interior
	module.replaceModule("end_m01aa");

	// 4. Stinger & Impact
	// We call this after a slight delay in a real runner, but for now we chain it
	module.playMusicStinger("mus_bat_ship");
	module.shakeCamera(4.0f, 0.8f); // Long initial tremor

	// 5. Initial Journal Entry
	module.addJournalQuestEntry("k_main_quest", 5); // Escape the Endar Spire

	// 6. Restore control for the wake-up sequence
	module.setCutsceneMode(false);
	module.setPlayerInputEnabled(true);
}

void performTraskEncounter(KotORBase::Module &module) {
	debug("Spawning Trask Ulgo Tutorial...");

	module.setCutsceneMode(true);
	module.setPlayerInputEnabled(false);

	// This is typically called when the player clicks the door in the starting room.
	// Trask runs up and initiates dialogue.
	module.shakeCamera(1.5f, 0.4f); // Secondary explosion
	module.playMusicStinger("mus_vfx_explosion");

	// Focus camera on the door/Trask reveal
	module.cameraTransitionToTarget("wp_trask_reveal", 2.0f);
	
	// Trask dialogue hook usually follows automatically via SignalEncounter handlers
	// but we set the cutscene mode to ensure smooth transition to dialogue components.
}

void performSithBoarding(KotORBase::Module &module) {
	debug("Orchestrating Sith Boarding Action...");

	module.setCutsceneMode(true);
	module.setPlayerInputEnabled(false);

	// 1. Zoom to the airlock door
	module.cameraTransitionToTarget("wp_sector_2_airlock", 2.5f);

	// 2. Timed tremors
	module.shakeCamera(1.0f, 0.7f);
	module.playMusicStinger("mus_vfx_impact");

	// 3. Spawn Sith Soldiers
	KotORBase::Creature *sith1 = module.createCreatureByTemplate("n_sithsoldier001");
	KotORBase::Creature *sith2 = module.createCreatureByTemplate("n_sithsoldier001");

	float x, y, z, angle;
	if (module.getObjectLocation("wp_sector_2_airlock", KotORBase::kObjectTypeWaypoint, x, y, z, angle)) {
		if (sith1) sith1->setPosition(x + 1.0f, y, z);
		if (sith2) sith2->setPosition(x - 1.0f, y, z);
	}

	if (sith1) sith1->setAIArchetype(KotORBase::Creature::kAIArchetypeTacticalHumanoid);
	if (sith2) sith2->setAIArchetype(KotORBase::Creature::kAIArchetypeTacticalHumanoid);

	// 4. Dialogue alert: "They're slicing through the door!"
	module.playMusicStinger("mus_bat_sith");

	// Restore control for the battle
	module.setCutsceneMode(false);
	module.setPlayerInputEnabled(true);
}

} // End of namespace KotOR

} // End of namespace Engines
