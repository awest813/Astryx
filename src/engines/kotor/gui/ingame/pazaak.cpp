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
	setWidgetText("LBL_PLAYER_SCORE", Common::composeString(_engine.getPlayer().score));
	setWidgetText("LBL_OPPONENT_SCORE", Common::composeString(_engine.getOpponent().score));
	setWidgetText("LBL_PLAYER_SETS", Common::composeString(_engine.getPlayer().setsWon));
	setWidgetText("LBL_OPPONENT_SETS", Common::composeString(_engine.getOpponent().setsWon));

	for (int i = 0; i < 4; ++i) {
		const Common::UString btnTag = Common::UString("BTN_HAND_") + Common::composeString(i);
		Odyssey::WidgetButton *btn = getButton(btnTag);
		if (!btn)
			continue;

		if (i < (int)_engine.getPlayer().hand.size()) {
			btn->setInvisible(false);
			setWidgetText(btnTag, formatCardValue(_engine.getPlayer().hand[i]));
			btn->show();
		} else {
			btn->hide();
		}
	}

	if (_engine.getState() == PazaakEngine::kStateMatchEnd) {
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
	if (_engine.getState() == PazaakEngine::kStateOpponentTurn) {
		_engine.opponentAI();
		updateUI();
	}
}

} // End of namespace KotOR
} // End of namespace Engines
