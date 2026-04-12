#include "src/common/debug.h"
#include "src/engines/kotorbase/module.h"
#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/area.h"
#include "src/engines/kotor/encounters_dan.h"

namespace Engines {
namespace KotOR {

void performMandalorianAmbush(KotORBase::Module &module) {
	debug("Orchestrating Mandalorian Ambush in danm14...");

	// 1. Lock player input for the reveal
	module.setPlayerInputEnabled(false);

	// 2. Setup Camera
	// Smooth pan towards the Grove's clearing
	module.cameraTransitionToTarget("wp_mand_ambush_reveal", 3.0f);

	// 3. Spawn enemies (if not already spawned by GIT)
	// In a real scenario, this would check plot flags first.
	KotORBase::Creature *lead = module.createCreatureByTemplate("dan14_mand_lead");
	KotORBase::Creature *thug1 = module.createCreatureByTemplate("dan14_mand_thug1");
	KotORBase::Creature *thug2 = module.createCreatureByTemplate("dan14_mand_thug2");

	if (lead) {
		lead->setPosition(45.0f, -12.0f, 0.0f); 
		lead->setAIArchetype(KotORBase::Creature::kAIArchetypeTacticalHumanoid);
	}
	if (thug1) {
		thug1->setPosition(42.0f, -15.0f, 0.0f);
		thug1->setAIArchetype(KotORBase::Creature::kAIArchetypeTacticalHumanoid);
	}
	if (thug2) {
		thug2->setPosition(48.0f, -15.0f, 0.0f);
		thug2->setAIArchetype(KotORBase::Creature::kAIArchetypeTacticalHumanoid);
	}

	// 4. Play Mandalorian Battle Stinger
	module.playMusicStinger("mus_bat_mandalorian");

	// 5. Restore control and initiate combat
	// In xoreos, restoring input allows the AI (driven by archetypes) to engage
	module.setPlayerInputEnabled(true);
}

void performKinrathSwarm(KotORBase::Module &module) {
	debug("Orchestrating Kinrath Swarm...");

	// Kinrath ambush usually involves sudden spawns and poison archetypes.
	KotORBase::Creature *k1 = module.createCreatureByTemplate("dan14_kinrath1");
	KotORBase::Creature *k2 = module.createCreatureByTemplate("dan14_kinrath2");

	if (k1) k1->setAIArchetype(KotORBase::Creature::kAIArchetypeBeastPoison);
	if (k2) k2->setAIArchetype(KotORBase::Creature::kAIArchetypeBeastPoison);
	
	module.playMusicStinger("mus_bat_beast");
}

void performStarMapReveal(KotORBase::Module &module) {
	debug("Orchestrating Star Map Reveal Climax...");

	// 1. Cinematic lockdown
	module.setPlayerInputEnabled(false);

	// 2. Open the inner Star Map doors
	// We use SignalEncounter with a sub-ID that scripts can handle if needed
	// or we can manipulate doors directly if we have tags.
	KotORBase::Area *area = module.getArea();
	if (area) {
		KotORBase::Object *door = area->getObjectByTag("dan17_starmap_door");
		if (door) {
			// In a real KOTOR engine, doors are 'Situated' objects.
			// KotORBase::Door *kDoor = ObjectContainer::toDoor(door);
			// if (kDoor) kDoor->open();
		}
	}

	// 3. Climax Camera Sweep
	// Slow reveal of the ancient Rakatan technology
	module.cameraTransitionToTarget("wp_starmap_reveal", 5.0f);

	// 4. Play Star Map Theme Stinger
	module.playMusicStinger("mus_theme_starmap");

	// 5. Update Journal to reflect the revelation
	module.addJournalQuestEntry("k_main_quest", 50); // Found first Star Map
	
	// 6. Final Dialogue Hook (Reaction shot)
	// After 5 seconds, restore control or trigger the next dialogue.
	// For now, we restore control for 'immersion'.
	module.setPlayerInputEnabled(true);
}

} // End of namespace KotOR
} // End of namespace Engines
