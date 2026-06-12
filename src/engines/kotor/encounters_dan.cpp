#include "src/common/util.h"
#include "src/engines/kotorbase/module.h"
#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/area.h"
#include "src/engines/kotor/encounters_dan.h"

namespace Engines {
namespace KotOR {

void performMandalorianAmbush(KotORBase::Module &module) {
	status("Orchestrating Mandalorian Ambush in danm14...");

	module.setCutsceneMode(true);
	module.setPlayerInputEnabled(false);

	module.cameraTransitionToTarget("wp_mand_ambush_reveal", 3.0f);
	module.runCinematicBeat(3.0f);

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

	module.playMusicStinger("mus_bat_mandalorian");
	module.runCinematicBeat(1.5f);

	module.restoreGameplayCamera(1.0f);
	module.runCinematicBeat(1.0f);

	module.setCutsceneMode(false);
	module.setPlayerInputEnabled(true);
}

void performKinrathSwarm(KotORBase::Module &module) {
	status("Orchestrating Kinrath Swarm...");

	module.setCutsceneMode(true);
	module.setPlayerInputEnabled(false);

	module.cameraTransitionToTarget("wp_kinrath_ambush", 2.0f);
	module.shakeCamera(1.0f, 0.4f);

	KotORBase::Creature *k1 = module.createCreatureByTemplate("dan14_kinrath1");
	KotORBase::Creature *k2 = module.createCreatureByTemplate("dan14_kinrath2");

	if (k1) k1->setAIArchetype(KotORBase::Creature::kAIArchetypeBeastPoison);
	if (k2) k2->setAIArchetype(KotORBase::Creature::kAIArchetypeBeastPoison);

	module.playMusicStinger("mus_bat_beast");
	module.runCinematicBeat(2.0f);

	module.restoreGameplayCamera(1.0f);
	module.runCinematicBeat(1.0f);

	module.setCutsceneMode(false);
	module.setPlayerInputEnabled(true);
}

void performStarMapReveal(KotORBase::Module &module) {
	status("Orchestrating Star Map Reveal Climax...");

	module.setCutsceneMode(true);
	module.setPlayerInputEnabled(false);

	KotORBase::Area *area = module.getCurrentArea();
	if (area) {
		KotORBase::Object *door = area->getObjectByTag("dan17_starmap_door");
		(void)door;
	}

	module.cameraTransitionToTarget("wp_starmap_reveal", 5.0f);
	module.runCinematicBeat(5.0f);

	module.playMusicStinger("mus_theme_starmap");

	module.addJournalQuestEntry("k_main_quest", 50);

	module.restoreGameplayCamera(2.0f);
	module.runCinematicBeat(2.0f);

	module.setCutsceneMode(false);
	module.setPlayerInputEnabled(true);
}

} // End of namespace KotOR
} // End of namespace Engines
