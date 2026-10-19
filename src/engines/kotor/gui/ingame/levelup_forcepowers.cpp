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

#include <algorithm>

#include "src/common/strutil.h"
#include "src/common/util.h"

#include "src/engines/odyssey/button.h"
#include "src/engines/odyssey/label.h"
#include "src/engines/odyssey/listbox.h"

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

/** Rows of the power list box. */
static const uint32_t kPowerListRows = 8;

LevelUpForcePowersMenu::LevelUpForcePowersMenu(KotORBase::CreatureInfo &info, Console *console) :
		KotORBase::GUI(console),
		_info(info),
		_picks(KotORBase::forcePowersForNextLevel(info)),
		_accepted(false) {

	// KotOR.js loads "pwrlvlup" for the K1 power level-up panel.
	try {
		load("pwrlvlup");
	} catch (...) {
		try {
			load("fpchrgen");
		} catch (...) {
			load("forcepnl");
		}
	}

	addBackground(KotORBase::kBackgroundTypeMenu);

	_availablePowers = KotORBase::getSelectableForcePowers(_info);

	// The retail panel lists the powers in a list box (KotOR.js: LB_POWERS).
	if (Odyssey::WidgetListBox *list = getListBox("LB_POWERS")) {
		list->setItemSelectionEnabled(true);
		list->setHideScrollbar(false);
		list->createItemWidgets(kPowerListRows);
	}

	updateLabels();
}

LevelUpForcePowersMenu::~LevelUpForcePowersMenu() {
}

void LevelUpForcePowersMenu::fillPowerList() {
	Odyssey::WidgetListBox *list = getListBox("LB_POWERS");
	if (!list)
		return;

	const int selected = list->getSelectedIndex();

	list->removeAllItems();
	for (uint32_t power : _availablePowers)
		list->addItem((isPowerSelected(power) ? "* " : "") + KotORBase::getForcePowerDisplayName(power));

	list->selectItemByIndex(selected);
	list->refreshItemWidgets();
}

bool LevelUpForcePowersMenu::isPowerAvailable(uint32_t power) const {
	for (uint32_t available : _availablePowers) {
		if (available == power)
			return true;
	}
	return false;
}

bool LevelUpForcePowersMenu::isPowerSelected(uint32_t power) const {
	return std::find(_selectedPowers.begin(), _selectedPowers.end(), power) != _selectedPowers.end();
}

void LevelUpForcePowersMenu::togglePower(uint32_t power) {
	auto selected = std::find(_selectedPowers.begin(), _selectedPowers.end(), power);
	if (selected != _selectedPowers.end())
		_selectedPowers.erase(selected);
	else if (isPowerAvailable(power) && static_cast<int>(_selectedPowers.size()) < _picks)
		_selectedPowers.push_back(power);
}

void LevelUpForcePowersMenu::updateLabels() {
	const int remaining = MAX<int>(_picks - static_cast<int>(_selectedPowers.size()), 0);
	setWidgetText("REMAINING_SELECTIONS_LBL", Common::composeString(remaining));

	fillPowerList();

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

		if (isPowerSelected(powerId))
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
			togglePower(_availablePowers[i]);
			updateLabels();
			return;
		}
	}

	// "pwrlvlup" names its buttons "<NAME>_BTN" rather than "BTN_<NAME>".
	const bool back        = (tag == "BTN_BACK")        || (tag == "BACK_BTN");
	const bool select      = (tag == "BTN_SELECT")      || (tag == "SELECT_BTN");
	const bool recommended = (tag == "BTN_RECOMMENDED") || (tag == "RECOMMENDED_BTN");
	const bool accept      = (tag == "BTN_ACCEPT")      || (tag == "ACCEPT_BTN");

	if (back) {
		_returnCode = 1;
		return;
	}

	if (select) {
		// Toggle the power highlighted in the list.
		Odyssey::WidgetListBox *list = getListBox("LB_POWERS");
		const int index = list ? list->getSelectedIndex() : -1;
		if (index >= 0 && static_cast<size_t>(index) < _availablePowers.size()) {
			togglePower(_availablePowers[index]);
			setWidgetText("LBL_POWER", KotORBase::getForcePowerDisplayName(_availablePowers[index]));
			updateLabels();
			return;
		}
	}

	if (recommended || select) {
		_selectedPowers = KotORBase::completeLevelUpPicks(_selectedPowers, _availablePowers, _picks);
		updateLabels();
		return;
	}

	if (accept) {
		// The retail panel may not carry a button per power; never let that
		// block the level-up: unspent picks go to the recommended powers.
		for (uint32_t power : KotORBase::completeLevelUpPicks(_selectedPowers, _availablePowers, _picks))
			_info.addForcePower(power);

		_accepted = true;
		_returnCode = 1;
		return;
	}

	updateLabels();
}

void LevelUpForcePowersMenu::callbackKeyInput(const Events::Key &key, const Events::EventType &type) {
	Odyssey::WidgetListBox *list = getListBox("LB_POWERS");
	if (!list || type != Events::kEventKeyDown)
		return;

	if (key == Events::kKeyUp)
		list->selectPreviousItem();
	else if (key == Events::kKeyDown)
		list->selectNextItem();
}

} // End of namespace KotOR

} // End of namespace Engines
