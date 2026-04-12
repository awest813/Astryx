#ifndef ENGINES_KOTOR_ENCOUNTERS_EBON_H
#define ENGINES_KOTOR_ENCOUNTERS_EBON_H

namespace Engines {

namespace KotORBase {
	class Module;
}

namespace KotOR {

/** Orchestrate the Ebon Hawk takeoff/hyperspace jump. */
void performHyperspaceJump(KotORBase::Module &module);

/** Orchestrate the Ebon Hawk planet arrival scene. */
void performPlanetArrival(KotORBase::Module &module);

/** Orchestrate the Ebon Hawk turret minigame entry. */
void performTurretMinigame(KotORBase::Module &module);

} // End of namespace KotOR
} // End of namespace Engines

#endif // ENGINES_KOTOR_ENCOUNTERS_EBON_H
