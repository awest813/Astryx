#include "src/common/util.h"
#include "src/engines/kotorbase/module.h"
#include "src/engines/kotor/encounters_ebon.h"

namespace Engines {
namespace KotOR {

void performHyperspaceJump(KotORBase::Module &module) {
	status("Orchestrating Ebon Hawk Hyperspace Jump...");

	module.setCutsceneMode(true);
	module.setPlayerInputEnabled(false);

	module.playMovie("Hyperspace");

	module.cameraTransitionToTarget("wp_cockpit_focus", 2.0f);
	module.runCinematicBeat(2.0f);

	module.restoreGameplayCamera(1.0f);
	module.runCinematicBeat(1.0f);

	module.setCutsceneMode(false);
	module.setPlayerInputEnabled(true);
}

void performPlanetArrival(KotORBase::Module &module) {
	status("Orchestrating Ebon Hawk Planet Arrival...");

	module.setCutsceneMode(true);
	module.setPlayerInputEnabled(false);

	module.playMovie("Landing");

	module.cameraTransitionToTarget("wp_arrival_focus", 3.0f);
	module.runCinematicBeat(3.0f);

	module.playMusicStinger("mus_vfx_arrival");

	module.restoreGameplayCamera(1.5f);
	module.runCinematicBeat(1.5f);

	module.setCutsceneMode(false);
	module.setPlayerInputEnabled(true);
}

void performTurretMinigame(KotORBase::Module &module) {
	status("Orchestrating Turret Minigame Transition...");

	module.setCutsceneMode(true);
	module.setPlayerInputEnabled(false);

	module.playMovie("SithFighters");

	module.cameraTransitionToTarget("wp_turret_focus", 2.0f);
	module.runCinematicBeat(2.0f);

	module.getFadeQuad().fadeOut();
	module.runCinematicBeat(0.5f);

	module.signalEncounter("ebon_turret");

	module.setCutsceneMode(false);
	module.setPlayerInputEnabled(true);
}

} // End of namespace KotOR
} // End of namespace Engines
