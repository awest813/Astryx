#include "src/common/util.h"

#include "src/engines/kotorbase/area.h"
#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/module.h"
#include "src/engines/kotorbase/objectcontainer.h"
#include "src/engines/kotorbase/types.h"

#include "src/engines/kotor/encounters_end.h"

namespace Engines {

namespace KotOR {

static KotORBase::Creature *findTraskUlgo(KotORBase::Module &module) {
	KotORBase::Area *area = module.getCurrentArea();
	if (!area)
		return nullptr;

	if (Object *o = area->getObjectByTag("end_trask"))
		return KotORBase::ObjectContainer::toCreature(o);
	if (Object *o = area->getObjectByTag("trask"))
		return KotORBase::ObjectContainer::toCreature(o);

	for (KotORBase::Creature *c : area->getCreatures()) {
		if (c->getTemplateResRef() == "end_trask")
			return c;
	}

	return nullptr;
}

void performEndarSpireOpening(KotORBase::Module &module) {
	status("Orchestrating Endar Spire Opening...");

	// 1. Initial Movies (The Star Wars hook)
	module.playMovie("swlogo");
	module.playMovie("openingcrawl");

	// 2. Cinematic Lockdown
	module.setCutsceneMode(true);
	module.setPlayerInputEnabled(false);

	// 3. Load the Endar Spire (runModule::enter() starts gameplay)
	if (!module.isLoaded())
		module.load("end_m01aa");

	// 4. Stinger & Impact
	module.playMusicStinger("mus_bat_ship");
	module.shakeCamera(4.0f, 0.8f);

	// 5. Initial Journal Entry
	module.addJournalQuestEntry("k_main_quest", 5);

	// 6. Restore control for the wake-up sequence
	module.setCutsceneMode(false);
	module.setPlayerInputEnabled(true);
}

void performTraskEncounter(KotORBase::Module &module) {
	status("Spawning Trask Ulgo Tutorial...");

	module.setCutsceneMode(true);
	module.setPlayerInputEnabled(false);

	module.shakeCamera(1.5f, 0.4f);
	module.playMusicStinger("mus_vfx_explosion");

	module.cameraTransitionToTarget("wp_trask_reveal", 2.0f);

	if (KotORBase::Creature *trask = findTraskUlgo(module)) {
		module.setCinematicFocus(trask);

		const Common::UString &conversation = trask->getConversation();
		if (!conversation.empty()) {
			module.setCutsceneMode(false);
			module.setPlayerInputEnabled(true);
			module.startConversation(conversation, trask);
			return;
		}
	}

	module.setCutsceneMode(false);
	module.setPlayerInputEnabled(true);
}

void performSithBoarding(KotORBase::Module &module) {
	status("Orchestrating Sith Boarding Action...");

	module.setCutsceneMode(true);
	module.setPlayerInputEnabled(false);

	module.cameraTransitionToTarget("wp_sector_2_airlock", 2.5f);

	module.shakeCamera(1.0f, 0.7f);
	module.playMusicStinger("mus_vfx_impact");

	KotORBase::Area *area = module.getCurrentArea();
	KotORBase::Creature *sith1 = module.createCreatureByTemplate("n_sithsoldier001");
	KotORBase::Creature *sith2 = module.createCreatureByTemplate("n_sithsoldier001");

	float x, y, z, angle;
	if (module.getObjectLocation("wp_sector_2_airlock", KotORBase::kObjectTypeWaypoint, x, y, z, angle)) {
		if (sith1) {
			sith1->setPosition(x + 1.0f, y, z);
			sith1->setOrientation(0.0f, 0.0f, 1.0f, angle);
		}
		if (sith2) {
			sith2->setPosition(x - 1.0f, y, z);
			sith2->setOrientation(0.0f, 0.0f, 1.0f, angle);
		}
	}

	auto setupSith = [](KotORBase::Creature *sith) {
		if (!sith)
			return;
		sith->setFaction(KotORBase::kFactionEndarSpire);
		sith->setAIArchetype(KotORBase::Creature::kAIArchetypeTacticalHumanoid);
	};

	setupSith(sith1);
	setupSith(sith2);

	if (area) {
		if (sith1)
			area->addCreature(sith1);
		if (sith2)
			area->addCreature(sith2);
	}

	module.playMusicStinger("mus_bat_sith");

	module.setCutsceneMode(false);
	module.setPlayerInputEnabled(true);
}

} // End of namespace KotOR

} // End of namespace Engines
