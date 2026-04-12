#include "src/common/debug.h"
#include "src/engines/kotorbase/module.h"
#include "src/engines/kotor/encounters_ebon.h"

namespace Engines {
namespace KotOR {

void performHyperspaceJump(KotORBase::Module &module) {
	debug("Orchestrating Ebon Hawk Hyperspace Jump...");

	module.setCutsceneMode(true);
	module.setPlayerInputEnabled(false);

	// 1. Play Hyperspace Movie
	module.playMovie("Hyperspace");

	// 2. Camera focus on the cockpit
	module.cameraTransitionToTarget("wp_cockpit_focus", 2.0f);

	module.setCutsceneMode(false);
	module.setPlayerInputEnabled(true);
}

void performPlanetArrival(KotORBase::Module &module) {
	debug("Orchestrating Ebon Hawk Planet Arrival...");

	module.setCutsceneMode(true);
	module.setPlayerInputEnabled(false);

	// 1. Play Landing Movie
	module.playMovie("Landing");

	// 2. Camera transition to the ship on the pad
	module.cameraTransitionToTarget("wp_arrival_focus", 3.0f);

	// 3. Play Arrival Stinger
	module.playMusicStinger("mus_vfx_arrival");

	module.setCutsceneMode(false);
	module.setPlayerInputEnabled(true);
}

void performTurretMinigame(KotORBase::Module &module) {
	debug("Orchestrating Turret Minigame Transition...");

	module.setCutsceneMode(true);
	module.setPlayerInputEnabled(false);

	// 1. Play Sith Fighter Swarm Movie
	module.playMovie("SithFighters");

	// 2. Fade out for transition to Turret GUI
	module.getFadeQuad().fadeOut();
	
	// Signal a custom encounter for the turret mini-game
	module.signalEncounter("ebon_turret_start");

	module.setCutsceneMode(false);
	module.setPlayerInputEnabled(true);
}

} // End of namespace KotOR
} // End of namespace Engines
