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
#include "src/common/util.h"

#include "src/engines/odyssey/button.h"
#include "src/engines/odyssey/label.h"

#include "src/engines/kotorbase/gui/chargeninfo.h"
#include "src/engines/kotorbase/levelup.h"

#include "src/engines/kotor/gui/chargen/chargenfeats.h"

namespace Engines {

namespace KotOR {

struct FeatButtonMap {
	const char *tag;
	uint32_t    feat;
};

static const FeatButtonMap kFeatButtons[] = {
	{ "BTN_POWER_ATTACK", KotORBase::kFeatPowerAttack },
	{ "BTN_FLURRY",       KotORBase::kFeatFlurry },
	{ "BTN_CRITICAL",     KotORBase::kFeatCriticalStrike },
	{ "BTN_POWER_BLAST",  KotORBase::kFeatPowerBlast },
	{ "BTN_RAPID_SHOT",   KotORBase::kFeatRapidShot },
	{ "BTN_SNIPER_SHOT",  KotORBase::kFeatSniperShot },
	{ "BTN_CONDITIONING", KotORBase::kFeatConditioning },
	{ "BTN_TOUGHNESS",    KotORBase::kFeatToughness },
	{ "BTN_JEDI_DEFENSE", KotORBase::kFeatJediDefense },
};

CharacterGenerationFeatsMenu::CharacterGenerationFeatsMenu(
		KotORBase::CharacterGenerationInfo &info,
		Console *console) :
		CharacterGenerationBaseMenu(info, console),
		_selectedFeat(0xFFFFFFFF) {

	try {
		load("ftchrgen");
	} catch (...) {
		load("featpnl");
	}

	addBackground(KotORBase::kBackgroundTypeMenu);

	_availableFeats = KotORBase::getSelectableFeats(info.getClass(), info.getFeats());
	if (_availableFeats.empty())
		_availableFeats.push_back(KotORBase::kFeatToughness);

	updateLabels();
}

void CharacterGenerationFeatsMenu::updateLabels() {
	setWidgetText("REMAINING_SELECTIONS_LBL", (_selectedFeat == 0xFFFFFFFF) ? "1" : "0");
}

bool CharacterGenerationFeatsMenu::isFeatAvailable(uint32_t feat) const {
	for (uint32_t available : _availableFeats) {
		if (available == feat)
			return true;
	}
	return false;
}

void CharacterGenerationFeatsMenu::callbackActive(Widget &widget) {
	const Common::UString &tag = widget.getTag();

	for (size_t i = 0; i < ARRAYSIZE(kFeatButtons); ++i) {
		if (tag != kFeatButtons[i].tag)
			continue;

		if (isFeatAvailable(kFeatButtons[i].feat))
			_selectedFeat = kFeatButtons[i].feat;
		updateLabels();
		return;
	}

	if (tag == "BTN_RECOMMENDED" && !_availableFeats.empty()) {
		_selectedFeat = _availableFeats.front();
		updateLabels();
		return;
	}

	if (tag == "BTN_BACK") {
		_returnCode = 1;
		return;
	}

	if (tag == "BTN_ACCEPT") {
		if (_selectedFeat != 0xFFFFFFFF && isFeatAvailable(_selectedFeat)) {
			_info.addFeat(_selectedFeat);
			accept();
			_returnCode = 1;
		}
		return;
	}
}

} // End of namespace KotOR

} // End of namespace Engines
