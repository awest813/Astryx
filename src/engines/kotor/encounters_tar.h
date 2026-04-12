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

/** Orchestrate the transition from Upper to Lower City (Sith/Vulkar skirmish). */
void performLowerCityIntro(KotORBase::Module &module);

/** Orchestrate the first Rakghoul ambush in the Undercity. */
void performRakghoulAmbush(KotORBase::Module &module);

/** Orchestrate Calo Nord's introduction in the Cantina. */
void performCaloNordReveal(KotORBase::Module &module);

/** Orchestrate the Ebon Hawk escape from Taris. */
void performEbonHawkEscape(KotORBase::Module &module);

/** Orchestrate the Brejik showdown after the swoop race. */
void performBrejikShowdown(KotORBase::Module &module);

} // End of namespace KotOR
} // End of namespace Engines

#endif // ENGINES_KOTOR_ENCOUNTERS_TAR_H
