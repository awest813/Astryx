#ifndef ENGINES_KOTOR_ENCOUNTERS_DAN_H
#define ENGINES_KOTOR_ENCOUNTERS_DAN_H

namespace Engines {
namespace KotORBase {
    class Module;
}

namespace KotOR {

/** Orchestrate the Mandalorian Ambush in the Grove (danm14). */
void performMandalorianAmbush(KotORBase::Module &module);

/** Orchestrate the Kinrath Swarm encounter. */
void performKinrathSwarm(KotORBase::Module &module);

/** Orchestrate the Star Map Reveal sequence. */
void performStarMapReveal(KotORBase::Module &module);

} // End of namespace KotOR
} // End of namespace Engines

#endif
