/* xoreos - A reimplementation of BioWare's Aurora engine
 *
 * xoreos is the legal property of its developers, whose names
 * can be found in the AUTHORS file distributed with this source
 * distribution.
 *
 * xoreos is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * xoreos is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with xoreos. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  The context needed to run a Star Wars: Knights of the Old Republic module.
 */

#include <algorithm>

#include "src/common/strutil.h"

#include "src/engines/kotor/module.h"
#include "src/engines/kotor/game.h"
#include "src/engines/kotor/creature.h"
#include "src/engines/kotorbase/creature.h"

#include "src/engines/kotor/gui/dialog.h"

#include "src/engines/kotor/gui/ingame/ingame.h"
#include "src/engines/kotor/gui/ingame/partyselection.h"

#include "src/engines/kotor/gui/loadscreen/loadscreen.h"

#include "src/engines/kotor/gui/chargen/chargeninfo.h"

#include "src/engines/kotor/gui/ingame/menu_jou.h"
#include "src/engines/kotor/gui/ingame/galaxymap.h"
#include "src/engines/kotor/gui/ingame/workbench.h"
#include "src/engines/kotor/encounters_dan.h"
#include "src/engines/kotor/encounters_end.h"
#include "src/engines/kotor/encounters_tar.h"
#include "src/engines/kotor/encounters_ebon.h"
#include "src/engines/kotor/pazaak.h"
#include "src/engines/kotor/gui/ingame/pazaak.h"
#include "src/engines/kotorbase/swoopminigame.h"
#include "src/engines/kotorbase/area.h"
#include "src/engines/kotor/gui/ingame/menu.h"

#include "src/graphics/graphics.h"

namespace Engines {

namespace KotOR {

Module::Module(Game &game, ::Engines::Console &console) : KotORBase::Module(game, console) {
	_ingame = std::make_unique<IngameGUI>(*this);
	_dialog = std::make_unique<DialogGUI>(*this);
	_partySelection = std::make_unique<PartySelectionGUI>();
}

KotORBase::LoadScreen *Module::createLoadScreen(const Common::UString &name) const {
	return new LoadScreen(name);
}

KotORBase::Creature *Module::createCreature(const Aurora::GFF3Struct &creature) const {
	return new Creature(creature);
}

KotORBase::Creature *Module::createCreature() const {
	return new Creature();
}

KotORBase::Creature *Module::createCreature(const Common::UString &resRef) const {
	return new Creature(resRef);
}

KotORBase::CharacterGenerationInfo *Module::createCharGenInfo(const KotORBase::CharacterGenerationInfo &info) const {
	return new CharacterGenerationInfo(info);
}

bool Module::deferIngameHUDOnEnter() const {
	return _playOpeningBeat;
}

void Module::enter() {
	_playOpeningBeat = getGlobalBoolean("__endar_opening_pending");
	if (_playOpeningBeat)
		setGlobalBoolean("__endar_opening_pending", false);

	KotORBase::Module::enter();

	if (_playOpeningBeat) {
		KotOR::performEndarSpireOpeningBeat(*this);
		_playOpeningBeat = false;
	}
}

void Module::showMenu() {
	showIngameOptionsMenu();
}

void Module::showDeathGUI() {
	showIngameOptionsMenu();
}

void Module::showGUIPanel(int panel) {
	switch (panel) {
	case 4: { // GUI_PANEL_INVENTORY
		Menu menu(*this, _console);
		menu.show();
		menu.showMenu("BTN_INV");
		menu.run();
		break;
	}
	case 6: // GUI_PANEL_JOURNAL
		showJournal();
		break;
	case 9: // GUI_PANEL_LEVELUP
		getGame().showLevelUpGUI();
		break;
	default:
		warning("Module::showGUIPanel(%d): unhandled panel", panel);
		break;
	}
}

void Module::showIngameOptionsMenu() {
	if (_inDialog || !_ingame || !_running)
		return;

	_cameraController.stopMovement();
	_partyLeaderController.clearUserInput();
	_partyLeaderController.stopMovement();
	_ingame->hideSelection();

	Menu menu(*this, _console);
	menu.show();
	menu.showMenu("BTN_OPT");

	updateFrameTimestamp();
	const uint32_t ret = menu.run();
	if (ret == 1)
		_exit = true;
	else if (ret == 2)
		GfxMan.unlockFrame();

	updateFrameTimestamp();
}

void Module::showGalaxyMap() {
	GalaxyMapMenu gui(*this, _console);
	gui.run();
}

void Module::showWorkbench() {
	WorkbenchMenu gui(*this, _console);
	gui.run();
}

void Module::signalEncounter(const Common::UString &id) {
	if (id == "end_opening") {
		KotOR::performEndarSpireOpening(*this);
	} else if (id == "end_trask") {
		KotOR::performTraskEncounter(*this);
	} else if (id == "end_sith_board") {
		KotOR::performSithBoarding(*this);
	} else if (id == "tar_ambush") {
		KotOR::performTarisAmbush(*this);
	} else if (id == "tar_duel_intro") {
		KotOR::performDuelRingIntro(*this);
	} else if (id == "dan14_mand") {
		KotOR::performMandalorianAmbush(*this);
	} else if (id == "dan14_kinrath") {
		KotOR::performKinrathSwarm(*this);
	} else if (id == "dan17_reveal") {
		KotOR::performStarMapReveal(*this);
	} else if (id == "ebon_galaxymap") {
		showGalaxyMap();
	} else if (id == "ebon_hyperspace") {
		performHyperspaceJump(*this);
	} else if (id == "ebon_arrival") {
		performPlanetArrival(*this);
	} else if (id == "ebon_turret") {
		performTurretMinigame(*this);
	} else if (id == "turret_combat_start") {
		setGlobalBoolean("__swmg_gunbank_targeting", true);
	} else if (id == "swmg_obstacle_hit") {
		setGlobalNumber("__swmg_last_event", KotORBase::SwoopMinigame::get().getLastEvent());
	} else if (id == "tar_brejik_post_race") {
		KotOR::performBrejikShowdown(*this);
	} else if (id == "pazaak_start") {
		const int sideIndex = getGlobalNumber("__pazaak_side");
		const std::vector<int> playerDeck = PazaakEngine::sideDeckForIndex(sideIndex);
		const std::vector<int> opponentDeck = {1, 2, 3, 4, 5, 6, kPazaakCardFlip};

		PazaakEngine engine;
		engine.startMatch(playerDeck, opponentDeck);

		PazaakGUI gui(engine, _console);
		gui.run();

		const bool playerWon = engine.getWinner() == 1;
		setGlobalNumber("__pazaak_result", playerWon ? 1 : 0);

		const int wager = getGlobalNumber("__pazaak_wager");
		if (wager > 0 && getPC()) {
			KotORBase::Inventory &inv = getPC()->getInventory();
			if (playerWon) {
				inv.addGold(static_cast<uint32_t>(wager));
			} else {
				const uint32_t paid = std::min(static_cast<uint32_t>(wager), inv.getGold());
				inv.removeGold(paid);
			}
		}
	}
}

void Module::onFrameUpdate(float frameTime) {
	const bool swoopModule = _module.endsWith("mg");
	const bool areaMinigame = _area && _area->isMinigame();
	KotORBase::SwoopMinigame &swoop = KotORBase::SwoopMinigame::get();

	swoop.setActive(swoopModule || areaMinigame);
	if (swoop.isActive())
		swoop.update(frameTime);
}

void Module::showJournal() {
	MenuJournal gui(_console);
	gui.setModule(this);
	gui.show();
	gui.run();
}

} // End of namespace KotOR

} // End of namespace Engines
