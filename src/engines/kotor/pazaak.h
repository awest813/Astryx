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
 *  Pazaak card game logic for Star Wars: Knights of the Old Republic.
 */

#ifndef ENGINES_KOTOR_PAZAAK_H
#define ENGINES_KOTOR_PAZAAK_H

#include <vector>

#include "src/common/types.h"

namespace Engines {
namespace KotOR {

/** Special side-deck card markers (values 1-6 and -1..-6 are normal). */
static const int kPazaakCardFlip   = 100;
static const int kPazaakCardDouble = 200;

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

	/** Build a side deck preset for chargen / script side index. */
	static std::vector<int> sideDeckForIndex(int sideIndex);

private:
	void checkRoundEnd();
	void nextTurn();
	int drawMainCard();
	void applySideCard(PazaakPlayer &who, int card);

	PazaakPlayer _player;
	PazaakPlayer _opponent;
	State _state;
	int _winner { 0 };
	int _lastMainDraw { 0 };
	bool _playerDoubleNext { false };
	bool _opponentDoubleNext { false };
};

} // End of namespace KotOR
} // End of namespace Engines

#endif // ENGINES_KOTOR_PAZAAK_H
