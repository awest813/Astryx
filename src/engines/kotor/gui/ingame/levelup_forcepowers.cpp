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
 *  The Force power selection menu for character level-up.
 */

#include "src/common/strutil.h"

#include "src/engines/odyssey/button.h"
#include "src/engines/odyssey/label.h"

#include "src/engines/kotor/gui/ingame/levelup_forcepowers.h"

namespace Engines {

namespace KotOR {

LevelUpForcePowersMenu::LevelUpForcePowersMenu(KotORBase::CreatureInfo &info, Console *console) :
		KotORBase::GUI(console),
		_info(info),
		_selectedPower(0xFFFFFFFF),
		_accepted(false) {

	try {
		load("fpchrgen");
	} catch (...) {
		load("forcepnl");
	}

	addBackground(KotORBase::kBackgroundTypeMenu);

	// Placeholder Force Powers for Phase 5.1
	// These IDs are placeholders; full implementation requires spells.2da lookup.
	_availablePowers.push_back(1); // Force Heal
	_availablePowers.push_back(2); // Force Push
	_availablePowers.push_back(3); // Burst of Speed

	updateLabels();
}

LevelUpForcePowersMenu::~LevelUpForcePowersMenu() {
}

void LevelUpForcePowersMenu::updateLabels() {
	auto setWidgetText = [this](const char *tag, const Common::UString &text) {
		Odyssey::WidgetLabel *lbl = getLabel(tag);
		if (lbl) {
			lbl->setText(text);
			return;
		}
		Odyssey::WidgetButton *btn = getButton(tag);
		if (btn)
			btn->setText(text);
	};

	setWidgetText("REMAINING_SELECTIONS_LBL", (_selectedPower == 0xFFFFFFFF) ? "1" : "0");
}

void LevelUpForcePowersMenu::callbackActive(Widget &widget) {
	const Common::UString &tag = widget.getTag();

	if (tag == "BTN_POWER_1") {
		_selectedPower = 1;
	} else if (tag == "BTN_POWER_2") {
		_selectedPower = 2;
	} else if (tag == "BTN_POWER_3") {
		_selectedPower = 3;
	}

	if (tag == "BTN_BACK") {
		_returnCode = 1;
		return;
	}

	if (tag == "BTN_ACCEPT") {
		if (_selectedPower != 0xFFFFFFFF) {
			// In a real implementation, add the Force power to the character.
			_accepted = true;
			_returnCode = 1;
		}
		return;
	}

	updateLabels();
}

} // End of namespace KotOR

} // End of namespace Engines
