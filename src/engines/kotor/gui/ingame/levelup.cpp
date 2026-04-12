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

#include "src/engines/kotor/gui/ingame/levelup_abilities.h"
#include "src/engines/kotor/gui/ingame/levelup_skills.h"
#include "src/engines/kotor/gui/ingame/levelup_feats.h"
#include "src/engines/kotor/gui/ingame/levelup_forcepowers.h"

namespace Engines {

namespace KotOR {

LevelUpGUI::LevelUpGUI(KotORBase::Module &module, KotORBase::Creature &pc, ::Engines::Console *console) :
		KotORBase::GUI(console),
		_module(module),
		_pc(pc),
		_step(0) {

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
		// New multi-step workflow
		
		int totalLevel = _pc.getHitDice();
		
		// 1. Ability Scores (every 4 levels)
		if (totalLevel % 4 == 0) {
			showAbilities();
		}

		// 2. Skills (always)
		showSkills();

		// 3. Feats (based on class progression — placeholder: always for now)
		showFeats();

		// 4. Force Powers (if Jedi — placeholder: always for now if class > 2)
		int classID = _pc.getCreatureInfo().getLatestClass();
		if (classID >= KotORBase::kClassJediGuardian && classID <= KotORBase::kClassJediSentinel) {
			showForcePowers();
		}

		// Finalize the level up
		KotORBase::CreatureInfo &info = _pc.getCreatureInfo();
		KotORBase::Class pcClass = info.getLatestClass();
		info.incrementClassLevel(pcClass);

		// HP gain: based on class hit die + CON modifier.
		int hpGain = 10;
		if (pcClass == KotORBase::kClassScout)          hpGain = 8;
		if (pcClass == KotORBase::kClassScoundrel)      hpGain = 6;
		if (pcClass == KotORBase::kClassJediGuardian)   hpGain = 10;
		if (pcClass == KotORBase::kClassJediSentinel)   hpGain = 8;
		if (pcClass == KotORBase::kClassJediConsular)   hpGain = 6;

		hpGain += info.getAbilityModifier(KotORBase::kAbilityConstitution);
		if (hpGain < 1) hpGain = 1;

		_pc.setMaxHitPoints(_pc.getMaxHitPoints() + hpGain);
		_pc.setCurrentHitPoints(_pc.getMaxHitPoints());

		_pc.setMaxForcePoints(_pc.computeMaxForcePoints());
		_pc.setForcePoints(_pc.getMaxForcePoints());

		_returnCode = 1;
		return;
	}
}

void LevelUpGUI::showAbilities() {
	LevelUpAbilitiesMenu menu(_pc.getCreatureInfo(), _console);
	sub(menu);
}

void LevelUpGUI::showSkills() {
	LevelUpSkillsMenu menu(_pc.getCreatureInfo(), _console);
	sub(menu);
}

void LevelUpGUI::showFeats() {
	LevelUpFeatsMenu menu(_pc.getCreatureInfo(), _console);
	sub(menu);
}

void LevelUpGUI::showForcePowers() {
	LevelUpForcePowersMenu menu(_pc.getCreatureInfo(), _console);
	sub(menu);
}

} // End of namespace KotOR

} // End of namespace Engines
