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
 *  The project's main entry point.
 */

#define SDL_MAIN_HANDLED

#include <cassert>
#include <climits>
#include <cstdio>

#include <vector>

#include "src/cline.h"
#include "src/engines.h"

#include "src/common/ustring.h"
#include "src/common/util.h"
#include "src/common/error.h"
#include "src/common/fatalerror.h"
#include "src/common/string.h"
#include "src/common/platform.h"
#include "src/common/filepath.h"
#include "src/common/threads.h"
#include "src/common/debugman.h"
#include "src/common/configman.h"
#include "src/common/configfile.h"
#include "src/common/random.h"
#ifdef ENABLE_XML
#include "src/common/xml.h"
#endif

#include "src/aurora/resman.h"
#include "src/aurora/2dareg.h"
#include "src/aurora/language.h"
#include "src/aurora/talkman.h"
#include "src/aurora/util.h"

#include "src/aurora/nwscript/objectman.h"
#include "src/aurora/nwscript/functionman.h"

#include "src/graphics/queueman.h"
#include "src/graphics/graphics.h"
#include "src/graphics/camera.h"
#include "src/graphics/windowman.h"
#include "src/graphics/yuv_to_rgb.h"

#include "src/graphics/aurora/textureman.h"
#include "src/graphics/aurora/cursorman.h"
#include "src/graphics/aurora/fontman.h"

#include "src/graphics/mesh/meshman.h"

#include "src/graphics/shader/surfaceman.h"
#include "src/graphics/shader/materialman.h"
#include "src/graphics/shader/shader.h"

#include "src/sound/sound.h"

#include "src/events/requests.h"
#include "src/events/events.h"
#include "src/events/timerman.h"
#include "src/events/notifications.h"

#include "src/engines/enginemanager.h"
#include "src/engines/gamethread.h"

#include "src/engines/aurora/tokenman.h"
#include "src/engines/aurora/flycamera.h"

static void initPlatform();
static void initConfig();

static void init();
static void deinit();

static void listDebug();

static Common::UString describeUnknownTarget(const Common::UString &target);

static bool configFileIsBroken = false;

int main(int argc, char **argv) {
	// Fatal errors are also shown in a dialog window, unless the "errordialog" option is false
	Common::enableFatalErrorDialog(true);

	initPlatform();
	initConfig();

	Common::enableFatalErrorDialog(ConfigMan.getBool("errordialog", true));

	std::vector<Common::UString> args;
	Common::UString target;

	try {
		Common::Platform::getParameters(argc, argv, args);

		int code;
		if (!parseCommandline(args, target, code))
			return code;

	} catch (...) {
		// NOTE: Printing the exception empties it, so describe it first
		const Common::UString reason = Common::describeCurrentException();

		Common::exceptionDispatcherError();
		Common::showFatalErrorDialog(reason, "");

		return 1;
	}

	// Check the requested target
	if (target.empty() || !ConfigMan.hasGame(target)) {
		Common::UString path = ConfigMan.getString("path", "");
		if (path.empty()) {
			if (ConfigMan.getBool("listdebug", false)) {
				listDebug();
				return 0;
			}

			error("%s", describeUnknownTarget(target).c_str());
		}

		if (!target.empty() && !Common::ConfigFile::isValidName(target))
			error("\"%s\" can't be used as a target name. Target names may only contain letters, "
			      "numbers, spaces, '-', '_' and '.'", target.c_str());

		bool useExisting = false;
		if (target.empty()) {
			target = ConfigMan.findGame(path);
			if (!target.empty()) {
				warning("No target specified, but found a target with a matching path");
				useExisting = true;
			}
		}

		target = ConfigMan.createGame(path, target);
		if (target.empty())
			error("Failed creating a new config target for the game");

		if (!useExisting)
			warning("Creating a new target for this game");

		if (!Common::ConfigFile::isValidValue(path))
			warning("The game path \"%s\" contains a '#' or leading/trailing spaces, which the config "
			        "file can't store. Target \"%s\" will only be used for this run and not be saved",
			        path.c_str(), target.c_str());
	}

	if (!ConfigMan.setGame(target) || !ConfigMan.isInGame())
		error("%s", describeUnknownTarget(target).c_str());

	/* Open the log file.
	 *
	 * NOTE: A log is opened by default, unless the logfile config value
	 *       is set to an empty string or nologfile is set to true.
	 */
	Common::UString logFile = DebugMan.getDefaultLogFile();
	if (ConfigMan.hasKey("logfile"))
		logFile = ConfigMan.getString("logfile", "");
	if (ConfigMan.getBool("nologfile", false))
		logFile.clear();

	if (!logFile.empty())
		if (!DebugMan.openLogFile(logFile))
			warning("Failed to open log file \"%s\" for writing", logFile.c_str());

	DebugMan.logCommandLine(args);

	status("Target \"%s\"", target.c_str());

	Common::UString dirArg = ConfigMan.getString("path", "");
	if (dirArg.empty())
		error("Target \"%s\" in the config file \"%s\" has no game path. Add a \"path=\" line with "
		      "the game's installation folder to it, or start xoreos with -p <game folder>",
		      target.c_str(), ConfigMan.getConfigFile().c_str());

	Common::UString baseDir;
	try {
		baseDir = Common::FilePath::canonicalize(dirArg);
	} catch (...) {
		error("The game path \"%s\" is not a valid path. Start xoreos with -p <game folder>, "
		      "or fix the \"path\" of target \"%s\" in the config file \"%s\"",
		      dirArg.c_str(), target.c_str(), ConfigMan.getConfigFile().c_str());
	}

	if (!Common::FilePath::isDirectory(baseDir) && !Common::FilePath::isRegularFile(baseDir))
		error("The game folder \"%s\" doesn't exist. Start xoreos with -p <game folder>, pointing it "
		      "to the game's installation folder, or fix the \"path\" of target \"%s\" in the config file \"%s\"",
		      baseDir.c_str(), target.c_str(), ConfigMan.getConfigFile().c_str());

	std::list<const Engines::EngineProbe *> probes;

	int exitCode = 0;
	Common::UString fatalError;

	Engines::GameThread *gameThread = new Engines::GameThread;
	try {
		// Enable requested debug channels
		DebugMan.setVerbosityLevelsFromConfig();

		// Create all our engine probes
		createEngineProbes(probes);

		/* Probe and create the game engine. We do this before opening a window,
		 * so that pointing xoreos at the wrong folder fails quickly and clearly. */
		gameThread->init(baseDir, probes);

		// Initialize all necessary subsystems
		init();

		if (ConfigMan.getBool("listdebug", false)) {
			// List debug channels
			listDebug();
		} else {
			// Run the game
			gameThread->run();
			EventMan.runMainLoop();
		}

	} catch (...) {
		// NOTE: Printing the exception empties it, so describe it first
		fatalError = Common::describeCurrentException();
		exitCode   = 1;

		Common::exceptionDispatcherError();
	}

	if (EventMan.fatalErrorRaised()) {
		// The game thread stopped because of an error
		Common::showFatalErrorDialog(gameThread->getFatalError(), DebugMan.getLogFile());
		std::exit(1);
	}

	status("Shutting down");

	try {
		delete gameThread;
	} catch (...) {
	}

	destroyEngineProbes(probes);

	try {
		// Sync changed debug channel settings
		DebugMan.setConfigToVerbosityLevels();

		// Configs changed, we should save them
		if (ConfigMan.changed()) {
			// But don't clobber a broken save
			if (!configFileIsBroken)
				ConfigMan.save();
		}
	} catch (...) {
		Common::exceptionDispatcherError();
	}

	const Common::UString openedLogFile = DebugMan.getLogFile();

	deinit();

	// Show the error only now, after the game window is gone and can't hide the dialog
	if (exitCode != 0)
		Common::showFatalErrorDialog(fatalError, openedLogFile);

	return exitCode;
}

static void initPlatform() {
	try {
		Common::Platform::init();
	} catch (...) {
		// NOTE: Printing the exception empties it, so describe it first
		const Common::UString reason = Common::describeCurrentException();

		Common::exceptionDispatcherError("Failed to initialize the low-level platform-specific subsystem");
		Common::showFatalErrorDialog("Failed to initialize the low-level platform-specific subsystem\nBecause: " +
		                             reason, "");
		std::exit(1);
	}
}

static void initConfig() {
	bool newConfig = false;
	if (!ConfigMan.load()) {
		// Loading failed, create an empty config file

		ConfigMan.create();

		// Mark the config as either broken or new
		if (ConfigMan.fileExists())
			configFileIsBroken = true;
		else
			newConfig = true;
	}

	if (configFileIsBroken)
		warning("The config file \"%s\" can't be read (see above) and will be ignored. xoreos won't save "
		        "any settings to it until it's fixed. Fix the line mentioned above, or delete the file "
		        "to let xoreos create a new one", ConfigMan.getConfigFile().c_str());

	ConfigMan.setInt   (Common::kConfigRealmDefault, "width" ,     800);
	ConfigMan.setInt   (Common::kConfigRealmDefault, "height",     600);
	ConfigMan.setBool  (Common::kConfigRealmDefault, "fullscreen", false);
	ConfigMan.setInt   (Common::kConfigRealmDefault, "fsaa",       0);

	ConfigMan.setDouble(Common::kConfigRealmDefault, "volume"      , 1.0);
	ConfigMan.setDouble(Common::kConfigRealmDefault, "volume_music", 1.0);
	ConfigMan.setDouble(Common::kConfigRealmDefault, "volume_sfx"  , 1.0);
	ConfigMan.setDouble(Common::kConfigRealmDefault, "volume_voice", 1.0);
	ConfigMan.setDouble(Common::kConfigRealmDefault, "volume_video", 1.0);

	ConfigMan.setBool(Common::kConfigRealmDefault, "showfps", false);

	ConfigMan.setBool(Common::kConfigRealmDefault, "skipvideos", false);

	ConfigMan.setBool(Common::kConfigRealmDefault, "saveconf", true);

	// Populate the new config with the defaults
	if (newConfig) {
		ConfigMan.setDefaults();
		ConfigMan.save();
	}
}

static void listDebug() {
	std::vector<Common::UString> names, descriptions;
	DebugMan.getDebugChannels(names, descriptions);

	assert(names.size() == descriptions.size());

	size_t maxNameLength = 0;
	for (std::vector<Common::UString>::const_iterator n = names.begin(); n != names.end(); n++)
		maxNameLength = MAX(maxNameLength, n->size());

	assert(maxNameLength < INT_MAX);

	for (size_t i = 0; i < names.size(); i++)
		std::printf("%-*s - %s\n", (int) maxNameLength, names[i].c_str(), descriptions[i].c_str());
}

static Common::UString describeUnknownTarget(const Common::UString &target) {
	const Common::UString configFile = ConfigMan.getConfigFile();

	Common::UString message;
	if (target.empty())
		message = "No game specified";
	else
		message = Common::String::format("There is no target \"%s\" in the config file \"%s\"",
		                                 target.c_str(), configFile.c_str());

	if (configFileIsBroken) {
		message += ". The config file can't be read, so no targets are known. Fix it (the warnings printed "
		           "at startup say which line is wrong) or delete it";
	} else {
		static const size_t kMaxListedTargets = 10;

		const std::vector<Common::UString> targets = ConfigMan.getGames();
		if (targets.empty()) {
			message += ". The config file doesn't have any targets yet";
		} else {
			message += ". Known targets: ";
			for (size_t i = 0; (i < targets.size()) && (i < kMaxListedTargets); i++)
				message += ((i > 0) ? ", " : "") + targets[i];

			if (targets.size() > kMaxListedTargets)
				message += ", ...";
		}
	}

	message += ". To start a game from its installation folder, use -p <game folder>. "
	           "Run with --help to see all options";

	return message;
}

static void init() {
	// Init threading system
	Common::initThreads();

#ifdef ENABLE_XML
	// Init libxml2
	Common::initXML();
#endif

	// Init subsystems
	try {
		GfxMan.init();
	} catch (Common::Exception &e) {
		e.add("Could not open the game window with OpenGL graphics. This is usually a graphics driver problem: "
		      "make sure your graphics driver is installed and up to date, and that your graphics card supports "
		      "OpenGL 2.1 or newer");
		throw;
	}
	status("Graphics subsystem initialized");
	SoundMan.init();
	status("Sound subsystem initialized");
	EventMan.init();
	status("Event subsystem initialized");
}

static void deinit() {
	// Deinit subsystems
	try {
		if (Common::initedThreads()) {
			EventMan.deinit();
			SoundMan.deinit();
			GfxMan.deinit();
		}
	} catch (...) {
	}

	// Destroy global singletons
	Graphics::Aurora::FontManager::destroy();
	Graphics::Aurora::CursorManager::destroy();
	Graphics::Aurora::TextureManager::destroy();

	Aurora::LanguageManager::destroy();
	Aurora::TalkManager::destroy();
	Aurora::TwoDARegistry::destroy();
	Aurora::ResourceManager::destroy();
	Aurora::FileTypeManager::destroy();

	Aurora::NWScript::ObjectManager::destroy();
	Aurora::NWScript::FunctionManager::destroy();

	Engines::EngineManager::destroy();
	Engines::TokenManager::destroy();
	Engines::FlyCamera::destroy();

	Events::EventsManager::destroy();
	Events::RequestManager::destroy();
	Events::TimerManager::destroy();

	Sound::SoundManager::destroy();

	Graphics::GraphicsManager::destroy();
	Graphics::QueueManager::destroy();
	Graphics::CameraManager::destroy();
	Graphics::WindowManager::destroy();
	Graphics::YUVToRGBManager::destroy();

	Graphics::Mesh::MeshManager::destroy();

	Graphics::Shader::SurfaceManager::destroy();
	Graphics::Shader::MaterialManager::destroy();
	Graphics::Shader::ShaderManager::destroy();

	Events::NotificationManager::destroy();

	Common::DebugManager::destroy();
	Common::ConfigManager::destroy();
	Common::Random::destroy();

#ifdef ENABLE_XML
	// Deinit libxml2
	Common::deinitXML();
#endif
}
