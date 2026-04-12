#include "src/engines/odyssey/button.h"
#include "src/engines/odyssey/label.h"
#include "src/engines/kotor/gui/ingame/pazaak.h"

namespace Engines {
namespace KotOR {

PazaakGUI::PazaakGUI(PazaakEngine &engine, Console *console) :
		KotORBase::GUI(console),
		_engine(engine) {

	load("pazaakpnl");
	addBackground(KotORBase::kBackgroundTypeMenu);

	updateUI();
}

PazaakGUI::~PazaakGUI() {}

void PazaakGUI::updateUI() {
	auto setLabel = [this](const char *tag, const Common::UString &text) {
		Odyssey::WidgetLabel *lbl = getLabel(tag);
		if (lbl) lbl->setText(text);
	};

	setLabel("LBL_PLAYER_SCORE", Common::composeString(_engine.getPlayer().score));
	setLabel("LBL_OPPONENT_SCORE", Common::composeString(_engine.getOpponent().score));
	
	setLabel("LBL_PLAYER_SETS", Common::composeString(_engine.getPlayer().setsWon));
	setLabel("LBL_OPPONENT_SETS", Common::composeString(_engine.getOpponent().setsWon));

	// Update hand buttons
	for (int i = 0; i < 4; ++i) {
		Common::UString btnTag = Common::String::format("BTN_HAND_%d", i);
		Odyssey::WidgetButton *btn = getButton(btnTag);
		if (btn) {
			if (i < (int)_engine.getPlayer().hand.size()) {
				int val = _engine.getPlayer().hand[i];
				btn->setText(Common::String::format("%s%d", val > 0 ? "+" : "", val));
				btn->show();
			} else {
				btn->hide();
			}
		}
	}

	if (_engine.getState() == PazaakEngine::kStateMatchEnd) {
		setLabel("LBL_STATUS", _engine.getWinner() == 1 ? "YOU WIN!" : "YOU LOSE!");
	} else if (_engine.getPlayer().standing) {
		setLabel("LBL_STATUS", "STANDING");
	} else {
		setLabel("LBL_STATUS", "YOUR TURN");
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
		int index = atoi(tag.c_str() + 9);
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
