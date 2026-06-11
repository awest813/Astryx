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
 *  Pazaak minigame GUI for Star Wars: Knights of the Old Republic.
 */

#include "src/common/strutil.h"

#include "src/engines/odyssey/button.h"
#include "src/engines/odyssey/label.h"

#include "src/engines/kotor/pazaak.h"
#include "src/engines/kotor/gui/ingame/pazaak.h"

namespace Engines {
namespace KotOR {

static Common::UString formatCardValue(int val) {
	if (val == kPazaakCardFlip)
		return "FLIP";
	if (val == kPazaakCardDouble)
		return "x2";
	if (val > 0)
		return "+" + Common::composeString(val);
	return Common::composeString(val);
}

PazaakGUI::PazaakGUI(PazaakEngine &engine, Console *console) :
		KotORBase::GUI(console),
		_engine(engine) {

	load("pazaakpnl");
	addBackground(KotORBase::kBackgroundTypePazaak);

	updateUI();
}

PazaakGUI::~PazaakGUI() {}

void PazaakGUI::updateUI() {
	const bool matchOver = _engine.getState() == PazaakEngine::kStateMatchEnd;
	const bool playerTurn = _engine.getState() == PazaakEngine::kStatePlayerTurn;

	setWidgetText("LBL_PLAYER_SCORE", Common::composeString(_engine.getPlayer().score));
	setWidgetText("LBL_OPPONENT_SCORE", Common::composeString(_engine.getOpponent().score));
	setWidgetText("LBL_PLAYER_SETS", Common::composeString(_engine.getPlayer().setsWon));
	setWidgetText("LBL_OPPONENT_SETS", Common::composeString(_engine.getOpponent().setsWon));

	for (int i = 0; i < 4; ++i) {
		const Common::UString btnTag = Common::UString("BTN_HAND_") + Common::composeString(i);
		Odyssey::WidgetButton *btn = getButton(btnTag);
		if (!btn)
			continue;

		if (!matchOver && playerTurn && i < (int)_engine.getPlayer().hand.size()) {
			btn->setInvisible(false);
			setWidgetText(btnTag, formatCardValue(_engine.getPlayer().hand[i]));
			btn->show();
		} else {
			btn->hide();
		}
	}

	const bool canAct = playerTurn && !_engine.getPlayer().standing && !_engine.getPlayer().bust;

	if (Odyssey::WidgetButton *standBtn = getButton("BTN_STAND")) {
		standBtn->setDisabled(!canAct);
		standBtn->setInvisible(!canAct);
		if (canAct)
			standBtn->show();
		else
			standBtn->hide();
	}

	if (Odyssey::WidgetButton *drawBtn = getButton("BTN_END_TURN")) {
		drawBtn->setDisabled(!canAct);
		drawBtn->setInvisible(!canAct);
		if (canAct)
			drawBtn->show();
		else
			drawBtn->hide();
	}

	if (matchOver) {
		setWidgetText("LBL_STATUS", _engine.getWinner() == 1 ? "YOU WIN!" : "YOU LOSE!");
	} else if (_engine.getPlayer().standing) {
		setWidgetText("LBL_STATUS", "STANDING");
	} else if (_engine.getState() == PazaakEngine::kStateOpponentTurn) {
		setWidgetText("LBL_STATUS", "OPPONENT TURN");
	} else {
		setWidgetText("LBL_STATUS", "YOUR TURN");
	}
}

void PazaakGUI::callbackActive(Widget &widget) {
	const Common::UString &tag = widget.getTag();

	if (_engine.getState() == PazaakEngine::kStateMatchEnd) {
		if (tag == "BTN_EXIT")
			_returnCode = 1;
		return;
	}

	if (_engine.getState() != PazaakEngine::kStatePlayerTurn ||
	    _engine.getPlayer().standing || _engine.getPlayer().bust)
		return;

	if (tag == "BTN_STAND") {
		_engine.playerStand();
		updateUI();
	} else if (tag == "BTN_END_TURN") {
		_engine.playerDraw();
		updateUI();
	} else if (tag.beginsWith("BTN_HAND_")) {
		const int index = atoi(tag.c_str() + 9);
		_engine.playerPlayCard(index);
		updateUI();
	} else if (tag == "BTN_EXIT") {
		_returnCode = 1;
	}
}

void PazaakGUI::callbackRun() {
	// Turn logic is handled synchronously by PazaakEngine.
}

} // End of namespace KotOR
} // End of namespace Engines
