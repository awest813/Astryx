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
#include "src/common/util.h"

#include "src/engines/odyssey/button.h"
#include "src/engines/odyssey/label.h"

#include "src/engines/kotorbase/levelup.h"

#include "src/engines/kotor/gui/ingame/levelup_forcepowers.h"

namespace Engines {

namespace KotOR {

static const char *kPowerButtonTags[] = {
	"BTN_POWER_1",
	"BTN_POWER_2",
	"BTN_POWER_3",
	"BTN_POWER_4",
	"BTN_POWER_5",
	"BTN_POWER_6",
};

static const char *kLegacyPowerButtonTags[] = {
	"BTN_FORCE_1",
	"BTN_FORCE_2",
	"BTN_FORCE_3",
	"BTN_FORCE_4",
	"BTN_FORCE_5",
	"BTN_FORCE_6",
};

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

	_availablePowers = KotORBase::getSelectableForcePowers(_info);
	if (_availablePowers.empty())
		_availablePowers.push_back(1);

	updateLabels();
}

LevelUpForcePowersMenu::~LevelUpForcePowersMenu() {
}

bool LevelUpForcePowersMenu::isPowerAvailable(uint32_t power) const {
	for (uint32_t available : _availablePowers) {
		if (available == power)
			return true;
	}
	return false;
}

void LevelUpForcePowersMenu::updateLabels() {
	setWidgetText("REMAINING_SELECTIONS_LBL", (_selectedPower == 0xFFFFFFFF) ? "1" : "0");

	for (size_t i = 0; i < ARRAYSIZE(kPowerButtonTags); ++i) {
		const bool hasPower = i < _availablePowers.size();
		const uint32_t powerId = hasPower ? _availablePowers[i] : 0;
		const Common::UString name = hasPower ? KotORBase::getForcePowerDisplayName(powerId) : "";

		setWidgetText(kPowerButtonTags[i], name);
		if (i < ARRAYSIZE(kLegacyPowerButtonTags))
			setWidgetText(kLegacyPowerButtonTags[i], name);

		Odyssey::WidgetButton *button = getButton(kPowerButtonTags[i]);
		if (!button && i < ARRAYSIZE(kLegacyPowerButtonTags))
			button = getButton(kLegacyPowerButtonTags[i]);

		if (!button)
			continue;

		button->setInvisible(!hasPower);
		if (!hasPower) {
			button->hide();
			continue;
		}

		if (_selectedPower == powerId)
			button->setStaticHighlight();
		else
			button->setPermanentHighlight(false);

		button->show();
	}
}

void LevelUpForcePowersMenu::callbackActive(Widget &widget) {
	const Common::UString &tag = widget.getTag();

	for (size_t i = 0; i < ARRAYSIZE(kPowerButtonTags) && i < _availablePowers.size(); ++i) {
		if (tag == kPowerButtonTags[i] ||
		    (i < ARRAYSIZE(kLegacyPowerButtonTags) && tag == kLegacyPowerButtonTags[i])) {
			_selectedPower = _availablePowers[i];
			updateLabels();
			return;
		}
	}

	if (tag == "BTN_BACK") {
		_returnCode = 1;
		return;
	}

	if (tag == "BTN_ACCEPT") {
		if (_selectedPower != 0xFFFFFFFF && isPowerAvailable(_selectedPower)) {
			_info.addForcePower(_selectedPower);
			_accepted = true;
			_returnCode = 1;
		}
		return;
	}

	updateLabels();
}

} // End of namespace KotOR

} // End of namespace Engines
