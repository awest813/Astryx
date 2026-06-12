#include <memory>

#include "src/common/configman.h"
#include "src/common/util.h"

#include "src/engines/kotorbase/area.h"
#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/module.h"
#include "src/engines/kotorbase/object.h"
#include "src/engines/kotorbase/objectcontainer.h"
#include "src/engines/kotorbase/types.h"

#include "src/engines/kotor/encounters_end.h"

namespace Engines {

namespace KotOR {

static KotORBase::Creature *findTraskUlgo(KotORBase::Module &module) {
	KotORBase::Area *area = module.getCurrentArea();
	if (!area)
		return nullptr;

	if (KotORBase::Object *o = area->getObjectByTag("end_trask"))
		return KotORBase::ObjectContainer::toCreature(o);
	if (KotORBase::Object *o = area->getObjectByTag("trask"))
		return KotORBase::ObjectContainer::toCreature(o);

	for (KotORBase::Creature *c : area->getCreatures()) {
		if (c->getTemplateResRef() == "end_trask")
			return c;
	}

	return nullptr;
}

static void addSithSoldier(KotORBase::Area *area, KotORBase::Module &module,
                           float x, float y, float z, float angle) {
	std::unique_ptr<KotORBase::Creature> sith(module.createCreatureByTemplate("n_sithsoldier001"));
	if (!sith)
		return;

	sith->setPosition(x, y, z);
	sith->setOrientation(0.0f, 0.0f, 1.0f, angle);
	sith->setFaction(KotORBase::kFactionEndarSpire);
	sith->setAIArchetype(KotORBase::Creature::kAIArchetypeTacticalHumanoid);

	if (area)
		area->addCreature(sith.release());
}

void performEndarSpireOpening(KotORBase::Module &module) {
	status("Orchestrating Endar Spire Opening...");

	module.playMovie("swlogo");
	module.playMovie("openingcrawl");
	module.setGlobalBoolean("__endar_opening_played", true);
	module.setGlobalBoolean("__endar_opening_pending", true);

	if (!module.isLoaded()) {
		Common::UString firstModule = ConfigMan.getString("KOTOR_startModule", "end_m01aa");
		if (firstModule.empty())
			firstModule = "end_m01aa";
		module.load(firstModule);
	}
}

void performEndarSpireOpeningBeat(KotORBase::Module &module) {
	status("Endar Spire: attack on the Republic cruiser...");

	module.setCutsceneMode(true);
	module.setPlayerInputEnabled(false);
	module.resetToOrbit();

	module.playMusicStinger("mus_bat_ship");
	module.shakeCamera(4.0f, 0.8f);
	module.addJournalQuestEntry("k_main_quest", 5);
	module.runCinematicBeat(4.5f);

	module.restoreGameplayCamera(1.0f);
	module.runCinematicBeat(1.0f);

	module.setCutsceneMode(false);
	module.setPlayerInputEnabled(true);

	if (KotORBase::Area *area = module.getCurrentArea())
		area->playAmbientMusic();
}

void performTraskEncounter(KotORBase::Module &module) {
	status("Orchestrating Trask Ulgo reveal...");

	module.setCutsceneMode(true);
	module.setPlayerInputEnabled(false);

	module.playMusicStinger("mus_vfx_explosion");
	module.cameraTransitionToTarget("wp_trask_reveal", 2.0f);

	if (KotORBase::Creature *trask = findTraskUlgo(module)) {
		module.setCameraMode(KotORBase::kCameraModeMedium, trask);
		module.setCinematicFocus(trask);
	}

	module.shakeCamera(1.5f, 0.4f);
	module.runCinematicBeat(2.5f);

	module.restoreGameplayCamera(1.0f);
	module.runCinematicBeat(1.0f);

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

	float x = 0.0f, y = 0.0f, z = 0.0f, angle = 0.0f;
	if (module.getObjectLocation("wp_sector_2_airlock", KotORBase::kObjectTypeWaypoint, x, y, z, angle)) {
		addSithSoldier(area, module, x + 1.0f, y, z, angle);
		addSithSoldier(area, module, x - 1.0f, y, z, angle);
	}

	module.playMusicStinger("mus_bat_sith");
	module.runCinematicBeat(3.0f);

	module.restoreGameplayCamera(1.5f);
	module.runCinematicBeat(1.5f);

	module.setCutsceneMode(false);
	module.setPlayerInputEnabled(true);
}

} // End of namespace KotOR

} // End of namespace Engines
