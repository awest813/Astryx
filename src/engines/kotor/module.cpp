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

#include "src/engines/kotor/module.h"
#include "src/engines/kotor/creature.h"

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

namespace Engines {

namespace KotOR {

Module::Module(::Engines::Console &console) : KotORBase::Module(console) {
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

void Module::showGalaxyMap() {
	GalaxyMapMenu gui(*this, &_console);
	sub(gui);
}

void Module::showWorkbench() {
	WorkbenchMenu gui(*this, &_console);
	sub(gui);
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
		setGlobalBoolean("__open_galaxymap", true);
	}
}

void Module::showJournal() {
	MenuJournal gui(*this, &_console); // Assuming MenuJournal constructor matches
	sub(gui);
}

} // End of namespace KotOR

} // End of namespace Engines
