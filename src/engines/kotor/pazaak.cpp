#include <algorithm>
#include <random>

#include "src/common/random.h"
#include "src/common/strutil.h"
#include "src/engines/kotor/pazaak.h"

namespace Engines {
namespace KotOR {

PazaakEngine::PazaakEngine() : _state(kStatePlayerTurn) {}

std::vector<int> PazaakEngine::sideDeckForIndex(int sideIndex) {
	switch (sideIndex) {
	case 1:
		return {2, 3, 4, 5, 6, -1, kPazaakCardFlip};
	case 2:
		return {1, -1, 2, -2, 3, -3, kPazaakCardDouble};
	default:
		return {1, -1, 2, -2, 3, -3, 4, -4};
	}
}

void PazaakEngine::startMatch(const std::vector<int> &playerSideDeck, const std::vector<int> &opponentSideDeck) {
	_player.sideDeck = playerSideDeck;
	_opponent.sideDeck = opponentSideDeck;

	std::mt19937 rng(std::random_device{}());
	std::shuffle(_player.sideDeck.begin(), _player.sideDeck.end(), rng);
	_player.hand.assign(_player.sideDeck.begin(),
	                    _player.sideDeck.begin() + std::min<size_t>(4, _player.sideDeck.size()));

	std::shuffle(_opponent.sideDeck.begin(), _opponent.sideDeck.end(), rng);
	_opponent.hand.assign(_opponent.sideDeck.begin(),
	                     _opponent.sideDeck.begin() + std::min<size_t>(4, _opponent.sideDeck.size()));

	_player.setsWon = 0;
	_opponent.setsWon = 0;
	_player.resetRound();
	_opponent.resetRound();
	_lastMainDraw = 0;
	_playerDoubleNext = false;
	_opponentDoubleNext = false;

	_state = kStatePlayerTurn;
	playerDraw();
}

int PazaakEngine::drawMainCard() {
	_lastMainDraw = RNG.getNext(1, 11);
	return _lastMainDraw;
}

void PazaakEngine::applySideCard(PazaakPlayer &who, int card) {
	if (card == kPazaakCardFlip) {
		who.score -= _lastMainDraw * 2;
		return;
	}

	if (card == kPazaakCardDouble) {
		if (&who == &_player)
			_playerDoubleNext = true;
		else
			_opponentDoubleNext = true;
		return;
	}

	who.score += card;
}

void PazaakEngine::playerDraw() {
	if (_player.standing) {
		nextTurn();
		return;
	}

	int draw = drawMainCard();
	if (_playerDoubleNext) {
		draw *= 2;
		_playerDoubleNext = false;
	}
	_player.score += draw;

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
	if (_state != kStatePlayerTurn || _player.standing)
		return;
	if (handIndex < 0 || handIndex >= (int)_player.hand.size())
		return;

	const int card = _player.hand[handIndex];
	_player.hand.erase(_player.hand.begin() + handIndex);
	applySideCard(_player, card);

	if (_player.score > 20)
		_player.bust = true;
	else
		_player.bust = false;

	checkRoundEnd();
}

void PazaakEngine::opponentAI() {
	if (_opponent.standing) {
		nextTurn();
		return;
	}

	int draw = drawMainCard();
	if (_opponentDoubleNext) {
		draw *= 2;
		_opponentDoubleNext = false;
	}
	_opponent.score += draw;

	if (_opponent.score > 20) {
		for (size_t i = 0; i < _opponent.hand.size(); ++i) {
			const int card = _opponent.hand[i];
			if (card < 0 && _opponent.score + card <= 20) {
				_opponent.hand.erase(_opponent.hand.begin() + i);
				applySideCard(_opponent, card);
				break;
			}
			if (card == kPazaakCardFlip && _opponent.score - (_lastMainDraw * 2) <= 20) {
				_opponent.hand.erase(_opponent.hand.begin() + i);
				applySideCard(_opponent, card);
				break;
			}
		}
	}

	if (_opponent.score > 20) {
		_opponent.bust = true;
	} else if (_opponent.score >= 18 || (_player.standing && _opponent.score > _player.score)) {
		_opponent.standing = true;
	}

	checkRoundEnd();
}

void PazaakEngine::nextTurn() {
	if (_state == kStateMatchEnd)
		return;

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
		} else if (!_opponent.standing) {
			opponentAI();
		} else {
			checkRoundEnd();
		}
	}
}

void PazaakEngine::checkRoundEnd() {
	if (_state == kStateMatchEnd)
		return;

	bool roundDone = false;
	int roundWinner = 0;

	if (_player.bust) {
		roundWinner = 2;
		roundDone = true;
	} else if (_opponent.bust) {
		roundWinner = 1;
		roundDone = true;
	} else if (_player.standing && _opponent.standing) {
		if (_player.score > _opponent.score)
			roundWinner = 1;
		else if (_opponent.score > _player.score)
			roundWinner = 2;
		roundDone = true;
	}

	if (!roundDone)
		return;

	if (roundWinner == 1)
		_player.setsWon++;
	else if (roundWinner == 2)
		_opponent.setsWon++;

	if (_player.setsWon >= 3) {
		_winner = 1;
		_state = kStateMatchEnd;
	} else if (_opponent.setsWon >= 3) {
		_winner = 2;
		_state = kStateMatchEnd;
	} else {
		_player.resetRound();
		_opponent.resetRound();
		_playerDoubleNext = false;
		_opponentDoubleNext = false;
		_state = kStatePlayerTurn;
		playerDraw();
	}
}

} // End of namespace KotOR
} // End of namespace Engines
