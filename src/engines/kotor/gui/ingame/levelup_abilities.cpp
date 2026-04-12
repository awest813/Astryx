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
 *  The ability score allocation menu for character level-up.
 */

#include "src/common/util.h"
#include "src/common/strutil.h"

#include "src/engines/odyssey/button.h"
#include "src/engines/odyssey/label.h"

#include "src/engines/kotor/gui/ingame/levelup_abilities.h"

namespace Engines {

namespace KotOR {

LevelUpAbilitiesMenu::LevelUpAbilitiesMenu(KotORBase::CreatureInfo &info, Console *console) :
		KotORBase::GUI(console),
		_info(info),
		_remainingPoints(1),
		_accepted(false) {

	try {
		load("abchrgen");
	} catch (...) {
		load("abitems");
	}

	addBackground(KotORBase::kBackgroundTypeMenu);

	_str  = _originalStr  = info.getAbilityScore(KotORBase::kAbilityStrength);
	_dex  = _originalDex  = info.getAbilityScore(KotORBase::kAbilityDexterity);
	_con  = _originalCon  = info.getAbilityScore(KotORBase::kAbilityConstitution);
	_intl = _originalIntl = info.getAbilityScore(KotORBase::kAbilityIntelligence);
	_wis  = _originalWis  = info.getAbilityScore(KotORBase::kAbilityWisdom);
	_cha  = _originalCha  = info.getAbilityScore(KotORBase::kAbilityCharisma);

	updateLabels();
}

LevelUpAbilitiesMenu::~LevelUpAbilitiesMenu() {
}

void LevelUpAbilitiesMenu::updateLabels() {
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

	setWidgetText("STR_POINTS_BTN", Common::composeString(_str));
	setWidgetText("DEX_POINTS_BTN", Common::composeString(_dex));
	setWidgetText("CON_POINTS_BTN", Common::composeString(_con));
	setWidgetText("INT_POINTS_BTN", Common::composeString(_intl));
	setWidgetText("WIS_POINTS_BTN", Common::composeString(_wis));
	setWidgetText("CHA_POINTS_BTN", Common::composeString(_cha));

	setWidgetText("REMAINING_SELECTIONS_LBL", Common::composeString(_remainingPoints));
}

void LevelUpAbilitiesMenu::callbackActive(Widget &widget) {
	const Common::UString &tag = widget.getTag();

	struct AbilityRef {
		const char *plusTag;
		const char *minusTag;
		uint32_t   *value;
		uint32_t    original;
	};

	static const AbilityRef kAbilityRefs[] = {
		{ "STR_PLUS_BTN", "STR_MINUS_BTN", &_str,  0 },
		{ "DEX_PLUS_BTN", "DEX_MINUS_BTN", &_dex,  0 },
		{ "CON_PLUS_BTN", "CON_MINUS_BTN", &_con,  0 },
		{ "INT_PLUS_BTN", "INT_MINUS_BTN", &_intl, 0 },
		{ "WIS_PLUS_BTN", "WIS_MINUS_BTN", &_wis,  0 },
		{ "CHA_PLUS_BTN", "CHA_MINUS_BTN", &_cha,  0 },
	};

	// Update original values in the static array (shady but works for this local loop)
	const_cast<AbilityRef&>(kAbilityRefs[0]).original = _originalStr;
	const_cast<AbilityRef&>(kAbilityRefs[1]).original = _originalDex;
	const_cast<AbilityRef&>(kAbilityRefs[2]).original = _originalCon;
	const_cast<AbilityRef&>(kAbilityRefs[3]).original = _originalIntl;
	const_cast<AbilityRef&>(kAbilityRefs[4]).original = _originalWis;
	const_cast<AbilityRef&>(kAbilityRefs[5]).original = _originalCha;

	for (size_t i = 0; i < ARRAYSIZE(kAbilityRefs); ++i) {
		if (tag == kAbilityRefs[i].plusTag) {
			if (_remainingPoints > 0) {
				_remainingPoints--;
				(*kAbilityRefs[i].value)++;
				updateLabels();
			}
			return;
		}
		if (tag == kAbilityRefs[i].minusTag) {
			if (*kAbilityRefs[i].value > kAbilityRefs[i].original) {
				_remainingPoints++;
				(*kAbilityRefs[i].value)--;
				updateLabels();
			}
			return;
		}
	}

	if (tag == "BTN_BACK") {
		_returnCode = 1;
		return;
	}

	if (tag == "BTN_ACCEPT") {
		if (_remainingPoints == 0) {
			_info.setAbilityScore(KotORBase::kAbilityStrength,     _str);
			_info.setAbilityScore(KotORBase::kAbilityDexterity,    _dex);
			_info.setAbilityScore(KotORBase::kAbilityConstitution, _con);
			_info.setAbilityScore(KotORBase::kAbilityIntelligence, _intl);
			_info.setAbilityScore(KotORBase::kAbilityWisdom,       _wis);
			_info.setAbilityScore(KotORBase::kAbilityCharisma,     _cha);
			_accepted = true;
			_returnCode = 1;
		}
		return;
	}
}

} // End of namespace KotOR

} // End of namespace Engines
