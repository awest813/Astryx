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

#include <algorithm>

#include "src/common/strutil.h"
#include "src/common/util.h"

#include "src/engines/odyssey/button.h"
#include "src/engines/odyssey/label.h"
#include "src/engines/odyssey/listbox.h"

#include "src/engines/kotorbase/levelup.h"

#include "src/engines/kotor/gui/ingame/levelup_feats.h"

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

/** Rows of the feat list box. */
static const uint32_t kFeatListRows = 8;

LevelUpFeatsMenu::LevelUpFeatsMenu(KotORBase::CreatureInfo &info, Console *console) :
		KotORBase::GUI(console),
		_info(info),
		_picks(KotORBase::featsForNextLevel(info)),
		_accepted(false) {

	try {
		load("ftchrgen");
	} catch (...) {
		load("featpnl");
	}

	addBackground(KotORBase::kBackgroundTypeMenu);

	_availableFeats = KotORBase::getSelectableFeats(_info);

	// The retail panel lists the feats in a list box (KotOR.js: LB_FEATS).
	if (Odyssey::WidgetListBox *list = getListBox("LB_FEATS")) {
		list->setItemSelectionEnabled(true);
		list->setHideScrollbar(false);
		list->createItemWidgets(kFeatListRows);
	}

	updateLabels();
}

LevelUpFeatsMenu::~LevelUpFeatsMenu() {
}

void LevelUpFeatsMenu::fillFeatList() {
	Odyssey::WidgetListBox *list = getListBox("LB_FEATS");
	if (!list)
		return;

	const int selected = list->getSelectedIndex();

	list->removeAllItems();
	for (uint32_t feat : _availableFeats)
		list->addItem((isFeatSelected(feat) ? "* " : "") + KotORBase::getFeatDisplayName(feat));

	list->selectItemByIndex(selected);
	list->refreshItemWidgets();
}

void LevelUpFeatsMenu::updateLabels() {
	const int remaining = MAX<int>(_picks - static_cast<int>(_selectedFeats.size()), 0);
	setWidgetText("REMAINING_SELECTIONS_LBL", Common::composeString(remaining));
	setWidgetText("STD_REMAINING_SELECTIONS_LBL", Common::composeString(remaining));

	fillFeatList();

	for (size_t i = 0; i < ARRAYSIZE(kFeatButtons); ++i) {
		Odyssey::WidgetButton *button = getButton(kFeatButtons[i].tag);
		if (!button)
			continue;

		const bool available = isFeatAvailable(kFeatButtons[i].feat);
		button->setInvisible(!available);
		if (!available) {
			button->hide();
			continue;
		}

		if (isFeatSelected(kFeatButtons[i].feat))
			button->setStaticHighlight();
		else
			button->setPermanentHighlight(false);

		button->show();
	}
}

bool LevelUpFeatsMenu::isFeatAvailable(uint32_t feat) const {
	for (uint32_t available : _availableFeats) {
		if (available == feat)
			return true;
	}
	return false;
}

bool LevelUpFeatsMenu::isFeatSelected(uint32_t feat) const {
	return std::find(_selectedFeats.begin(), _selectedFeats.end(), feat) != _selectedFeats.end();
}

void LevelUpFeatsMenu::toggleFeat(uint32_t feat) {
	auto selected = std::find(_selectedFeats.begin(), _selectedFeats.end(), feat);
	if (selected != _selectedFeats.end())
		_selectedFeats.erase(selected);
	else if (isFeatAvailable(feat) && static_cast<int>(_selectedFeats.size()) < _picks)
		_selectedFeats.push_back(feat);
}

void LevelUpFeatsMenu::callbackActive(Widget &widget) {
	const Common::UString &tag = widget.getTag();

	for (size_t i = 0; i < ARRAYSIZE(kFeatButtons); ++i) {
		if (tag != kFeatButtons[i].tag)
			continue;

		toggleFeat(kFeatButtons[i].feat);
		updateLabels();
		return;
	}

	if (tag == "BTN_BACK") {
		_returnCode = 1;
		return;
	}

	if (tag == "BTN_SELECT") {
		// Toggle the feat highlighted in the list.
		Odyssey::WidgetListBox *list = getListBox("LB_FEATS");
		const int index = list ? list->getSelectedIndex() : -1;
		if (index >= 0 && static_cast<size_t>(index) < _availableFeats.size()) {
			toggleFeat(_availableFeats[index]);
			setWidgetText("LBL_NAME", KotORBase::getFeatDisplayName(_availableFeats[index]));
			updateLabels();
			return;
		}
	}

	if (tag == "BTN_RECOMMENDED" || tag == "BTN_SELECT") {
		_selectedFeats = KotORBase::completeLevelUpPicks(_selectedFeats, _availableFeats, _picks);
		updateLabels();
		return;
	}

	if (tag == "BTN_ACCEPT") {
		// The retail panel may not carry a button per feat; never let that
		// block the level-up: unspent picks go to the recommended feats.
		for (uint32_t feat : KotORBase::completeLevelUpPicks(_selectedFeats, _availableFeats, _picks))
			_info.addFeat(feat);

		_accepted = true;
		_returnCode = 1;
		return;
	}

	updateLabels();
}

void LevelUpFeatsMenu::callbackKeyInput(const Events::Key &key, const Events::EventType &type) {
	Odyssey::WidgetListBox *list = getListBox("LB_FEATS");
	if (!list || type != Events::kEventKeyDown)
		return;

	if (key == Events::kKeyUp)
		list->selectPreviousItem();
	else if (key == Events::kKeyDown)
		list->selectNextItem();
}

} // End of namespace KotOR

} // End of namespace Engines
