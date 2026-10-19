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
 *
 *  A player who starts xoreos from a desktop launcher never sees stderr.
 *  Without a dialog, a fatal startup error just looks like the game
 *  silently failing to open its window.
 */

#ifndef COMMON_FATALERROR_H
#define COMMON_FATALERROR_H

#include "src/common/ustring.h"

namespace Common {

/** Allow (or forbid) showing fatal errors in a native dialog window.
 *
 *  This is off by default, so that unit tests and other users of the
 *  common code never pop up a dialog. The main program switches it on.
 */
void enableFatalErrorDialog(bool enable);

/** The width, in characters, that fatal error dialog lines are wrapped to. */
static const size_t kFatalErrorLineWidth = 80;

/** Word-wrap text to lines of at most width characters.
 *
 *  Existing line breaks are kept, and runs of spaces collapse into one.
 *  Words longer than width (like long paths) are never broken up.
 */
UString wrapText(const UString &text, size_t width);

/** Build the text of a fatal error dialog. */
UString buildFatalErrorMessage(const UString &error, const UString &logFile);

/** Judging by the given environment variable values, can we show a dialog window?
 *
 *  Any of the parameters may be 0 for an unset variable.
 *
 *  @param display        The value of DISPLAY (X11).
 *  @param waylandDisplay The value of WAYLAND_DISPLAY.
 *  @param videoDriver    The value of SDL_VIDEODRIVER.
 */
bool hasGraphicalDisplay(const char *display, const char *waylandDisplay, const char *videoDriver);

/** Show a fatal error to the player in a native dialog window.
 *
 *  Nothing happens when dialogs haven't been enabled, when there's no
 *  display to show a dialog on (for example when running headless), or
 *  when a fatal error dialog has already been shown. This blocks until
 *  the dialog is closed and never throws.
 *
 *  @param error   The error message to show.
 *  @param logFile The log file to point the player to, or "" for none.
 */
void showFatalErrorDialog(const UString &error, const UString &logFile);

/** Return a text describing the exception that's currently being handled.
 *
 *  This must only be called from within a catch block. For a
 *  Common::Exception, every reason on the exception's stack is included,
 *  from the outermost one to the innermost one.
 */
UString describeCurrentException();

} // End of namespace Common

#endif // COMMON_FATALERROR_H
