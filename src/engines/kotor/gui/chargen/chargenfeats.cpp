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
 *  The feat selection menu for custom character creation.
 */

#include "src/common/strutil.h"

#include "src/engines/odyssey/button.h"
#include "src/engines/odyssey/label.h"

#include "src/engines/kotorbase/gui/chargeninfo.h"
#include "src/engines/kotorbase/creatureinfo.h"

#include "src/engines/kotor/gui/chargen/chargenfeats.h"

namespace Engines {

namespace KotOR {

CharacterGenerationFeatsMenu::CharacterGenerationFeatsMenu(
		KotORBase::CharacterGenerationInfo &info,
		Console *console) :
		CharacterGenerationBaseMenu(info, console),
		_selectedFeat(0xFFFFFFFF) {

	try {
		load("ftchrgen");
	} catch (...) {
		// Fallback for some alternate versions of data
		load("featpnl");
	}

	addBackground(KotORBase::kBackgroundTypeMenu);

	// In a real implementation, we would populate available feats from feats.2da
	// based on the character's class, level, and ability requirements.
	// For this parity milestone, we just provide a basic choice.
	_availableFeats.push_back(KotORBase::kFeatPowerAttack);
	_availableFeats.push_back(KotORBase::kFeatFlurry);
	_availableFeats.push_back(KotORBase::kFeatCriticalStrike);

	updateLabels();
}

void CharacterGenerationFeatsMenu::updateLabels() {
	auto setWidgetText = [this](const char *tag, const Common::UString &text) {
		Odyssey::WidgetLabel *lbl = getLabel(tag);
		if (lbl)
			lbl->setText(text);

		Odyssey::WidgetButton *btn = getButton(tag);
		if (btn)
			btn->setText(text);
	};

	// Highlight selected feat if any.
	// We assume there's a label describing the selection.
	setWidgetText("REMAINING_SELECTIONS_LBL", (_selectedFeat == 0xFFFFFFFF) ? "1" : "0");
}

void CharacterGenerationFeatsMenu::callbackActive(Widget &widget) {
	const Common::UString &tag = widget.getTag();

	// In a real GUI, each feat would have a button in a list.
	// For this implementation, we map tags to feat choices.
	if (tag == "BTN_POWER_ATTACK") {
		_selectedFeat = KotORBase::kFeatPowerAttack;
		updateLabels();
		return;
	}
	if (tag == "BTN_FLURRY") {
		_selectedFeat = KotORBase::kFeatFlurry;
		updateLabels();
		return;
	}
	if (tag == "BTN_CRITICAL") {
		_selectedFeat = KotORBase::kFeatCriticalStrike;
		updateLabels();
		return;
	}

	if (tag == "BTN_RECOMMENDED") {
		_selectedFeat = KotORBase::kFeatPowerAttack;
		updateLabels();
		return;
	}

	if (tag == "BTN_BACK") {
		_returnCode = 1;
		return;
	}

	if (tag == "BTN_ACCEPT") {
		if (_selectedFeat != 0xFFFFFFFF) {
			_info.addFeat(_selectedFeat);
			accept();
			_returnCode = 1;
		}
		return;
	}
}

} // End of namespace KotOR

} // End of namespace Engines
