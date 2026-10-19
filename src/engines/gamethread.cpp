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
 *  The thread the game logic runs in.
 */

#include <cassert>

#include "src/version/version.h"

#include "src/common/util.h"
#include "src/common/error.h"
#include "src/common/fatalerror.h"
#include "src/common/ustring.h"
#include "src/common/configman.h"

#include "src/engines/gamethread.h"
#include "src/engines/enginemanager.h"

#include "src/graphics/windowman.h"

#include "src/events/events.h"

namespace Engines {

GameThread::GameThread() {
}

GameThread::~GameThread() {
	destroyThread();
}

void GameThread::init(const Common::UString &baseDir, const std::list<const EngineProbe *> &probes) {
	// Detecting the game

	_game.reset(EngineMan.probeGame(baseDir, probes));
	if (!_game)
		throw Common::Exception("No game that xoreos supports was found in \"%s\". The game path must be the "
		                        "game's main installation folder: for KotOR, the folder with chitin.key and "
		                        "swkotor.exe in it, not a subfolder like \"data\" or \"modules\"",
		                        baseDir.c_str());

	// Get the game description from the config, or alternatively
	// construct one from the game name and platform.
	Common::UString description;
	if (!ConfigMan.getKey("description", description))
		description = _game->getGameName(true);

	WindowMan.setWindowTitle(Common::UString(Version::getProjectName()) + " -- " + description);

	status("Detected game \"%s\"", _game->getGameName(false).c_str());
}

void GameThread::run() {
	if (ConfigMan.getBool("listlangs", false)) {
		assert(_game);

		EngineMan.listLanguages(*_game);

		EventMan.requestQuit();
		EventMan.doQuit();
		return;
	}

	if (!createThread("GameInstance"))
		throw Common::Exception("Failed creating game logic thread");
}

void GameThread::threadMethod() {
	assert(_game);

	try {
		EngineMan.run(*_game);
	} catch (...) {
		// Let the main thread report the error to the player and exit with a failure code.
		// NOTE: Printing the exception empties it, so describe it first.
		{
			std::lock_guard<std::mutex> lock(_fatalErrorMutex);
			_fatalError = Common::describeCurrentException();
		}

		Common::exceptionDispatcherError();

		EventMan.raiseFatalError();
	}

	EventMan.doQuit();
}

Common::UString GameThread::getFatalError() const {
	std::lock_guard<std::mutex> lock(_fatalErrorMutex);

	return _fatalError;
}

} // End of namespace Engines
