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
 *  Unit tests for the fatal error reporting helpers.
 */

#include <stdexcept>

#include "gtest/gtest.h"

#include "src/common/error.h"
#include "src/common/fatalerror.h"

GTEST_TEST(FatalError, wrapText) {
	EXPECT_STREQ(Common::wrapText("", 10).c_str(), "");
	EXPECT_STREQ(Common::wrapText("short", 10).c_str(), "short");
	EXPECT_STREQ(Common::wrapText("one two three four", 9).c_str(), "one two\nthree\nfour");
	EXPECT_STREQ(Common::wrapText("one two  three", 80).c_str(), "one two three");

	// Existing line breaks are kept
	EXPECT_STREQ(Common::wrapText("one\ntwo three", 7).c_str(), "one\ntwo\nthree");
	EXPECT_STREQ(Common::wrapText("one\n\ntwo", 80).c_str(), "one\n\ntwo");

	// Long words aren't broken up
	EXPECT_STREQ(Common::wrapText("a /a/very/long/path b", 5).c_str(), "a\n/a/very/long/path\nb");
}

GTEST_TEST(FatalError, buildFatalErrorMessage) {
	EXPECT_STREQ(Common::buildFatalErrorMessage("Something broke", "").c_str(), "Something broke");

	EXPECT_STREQ(Common::buildFatalErrorMessage("Something broke", "/tmp/xoreos.log").c_str(),
	             "Something broke\n\nMore details can be found in the log file:\n/tmp/xoreos.log");

	EXPECT_FALSE(Common::buildFatalErrorMessage("", "").empty());

	// Long lines are wrapped
	const Common::UString message = Common::buildFatalErrorMessage(Common::UString('x', 50) + " " +
	                                                               Common::UString('y', 50), "");
	EXPECT_STREQ(message.c_str(), (Common::UString('x', 50) + "\n" + Common::UString('y', 50)).c_str());
}

GTEST_TEST(FatalError, hasGraphicalDisplay) {
	EXPECT_FALSE(Common::hasGraphicalDisplay(":0", "wayland-0", "dummy"));
	EXPECT_FALSE(Common::hasGraphicalDisplay(":0", "wayland-0", "offscreen"));

#if !defined(WIN32) && !defined(__APPLE__)
	EXPECT_TRUE(Common::hasGraphicalDisplay(":0", 0, 0));
	EXPECT_TRUE(Common::hasGraphicalDisplay(0, "wayland-0", 0));
	EXPECT_TRUE(Common::hasGraphicalDisplay(":0", 0, "x11"));

	EXPECT_FALSE(Common::hasGraphicalDisplay(0, 0, 0));
	EXPECT_FALSE(Common::hasGraphicalDisplay("", "", 0));
#else
	EXPECT_TRUE(Common::hasGraphicalDisplay(0, 0, 0));
#endif
}

GTEST_TEST(FatalError, describeCurrentException) {
	try {
		Common::Exception e("Inner reason");
		e.add("Outer reason");

		throw e;
	} catch (...) {
		EXPECT_STREQ(Common::describeCurrentException().c_str(), "Outer reason\nBecause: Inner reason");

		// Describing the exception must not change it
		EXPECT_STREQ(Common::describeCurrentException().c_str(), "Outer reason\nBecause: Inner reason");
	}

	try {
		throw std::runtime_error("Standard exception");
	} catch (...) {
		EXPECT_STREQ(Common::describeCurrentException().c_str(), "Standard exception");
	}

	try {
		throw 23;
	} catch (...) {
		EXPECT_STREQ(Common::describeCurrentException().c_str(), "");
	}
}

GTEST_TEST(FatalError, showFatalErrorDialogDisabled) {
	// Dialogs are disabled by default, so this must return right away
	Common::showFatalErrorDialog("Test error", "");
}
