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
#include "src/engines/kotorbase/levelup.h"

#include "src/engines/kotor/gui/ingame/levelup.h"
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
	updateSummaryLabels();
}

LevelUpGUI::~LevelUpGUI() {
}

void LevelUpGUI::updateSummaryLabels() {
	populateCharacterSheet(_pc);

	const int nextLevel = _pc.getHitDice() + 1;
	const Common::UString nextLevelText = Common::composeString(nextLevel);
	setWidgetText("LBL_LEVEL_VAL", nextLevelText);
	setWidgetText("LBL_LEVEL", nextLevelText);
}

void LevelUpGUI::callbackActive(::Engines::Widget &widget) {
	if (widget.getTag() == "BTN_CANCEL") {
		_returnCode = 1;
		return;
	}

	if (widget.getTag() == "BTN_ACCEPT") {
		if (!KotORBase::canLevelUp(_pc)) {
			_returnCode = 1;
			return;
		}

		_step = 1;
		callbackRun();
		return;
	}
}

void LevelUpGUI::callbackRun() {
	if (_step == 0)
		return;

	const int totalLevel = _pc.getHitDice();

	switch (_step) {
	case 1:
		_step++;
		if (KotORBase::grantsAbilityIncrease(totalLevel)) {
			if (!showAbilities()) {
				_step = 0;
				return;
			}
		}
		// Fall through
	case 2:
		_step++;
		if (!showSkills()) {
			_step = 0;
			return;
		}
		// Fall through
	case 3:
		_step++;
		if (!KotORBase::getSelectableFeats(_pc.getCreatureInfo()).empty()) {
			if (!showFeats()) {
				_step = 0;
				return;
			}
		}
		// Fall through
	case 4:
		_step++;
		if (_pc.getCreatureInfo().isJedi() &&
		    !KotORBase::getSelectableForcePowers(_pc.getCreatureInfo()).empty()) {
			if (!showForcePowers()) {
				_step = 0;
				return;
			}
		}
		// Fall through
	case 5:
		finalizeLevelUp();
		updateSummaryLabels();
		_step = 0;
		_returnCode = 1;
		break;
	}
}

void LevelUpGUI::finalizeLevelUp() {
	KotORBase::applyLevelUp(_pc);
}

bool LevelUpGUI::showAbilities() {
	LevelUpAbilitiesMenu menu(_pc.getCreatureInfo(), _console);
	sub(menu);
	return menu.isAccepted();
}

bool LevelUpGUI::showSkills() {
	LevelUpSkillsMenu menu(_pc.getCreatureInfo(), _console);
	sub(menu);
	return menu.isAccepted();
}

bool LevelUpGUI::showFeats() {
	LevelUpFeatsMenu menu(_pc.getCreatureInfo(), _console);
	sub(menu);
	return menu.isAccepted();
}

bool LevelUpGUI::showForcePowers() {
	LevelUpForcePowersMenu menu(_pc.getCreatureInfo(), _console);
	sub(menu);
	return menu.isAccepted();
}

} // End of namespace KotOR

} // End of namespace Engines
