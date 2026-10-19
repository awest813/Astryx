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
 *  Reporting fatal errors to the player in a native dialog window.
 */

#include <cstdlib>
#include <cstring>

#include <atomic>
#include <exception>

#include <SDL_messagebox.h>

#include "src/version/version.h"

#include "src/common/fatalerror.h"
#include "src/common/error.h"
#include "src/common/threads.h"

namespace Common {

static std::atomic<bool> fatalErrorDialogEnabled(false);
static std::atomic<bool> fatalErrorDialogShown(false);

void enableFatalErrorDialog(bool enable) {
	fatalErrorDialogEnabled = enable;
}

UString wrapText(const UString &text, size_t width) {
	UString wrapped, line, word;

	// Add the current word to the current line, breaking the line first if necessary
	auto addWord = [&]() {
		if (word.empty())
			return;

		if (!line.empty() && ((line.size() + 1 + word.size()) > width)) {
			wrapped += line + "\n";
			line.clear();
		}

		if (!line.empty())
			line += ' ';

		line += word;
		word.clear();
	};

	for (UString::iterator c = text.begin(); c != text.end(); ++c) {
		if (*c == '\n') {
			addWord();

			wrapped += line + "\n";
			line.clear();
		} else if (*c == ' ') {
			addWord();
		} else
			word += *c;
	}

	addWord();
	wrapped += line;

	return wrapped;
}

UString buildFatalErrorMessage(const UString &error, const UString &logFile) {
	UString message = error.empty() ? UString("An unknown error occurred.") : error;

	if (!logFile.empty())
		message += "\n\nMore details can be found in the log file:\n" + logFile;

	// Not all dialog implementations wrap long lines by themselves
	return wrapText(message, kFatalErrorLineWidth);
}

bool hasGraphicalDisplay(const char *display, const char *waylandDisplay, const char *videoDriver) {
	// These SDL video drivers never show anything on a screen
	if (videoDriver && (!std::strcmp(videoDriver, "dummy") || !std::strcmp(videoDriver, "offscreen")))
		return false;

#if defined(WIN32) || defined(__APPLE__)
	(void) display;
	(void) waylandDisplay;

	return true;
#else
	// On other systems, we need either an X11 or a Wayland display
	return (display && (display[0] != '\0')) || (waylandDisplay && (waylandDisplay[0] != '\0'));
#endif
}

void showFatalErrorDialog(const UString &error, const UString &logFile) {
	if (!fatalErrorDialogEnabled)
		return;

	if (!hasGraphicalDisplay(std::getenv("DISPLAY"), std::getenv("WAYLAND_DISPLAY"), std::getenv("SDL_VIDEODRIVER")))
		return;

#ifdef __APPLE__
	/* On macOS, the dialog is shown by the main thread. Waiting for it from
	 * another thread could deadlock, so don't risk that. */
	if (initedThreads() && !isMainThread())
		return;
#endif

	// Only ever show one fatal error dialog
	if (fatalErrorDialogShown.exchange(true))
		return;

	try {
		const UString title = UString(Version::getProjectName()) + " - Fatal error";
		const UString message = buildFatalErrorMessage(error, logFile);

		// This is safe to call before SDL_Init() and after SDL_Quit()
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title.c_str(), message.c_str(), 0);
	} catch (...) {
	}
}

UString describeCurrentException() {
	try {
		try {
			throw;
		} catch (Exception &e) {
			// Work on a copy, to leave the original exception intact
			Exception copy(e);
			Exception::Stack &stack = copy.getStack();

			UString text;
			while (!stack.empty()) {
				if (!text.empty())
					text += "\nBecause: ";

				text += stack.top();
				stack.pop();
			}

			return text;
		} catch (std::exception &e) {
			return e.what();
		} catch (...) {
		}
	} catch (...) {
	}

	return "";
}

} // End of namespace Common
