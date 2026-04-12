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
 *  The feat selection menu for character level-up.
 */

#include "src/common/strutil.h"

#include "src/engines/odyssey/button.h"
#include "src/engines/odyssey/label.h"

#include "src/engines/kotor/gui/ingame/levelup_feats.h"

namespace Engines {

namespace KotOR {

LevelUpFeatsMenu::LevelUpFeatsMenu(KotORBase::CreatureInfo &info, Console *console) :
		KotORBase::GUI(console),
		_info(info),
		_selectedFeat(0xFFFFFFFF),
		_accepted(false) {

	try {
		load("ftchrgen");
	} catch (...) {
		load("featpnl");
	}

	addBackground(KotORBase::kBackgroundTypeMenu);

	// Placeholder feats for Phase 5.1
	_availableFeats.push_back(KotORBase::kFeatPowerAttack);
	_availableFeats.push_back(KotORBase::kFeatFlurry);
	_availableFeats.push_back(KotORBase::kFeatCriticalStrike);

	updateLabels();
}

LevelUpFeatsMenu::~LevelUpFeatsMenu() {
}

void LevelUpFeatsMenu::updateLabels() {
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

	setWidgetText("REMAINING_SELECTIONS_LBL", (_selectedFeat == 0xFFFFFFFF) ? "1" : "0");
}

void LevelUpFeatsMenu::callbackActive(Widget &widget) {
	const Common::UString &tag = widget.getTag();

	// Basic tag mapping for placeholder feats
	if (tag == "BTN_POWER_ATTACK") {
		_selectedFeat = KotORBase::kFeatPowerAttack;
	} else if (tag == "BTN_FLURRY") {
		_selectedFeat = KotORBase::kFeatFlurry;
	} else if (tag == "BTN_CRITICAL") {
		_selectedFeat = KotORBase::kFeatCriticalStrike;
	}

	if (tag == "BTN_BACK") {
		_returnCode = 1;
		return;
	}

	if (tag == "BTN_ACCEPT") {
		if (_selectedFeat != 0xFFFFFFFF) {
			_info.addFeat(_selectedFeat);
			_accepted = true;
			_returnCode = 1;
		}
		return;
	}

	updateLabels();
}

} // End of namespace KotOR

} // End of namespace Engines
