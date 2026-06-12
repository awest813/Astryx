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
 *  The panel to customize a quick character.
 */

#include "src/common/strutil.h"
#include "src/common/util.h"

#include "src/graphics/graphics.h"

#include "src/engines/odyssey/label.h"
#include "src/engines/odyssey/button.h"

#include "src/engines/kotor/gui/chargen/customchar.h"

namespace Engines {

namespace KotOR {

CustomCharPanel::CustomCharPanel(CharacterGenerationMenu *charGenMenu, Console *console) :
		KotORBase::GUI(console),
		_charGen(charGenMenu) {

	load("custpnl");

	setPosition(137, 16, 0);

	float width = getLabel("LBL_BG")->getWidth();
	float height = getLabel("LBL_BG")->getHeight();

	getLabel("LBL_BG")->setScissor(5, 40, width - 5, height - 60);

	for (int i = 1; i <= 6; ++i) {
		if (Odyssey::WidgetButton *btn = getButton(Common::UString("BTN_STEPNAME") + Common::composeString(i)))
			btn->setDisableHoverSound(true);
	}

	updateButtons();
}

void CustomCharPanel::updateButtons() {
	static const char * const kStepTags[] = {
		"BTN_STEPNAME1", "BTN_STEPNAME2", "BTN_STEPNAME3",
		"BTN_STEPNAME4", "BTN_STEPNAME5", "BTN_STEPNAME6"
	};

	const int step = _charGen->getStep();

	for (size_t i = 0; i < ARRAYSIZE(kStepTags); ++i) {
		Odyssey::WidgetButton *btn = getButton(kStepTags[i]);
		if (!btn)
			continue;

		const bool completed = static_cast<int>(i) < step;
		const bool active = static_cast<int>(i) == step;

		btn->setDisabled(!completed && !active);
		btn->setPermanentHighlight(active);
		btn->setDisableHighlight(!active);
	}

	if (Widget *back = getWidget("BTN_BACK"))
		back->setDisabled(step == 0);
}

void CustomCharPanel::callbackActive(Widget &widget) {
	if (widget.getTag() == "BTN_CANCEL") {
		_charGen->showQuickOrCustom();
		return;
	}

	if (widget.getTag() == "BTN_BACK") {
		_charGen->decStep();
		return;
	}

	if (widget.getTag() == "BTN_STEPNAME1") {
		_charGen->showAbilities();
		return;
	}

	if (widget.getTag() == "BTN_STEPNAME2") {
		_charGen->showSkills();
		return;
	}

	if (widget.getTag() == "BTN_STEPNAME3") {
		_charGen->showFeats();
		return;
	}

	if (widget.getTag() == "BTN_STEPNAME4") {
		_charGen->showPortrait();
		return;
	}

	if (widget.getTag() == "BTN_STEPNAME5") {
		_charGen->showName();
		return;
	}

	if (widget.getTag() == "BTN_STEPNAME6") {
		if (_charGen->getStep() < 5)
			return;

		_charGen->start();
		GfxMan.lockFrame();
		_returnCode = 2;
		return;
	}
}

} // End of namespace KotOR

} // End of namespace Engines
