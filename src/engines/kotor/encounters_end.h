#ifndef ENGINES_KOTOR_ENCOUNTERS_END_H
#define ENGINES_KOTOR_ENCOUNTERS_END_H

namespace Engines {

namespace KotORBase {
	class Module;
}

namespace KotOR {

/** Load the first module after character generation. The Endar Spire's own
 *  OnEnter script (k_pend_area01) plays the opening movie, fades in and
 *  starts Trask's wake-up conversation. */
void performEndarSpireOpening(KotORBase::Module &module);

} // End of namespace KotOR

} // End of namespace Engines

#endif // ENGINES_KOTOR_ENCOUNTERS_END_H
