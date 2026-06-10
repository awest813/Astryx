#ifndef ENGINES_KOTOR_ENCOUNTERS_END_H
#define ENGINES_KOTOR_ENCOUNTERS_END_H

namespace Engines {

namespace KotORBase {
	class Module;
}

namespace KotOR {

/** Play logo/crawl movies and load the first module (from the main menu). */
void performEndarSpireOpening(KotORBase::Module &module);

/** In-module beat after enter(): ship attack shake, journal, music. */
void performEndarSpireOpeningBeat(KotORBase::Module &module);

/** Orchestrate the Trask Ulgo tutorial encounter. */
void performTraskEncounter(KotORBase::Module &module);

/** Orchestrate the Sith boarding event in Sector 2. */
void performSithBoarding(KotORBase::Module &module);

} // End of namespace KotOR
} // End of namespace Engines

#endif // ENGINES_KOTOR_ENCOUNTERS_END_H
