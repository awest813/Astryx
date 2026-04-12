#ifndef ENGINES_KOTOR_PAZAAK_H
#define ENGINES_KOTOR_PAZAAK_H

#include <vector>
#include "src/common/types.h"

namespace Engines {
namespace KotOR {

class PazaakPlayer {
public:
	int score { 0 };
	int setsWon { 0 };
	bool standing { false };
	bool bust { false };
	std::vector<int> sideDeck;
	std::vector<int> hand;

	void resetRound() {
		score = 0;
		standing = false;
		bust = false;
	}
};

class PazaakEngine {
public:
	enum State {
		kStatePlayerTurn,
		kStateOpponentTurn,
		kStateRoundEnd,
		kStateMatchEnd
	};

	PazaakEngine();

	void startMatch(const std::vector<int> &playerSideDeck, const std::vector<int> &opponentSideDeck);
	void playerDraw();
	void playerStand();
	void playerPlayCard(int handIndex);
	void opponentAI();

	State getState() const { return _state; }
	int getWinner() const { return _winner; } // 1 = Player, 2 = Opponent

	PazaakPlayer &getPlayer() { return _player; }
	PazaakPlayer &getOpponent() { return _opponent; }

private:
	void checkRoundEnd();
	void nextTurn();
	int drawMainCard();

	PazaakPlayer _player;
	PazaakPlayer _opponent;
	State _state;
	int _winner { 0 };
};

} // End of namespace KotOR
} // End of namespace Engines

#endif
