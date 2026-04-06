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
 *  The character level-up GUI.
 */

#include "src/engines/aurora/widget.h"

#include "src/engines/kotorbase/module.h"
#include "src/engines/kotorbase/creature.h"

#include "src/engines/kotor/gui/ingame/levelup.h"

namespace Engines {

namespace KotOR {

LevelUpGUI::LevelUpGUI(KotORBase::Module &module, KotORBase::Creature &pc, ::Engines::Console *console) :
		KotORBase::GUI(console),
		_module(module),
		_pc(pc) {

	load("levelpnl");

	addBackground(KotORBase::kBackgroundTypeMenu);
}

LevelUpGUI::~LevelUpGUI() {
}

void LevelUpGUI::callbackActive(::Engines::Widget &widget) {
	if (widget.getTag() == "BTN_CANCEL") {
		_returnCode = 1;
		return;
	}

	if (widget.getTag() == "BTN_ACCEPT") {
		// Minimum parity implementation: perform the same logic as the old auto-level-up script stub.
		// In a full implementation, this GUI would have its own step-based logic (skills, feats, etc.)
		// like character generation.
		
		int currentLevel = _pc.getHitDice();
		KotORBase::CreatureInfo &info = _pc.getCreatureInfo();
		
		if (info.getNumClasses() > 0) {
			KotORBase::Class pcClass = info.getClassByPosition(0);
			info.incrementClassLevel(pcClass);

			// HP gain: 6 for scoundrel, 10 for soldier, 8 for scout + CON modifier.
			int hpGain = 10;
			if (pcClass == KotORBase::kClassScout)     hpGain = 8;
			if (pcClass == KotORBase::kClassScoundrel) hpGain = 6;
			hpGain += info.getAbilityModifier(KotORBase::kAbilityConstitution);
			if (hpGain < 1) hpGain = 1;

			_pc.setMaxHitPoints(_pc.getMaxHitPoints() + hpGain);
			_pc.setCurrentHitPoints(_pc.getMaxHitPoints());
		}

		_returnCode = 1;
		return;
	}
}

} // End of namespace KotOR

} // End of namespace Engines
