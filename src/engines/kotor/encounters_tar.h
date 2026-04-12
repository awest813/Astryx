#ifndef ENGINES_KOTOR_ENCOUNTERS_TAR_H
#define ENGINES_KOTOR_ENCOUNTERS_TAR_H

namespace Engines {

namespace KotORBase {
	class Module;
}

namespace KotOR {

/** Orchestrate the Taris Apartment Ambush (Sith Patrol). */
void performTarisAmbush(KotORBase::Module &module);

/** Orchestrate the Duel Ring introduction in the Upper City Cantina. */
void performDuelRingIntro(KotORBase::Module &module);

/** Orchestrate the Hidden Bek base entrance. */
void performBekEntrance(KotORBase::Module &module);

} // End of namespace KotOR
} // End of namespace Engines

#endif // ENGINES_KOTOR_ENCOUNTERS_TAR_H
