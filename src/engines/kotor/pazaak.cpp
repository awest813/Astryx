#include <algorithm>
#include "src/common/random.h"
#include "src/engines/kotor/pazaak.h"

namespace Engines {
namespace KotOR {

PazaakEngine::PazaakEngine() : _state(kStatePlayerTurn) {}

void PazaakEngine::startMatch(const std::vector<int> &playerSideDeck, const std::vector<int> &opponentSideDeck) {
	_player.sideDeck = playerSideDeck;
	_opponent.sideDeck = opponentSideDeck;
	
	// Draw 4 random cards from side deck for the hand
	std::random_shuffle(_player.sideDeck.begin(), _player.sideDeck.end());
	_player.hand.assign(_player.sideDeck.begin(), _player.sideDeck.begin() + 4);

	std::random_shuffle(_opponent.sideDeck.begin(), _opponent.sideDeck.end());
	_opponent.hand.assign(_opponent.sideDeck.begin(), _opponent.sideDeck.begin() + 4);

	_player.setsWon = 0;
	_opponent.setsWon = 0;
	_player.resetRound();
	_opponent.resetRound();
	
	_state = kStatePlayerTurn;
	playerDraw();
}

int PazaakEngine::drawMainCard() {
	return (rand() % 10) + 1;
}

void PazaakEngine::playerDraw() {
	if (_player.standing) {
		nextTurn();
		return;
	}
	_player.score += drawMainCard();
	if (_player.score > 20) {
		_player.bust = true;
		checkRoundEnd();
	}
}

void PazaakEngine::playerStand() {
	_player.standing = true;
	nextTurn();
}

void PazaakEngine::playerPlayCard(int handIndex) {
	if (handIndex < 0 || handIndex >= (int)_player.hand.size()) return;
	_player.score += _player.hand[handIndex];
	_player.hand.erase(_player.hand.begin() + handIndex);
	
	if (_player.score > 20) _player.bust = true;
	else _player.bust = false; // Could have played a negative card to save themselves
}

void PazaakEngine::opponentAI() {
	if (_opponent.standing) {
		nextTurn();
		return;
	}

	_opponent.score += drawMainCard();

	// Basic AI: Use negative cards if bust, stand if >= 18
	if (_opponent.score > 20) {
		for (size_t i = 0; i < _opponent.hand.size(); ++i) {
			if (_opponent.hand[i] < 0 && _opponent.score + _opponent.hand[i] <= 20) {
				_opponent.score += _opponent.hand[i];
				_opponent.hand.erase(_opponent.hand.begin() + i);
				break;
			}
		}
	}

	if (_opponent.score > 20) {
		_opponent.bust = true;
	} else if (_opponent.score >= 18 || (_opponent.score > _player.score && _player.standing)) {
		_opponent.standing = true;
	}

	checkRoundEnd();
}

void PazaakEngine::nextTurn() {
	if (_state == kStatePlayerTurn) {
		if (!_opponent.standing) {
			_state = kStateOpponentTurn;
			opponentAI();
		} else if (!_player.standing) {
			playerDraw();
		} else {
			checkRoundEnd();
		}
	} else {
		if (!_player.standing) {
			_state = kStatePlayerTurn;
			playerDraw();
		} else if (!_opponent.standing) {
			opponentAI();
		} else {
			checkRoundEnd();
		}
	}
}

void PazaakEngine::checkRoundEnd() {
	bool roundDone = false;
	int roundWinner = 0;

	if (_player.bust) {
		roundWinner = 2;
		roundDone = true;
	} else if (_opponent.bust) {
		roundWinner = 1;
		roundDone = true;
	} else if (_player.standing && _opponent.standing) {
		if (_player.score > _opponent.score) roundWinner = 1;
		else if (_opponent.score > _player.score) roundWinner = 2;
		else roundWinner = 0; // Draw
		roundDone = true;
	}

	if (roundDone) {
		if (roundWinner == 1) _player.setsWon++;
		else if (roundWinner == 2) _opponent.setsWon++;

		if (_player.setsWon >= 3) {
			_winner = 1;
			_state = kStateMatchEnd;
		} else if (_opponent.setsWon >= 3) {
			_winner = 2;
			_state = kStateMatchEnd;
		} else {
			_player.resetRound();
			_opponent.resetRound();
			_state = kStatePlayerTurn;
			playerDraw();
		}
	} else {
		nextTurn();
	}
}

} // End of namespace KotOR
} // End of namespace Engines
