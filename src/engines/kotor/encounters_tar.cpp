#include "src/common/util.h"
#include "src/engines/kotorbase/module.h"
#include "src/engines/kotor/encounters_tar.h"

namespace Engines {

namespace KotOR {

void performTarisAmbush(KotORBase::Module &module) {
	debug("Orchestrating Taris Apartment Ambush...");

	module.setCutsceneMode(true);
	module.setPlayerInputEnabled(false);

	// 1. Initial Dialogue stubs for Carth
	module.addJournalQuestEntry("tar_main", 10); // Search for Bastila

	// 2. Transition to the door before it blows
	module.cameraTransitionToTarget("wp_tar_patrol_spawn", 2.0f);
	
	// 3. Stinger & Combat
	module.playMusicStinger("mus_vfx_explosion_lrg");
	module.shakeCamera(0.8f, 0.4f); // Door kick/breach impact
	
	// 4. Spawn Sith Patrol
	KotORBase::Creature *s1 = module.createCreatureByTemplate("n_sithsoldier001");
	KotORBase::Creature *s2 = module.createCreatureByTemplate("n_sithsoldier001");
	
	float x, y, z, angle;
	if (module.getObjectLocation("wp_tar_patrol_spawn", KotORBase::kObjectTypeWaypoint, x, y, z, angle)) {
		if (s1) s1->setPosition(x, y + 1.0f, z);
		if (s2) s2->setPosition(x, y - 1.0f, z);
	}

	if (s1) s1->setAIArchetype(KotORBase::Creature::kAIArchetypeTacticalHumanoid);
	if (s2) s2->setAIArchetype(KotORBase::Creature::kAIArchetypeTacticalHumanoid);

	module.playMusicStinger("mus_bat_sith");
	
	// 5. Assign Tactical AI to Sith soldiers via encounter signal
	module.signalEncounter("tar_patrol_engage");

	module.setCutsceneMode(false);
	module.setPlayerInputEnabled(true);
}

void performDuelRingIntro(KotORBase::Module &module) {
	debug("Orchestrating Duel Ring Introduction...");

	module.setCutsceneMode(true);
	module.setPlayerInputEnabled(false);

	// 1. Pan across the ring
	module.cameraTransitionToTarget("wp_tar_duel_ring", 3.0f);
	
	// 2. Crowd Cheering Stinger
	module.playMusicStinger("mus_vfx_cheer");
	
	// 3. Zoom into Ajan Kest (Announcer)
	module.cameraTransitionToTarget("wp_tar_announcer_focus", 2.0f);

	// 4. Hold for a moment then restore
	module.setCutsceneMode(false);
	module.setPlayerInputEnabled(true);
}

void performBekEntrance(KotORBase::Module &module) {
	debug("Orchestrating Hidden Bek Entrance...");
	// Logic for Gadon Theeth's security protocol
	module.setCutsceneMode(true);
	module.cameraTransitionToTarget("wp_bek_gadon_reveal", 4.0f);
	module.setCutsceneMode(false);
}

void performLowerCityIntro(KotORBase::Module &module) {
	debug("Orchestrating Lower City Introduction...");

	module.setCutsceneMode(true);
	module.setPlayerInputEnabled(false);

	// 1. Elevator exit focus
	module.cameraTransitionToTarget("wp_tar_elevator_exit", 2.0f);

	// 2. Spawn combatants
	KotORBase::Creature *sith = module.createCreatureByTemplate("n_sithsoldier001");
	KotORBase::Creature *vulkar = module.createCreatureByTemplate("n_vulkarthug001");

	float x, y, z, angle;
	if (module.getObjectLocation("wp_tar_skirmish_center", KotORBase::kObjectTypeWaypoint, x, y, z, angle)) {
		if (sith) sith->setPosition(x + 2.0f, y, z);
		if (vulkar) vulkar->setPosition(x - 2.0f, y, z);
	}

	if (sith) sith->setAIArchetype(KotORBase::Creature::kAIArchetypeTacticalHumanoid);
	if (vulkar) vulkar->setAIArchetype(KotORBase::Creature::kAIArchetypeTacticalHumanoid);

	// 3. Play Skirmish Stinger
	module.playMusicStinger("mus_bat_skirmish");
	
	// Ensure they are hostile to each other (Custom script signal)
	module.signalEncounter("tar_lower_skirmish_engage");

	module.setCutsceneMode(false);
	module.setPlayerInputEnabled(true);
}

void performRakghoulAmbush(KotORBase::Module &module) {
	debug("Orchestrating Undercity Rakghoul Ambush...");

	module.setCutsceneMode(true);
	module.setPlayerInputEnabled(false);

	// 1. Horror-style focus
	module.cameraTransitionToTarget("wp_rakghoul_reveal", 3.0f);
	module.shakeCamera(1.5f, 0.5f);
	
	// 2. Play Rakghoul scream stinger
	module.playMusicStinger("mus_vfx_rakghoul_scream");

	// 3. Spawn a swarm
	for (int i = 0; i < 3; ++i) {
		KotORBase::Creature *rak = module.createCreatureByTemplate("n_rakghoul001");
		if (rak) {
			float x, y, z, angle;
			module.getObjectLocation("wp_rakghoul_spawn_" + Common::composeString(i), KotORBase::kObjectTypeWaypoint, x, y, z, angle);
			rak->setPosition(x, y, z);
			rak->setAIArchetype(KotORBase::Creature::kAIArchetypeBeastPoison);
		}
	}

	module.playMusicStinger("mus_bat_rakghoul");

	module.setCutsceneMode(false);
	module.setPlayerInputEnabled(true);
}

void performCaloNordReveal(KotORBase::Module &module) {
	debug("Orchestrating Calo Nord Cantina Reveal...");

	module.setCutsceneMode(true);
	module.setPlayerInputEnabled(false);

	// 1. Focus on the legendary bounty hunter sitting in the corner
	module.cameraTransitionToTarget("wp_tar_calo_focus", 4.0f);
	
	// 2. Play Calo's Theme Stinger
	module.playMusicStinger("mus_theme_calo");

	// 3. Zoom into his face for the iconic "One..." line setup
	module.cameraTransitionToTarget("wp_tar_calo_face", 2.0f);

	// Restore control for the dialogue interaction
	module.setCutsceneMode(false);
	module.setPlayerInputEnabled(true);
}

void performEbonHawkEscape(KotORBase::Module &module) {
	debug("Orchestrating Ebon Hawk Escape from Taris...");

	module.setCutsceneMode(true);
	module.setPlayerInputEnabled(false);

	// 1. Dramatic reveal of the Hawk in Davik's hangar
	module.cameraTransitionToTarget("wp_ebon_hawk_reveal", 4.0f);
	
	// 2. Play Ship Power-up Stinger
	module.playMusicStinger("mus_theme_ebonhawk");
	module.shakeCamera(2.0f, 0.3f); // Engine vibration

	// 3. Final fade to black for the orbital transition
	// In xoreos, we usually trigger a module transition at the end of the script
	module.playMusicStinger("mus_vfx_ship_takeoff");
	
	// Transition to the space battle or next world (Dantooine)
	// module.replaceModule("dan_m13"); 
}

void performBrejikShowdown(KotORBase::Module &module) {
	debug("Orchestrating Brejik Showdown on Swoop Platform...");

	module.setCutsceneMode(true);
	module.setPlayerInputEnabled(false);

	// 1. Initial Pan of the winners circle
	module.cameraTransitionToTarget("wp_tar_platform_pan", 4.0f);
	
	// 2. Focus on Bastila in the Neuromesh Cage
	module.cameraTransitionToTarget("wp_tar_bastila_cage", 3.0f);
	module.playMusicStinger("mus_vfx_electric_hum");

	// 3. Brejik's confrontation reveal
	module.cameraTransitionToTarget("wp_tar_brejik_approach", 2.0f);
	module.playMusicStinger("mus_theme_brejik");

	// 4. Spawn Brejik and his guards
	KotORBase::Creature *brejik = module.createCreatureByTemplate("n_brejik001");
	if (brejik) {
		float x, y, z, angle;
		if (module.getObjectLocation("wp_tar_brejik_spawn", KotORBase::kObjectTypeWaypoint, x, y, z, angle)) {
			brejik->setPosition(x, y, z);
			brejik->setAIArchetype(KotORBase::Creature::kAIArchetypeTacticalHumanoid);
		}
	}

	for (int i = 0; i < 2; ++i) {
		KotORBase::Creature *guard = module.createCreatureByTemplate("n_vulkarthug001");
		if (guard) {
			float x, y, z, angle;
			module.getObjectLocation("wp_tar_brejik_guard_" + Common::composeString(i), KotORBase::kObjectTypeWaypoint, x, y, z, angle);
			guard->setPosition(x, y, z);
			guard->setAIArchetype(KotORBase::Creature::kAIArchetypeTacticalHumanoid);
		}
	}

	// 5. Final tension stinger before control is restored for combat
	module.playMusicStinger("mus_bat_brejik");
	module.signalEncounter("tar_brejik_combat_start");

	module.setCutsceneMode(false);
	module.setPlayerInputEnabled(true);
}

} // End of namespace KotOR

} // End of namespace Engines
