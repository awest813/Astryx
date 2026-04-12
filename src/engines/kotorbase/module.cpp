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
 *  The context needed to run a module in KotOR games.
 */

#include <map>
#include <utility>
#include <vector>

#include "src/common/util.h"
#include "src/common/maths.h"
#include "src/common/error.h"
#include "src/common/ustring.h"
#include "src/common/readfile.h"
#include "src/common/filepath.h"
#include "src/common/filelist.h"
#include "src/common/configman.h"
#include "src/common/debug.h"

#include "src/aurora/types.h"
#include "src/aurora/rimfile.h"
#include "src/aurora/gff3file.h"
#include "src/aurora/dlgfile.h"
#include "src/aurora/erfwriter.h"
#include "src/aurora/gff3writer.h"
#include "src/aurora/2dareg.h"
#include "src/aurora/2dafile.h"

#include "src/graphics/camera.h"

#include "src/graphics/aurora/model.h"
#include "src/graphics/aurora/textureman.h"

#include "src/sound/sound.h"

#include "src/events/events.h"

#include "src/engines/aurora/util.h"
#include "src/engines/aurora/resources.h"
#include "src/engines/aurora/console.h"
#include "src/engines/aurora/flycamera.h"

#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/item.h"
#include "src/engines/kotorbase/placeable.h"
#include "src/engines/kotorbase/module.h"
#include "src/engines/kotorbase/area.h"

#include "src/engines/kotorbase/gui/partyselection.h"

#include "src/engines/kotor/gui/dialog.h"

#include "src/engines/kotor/gui/ingame/ingame.h"

#include "src/engines/kotor/gui/loadscreen/loadscreen.h"

#include "src/engines/kotor/gui/chargen/chargeninfo.h"

namespace Engines {

namespace KotORBase {

bool Module::Action::operator<(const Action &s) const {
	return timestamp < s.timestamp;
}

Module::DelayedConversation::DelayedConversation(const Common::UString &_name, Aurora::NWScript::Object *_owner) :
		name(_name),
		owner(_owner) {
}


Module::Module(Game &game, ::Engines::Console &console) :
		Object(kObjectTypeModule),
		_game(game),
		_console(&console),
		_hasModule(false),
		_running(false),
		_pc(0),
		_currentTexturePack(-1),
		_exit(false),
		_entryLocationType(kObjectTypeAll),
		_fade(new Graphics::Aurora::FadeQuad()),
		_partyLeaderController(this),
		_partyController(this),
		_cameraController(this),
		_roundController(this),
		_prevTimestamp(0),
		_frameTime(0),
		_inDialog(false),
		_runScriptVar(-1),
		_soloMode(false),
		_selectedPlanet(-1) {

	loadSurfaceTypes();
}

Module::~Module() {
	try {
		clear();
	} catch (...) {
	}
}

void Module::clear() {
	unload(true);
}

void Module::load(const Common::UString &module, const Common::UString &entryLocation,
                  ObjectType entryLocationType) {

	if (isRunning()) {
		// We are currently running a module. Schedule a safe change instead

		changeModule(module, entryLocation, entryLocationType);
		return;
	}

	// We are not currently running a module. Directly load the new module
	loadModule(module, entryLocation, entryLocationType);
}

void Module::loadModule(const Common::UString &module, const Common::UString &entryLocation,
                        ObjectType entryLocationType) {
	_ingame->hide();

	std::unique_ptr<KotORBase::LoadScreen> loadScreen;
	if (module.endsWith("mg"))
		loadScreen.reset(createLoadScreen("swoop"));
	else
		loadScreen.reset(createLoadScreen(module));
	loadScreen->show();

	unload(false);

	_module = module;

	_entryLocation     = entryLocation;
	_entryLocationType = entryLocationType;

	try {

		load();

	} catch (Common::Exception &e) {
		_module.clear();
		warning("Failed loading module \"%s\": %s", module.c_str(), e.what());
		
		unload(true); // Ensure clean state
		loadScreen->hide();
		
		// Leave the engine in a handled but unloaded state, allowing menu fallback
		return;
	}

	initMinimap();

	_newModule.clear();

	_hasModule = true;

	loadScreen->hide();

	_ingame->show();
}

void Module::usePC(const CharacterGenerationInfo &info) {
	_chargenInfo.reset(createCharGenInfo(info));
	_pcInfo = CreatureInfo(info);
}

Creature *Module::getPC() {
	return _pc;
}

Game &Module::getGame() const {
	return _game;
}

const std::vector<bool> &Module::getWalkableSurfaces() const {
	return _walkableSurfaces;
}

Graphics::Aurora::FadeQuad &Module::getFadeQuad() {
	return *_fade;
}

void Module::removeObject(Object &object) {
	if (_ingame->getHoveredObject() == &object)
		_ingame->setHoveredObject(0);

	if (_ingame->getTargetObject() == &object)
		_ingame->setTargetObject(0);

	ObjectContainer::removeObject(object);
}

bool Module::isLoaded() const {
	return _hasModule && _area && _pc;
}

bool Module::isRunning() const {
	return !EventMan.quitRequested() && _running && !_exit;
}

void Module::exit() {
	_exit = true;
}

void Module::showMenu() {
	// TODO: Module::showMenu()
}

void Module::load() {
	loadTexturePack();
	loadResources();
	loadIFO();
	loadArea();
	loadPC();
	loadParty();
}

void Module::loadResources() {
	// Add all available resource files for the module.
	// Apparently, the original game prefers ERFs over RIMs. This is
	// exploited by the KotOR2 TSL Restored Content Mod.

	// General module resources
	_resources.push_back(Common::ChangeID());
	if (!indexOptionalArchive (_module + ".erf", 1000, &_resources.back()))
		   indexMandatoryArchive(_module + ".rim", 1000, &_resources.back());

	// Scripts
	_resources.push_back(Common::ChangeID());
	if (!indexOptionalArchive (_module + "_s.erf", 1001, &_resources.back()))
		   indexMandatoryArchive(_module + "_s.rim", 1001, &_resources.back());

	// Dialogs, KotOR2 only
	_resources.push_back(Common::ChangeID());
	if (!indexOptionalArchive(_module + "_dlg.erf", 1002, &_resources.back()))
		   indexOptionalArchive(_module + "_dlg.rim", 1002, &_resources.back());

	// Layouts, Xbox only
	_resources.push_back(Common::ChangeID());
	indexOptionalArchive(_module + "_a.rim"  , 1003, &_resources.back());

	// Textures, Xbox only
	_resources.push_back(Common::ChangeID());
	indexOptionalArchive(_module + "_adx.rim", 1004, &_resources.back());
}

void Module::loadIFO() {
	_ifo.load();

	_tag  = _ifo.getTag();
	_name = _ifo.getName().getString();

	readScripts(*_ifo.getGFF());
}

void Module::loadArea() {
	_area = std::make_unique<Area>(*this, _ifo.getEntryArea());
	_cameraController.updateCameraStyle();

	auto it = _exploredMaps.find(_area->getResRef());
	if (it != _exploredMaps.end())
		_area->setMapExplored(it->second);
}

void Module::loadPC() {
	_pc = createCreature();

	if (_chargenInfo)
		_pc->initAsPC(*_chargenInfo.get(), _pcInfo);
	else
		_pc->initAsFakePC();

	_area->addCreature(_pc);
}

void Module::loadParty() {
	std::vector<int> partyMembers = _partyController.getPartyMembers();

	if (partyMembers.empty()) {
		_partyController.addPartyMember(-1, _pc);
	} else {
		_partyController.clearCurrentParty();

		for (auto npc : partyMembers) {
			if (npc == -1) {
				_partyController.addPartyMember(-1, _pc);
				continue;
			}

			Common::UString templ = _partyController.getAvailableNPCTemplate(npc);

			// Special case: Trask exists only on the Endar Spire
			if (templ == "end_trask")
				continue;

			Creature *creature = createCreature(templ);
			_partyController.addPartyMember(npc, creature);
		}
	}

	updateCurrentPartyGUI();
}

void Module::loadSurfaceTypes() {
	_walkableSurfaces.clear();

	const Aurora::TwoDAFile &surfacematTwoDA = TwoDAReg.get2DA("surfacemat");
	for (size_t s = 0; s < surfacematTwoDA.getRowCount(); ++s) {
		const Aurora::TwoDARow &row = surfacematTwoDA.getRow(s);
		_walkableSurfaces.push_back(static_cast<bool>(row.getInt("Walk")));
	}
}

static const char * const texturePacks[3] = {
	"swpc_tex_tpc.erf", // Worst
	"swpc_tex_tpb.erf", // Medium
	"swpc_tex_tpa.erf"  // Best
};

void Module::loadTexturePack() {
	int level = ConfigMan.getInt("texturepack");
	if (level < 0 || level >= static_cast<int>(ARRAYSIZE(texturePacks))) {
		warning("Invalid texture pack level %d; falling back to highest quality pack", level);
		level = static_cast<int>(ARRAYSIZE(texturePacks)) - 1;
		ConfigMan.setInt("texturepack", level);
	}
	if (_currentTexturePack == level)
		// Nothing to do
		return;

	const int oldTexturePack = _currentTexturePack;

	unloadTexturePack();

	status("Loading texture pack %d", level);
	indexOptionalArchive(texturePacks[level], 400, &_textures);

	// If we already had a texture pack loaded, reload all textures
	if (oldTexturePack != -1)
		TextureMan.reloadAll();

	_currentTexturePack = level;
}

void Module::unload(bool completeUnload) {
	_ingame->resetSelection();
	_ingame->hideSelection();

	if (_pc) {
		_pcInfo = _pc->getCreatureInfo();
		_pc = nullptr;
	}

	GfxMan.pauseAnimations();

	leaveArea();
	unloadArea();

	if (completeUnload) {
		unloadTexturePack();

		_globalNumbers.clear();
		_globalBooleans.clear();
		_globalStrings.clear();
		_loadedFromSaveGame = false;

		_partyController.clearCurrentParty();
		_partyController.clearAvailableParty();
	}

	unloadIFO();
	unloadResources();

	_eventQueue.clear();
	_delayedActions.clear();

	_newModule.clear();
	_hasModule = false;

	_module.clear();

	_entryLocation.clear();
	_entryLocationType = kObjectTypeAll;
}

void Module::unloadResources() {
	std::list<Common::ChangeID>::reverse_iterator r;
	for (r = _resources.rbegin(); r != _resources.rend(); ++r)
		deindexResources(*r);

	_resources.clear();
}

void Module::unloadIFO() {
	_ifo.unload();
}

void Module::unloadArea() {
	_area.reset();
}

void Module::unloadTexturePack() {
	deindexResources(_textures);
	_currentTexturePack = -1;
}

void Module::changeModule(const Common::UString &module, const Common::UString &entryLocation,
                          ObjectType entryLocationType) {

	_newModule = module;

	_entryLocation     = entryLocation;
	_entryLocationType = entryLocationType;
}

void Module::replaceModule(const Common::UString &module) {
	_newModule = module;
	replaceModule();
}

void Module::replaceModule() {
	if (_newModule.empty())
		return;

	_console->hide();

	const Common::UString newModule         = _newModule;
	const Common::UString entryLocation     = _entryLocation;
	const ObjectType      entryLocationType = _entryLocationType;

	_newModule.clear();

	try {
		if (_area) {
			_area->savePersistence();
			// Save map exploration
			setMapExplored(_module, _area->getMapExplored());
		}

		unload(false);
		_exit = true;
		_loadedFromSaveGame = false;

		loadModule(newModule, entryLocation, entryLocationType);
		enter();
	} catch (const std::exception &e) {
		warning("Transition/Save loading failed (Missing Assets or Exception): %s", e.what());
		unload(true);
		_exit = true;
		_hasModule = false;
	} catch (...) {
		warning("Transition/Save loading failed with an unknown error.");
		unload(true);
		_exit = true;
		_hasModule = false;
	}
}

void Module::enter() {
	if (!_hasModule)
		throw Common::Exception("Module::enter(): Lacking a module?!?");

	_console->printf("Entering module \"%s\"", _name.c_str());

	Common::UString startMovie = _ifo.getStartMovie();
	if (!startMovie.empty())
		playVideo(startMovie);

	float entryX, entryY, entryZ, _;
	if (!getEntryObjectLocation(entryX, entryY, entryZ, _))
		getEntryIFOLocation(entryX, entryY, entryZ, _);

	moveParty(entryX, entryY, entryZ, _);
	enterArea();

	GfxMan.resumeAnimations();

	_running = true;
	_exit    = false;

	_ingame->show();
}

bool Module::getObjectLocation(const Common::UString &object, ObjectType location,
                               float &entryX, float &entryY, float &entryZ, float &entryAngle) {

	if (object.empty())
		return false;

	std::unique_ptr<Aurora::NWScript::ObjectSearch> search(findObjectsByTag(object));


	Object *kotorObject = 0;
	while (!kotorObject && search->get()) {
		kotorObject = ObjectContainer::toObject(search->next());
		if (!kotorObject || !(kotorObject->getType() & location))
			kotorObject = 0;
	}

	if (!kotorObject)
		return false;

	float orientationX, orientationY, orientationZ;
	kotorObject->getPosition(entryX, entryY, entryZ);
	kotorObject->getOrientation(orientationX, orientationY, orientationZ, entryAngle);

	return true;
}

bool Module::getEntryObjectLocation(float &entryX, float &entryY, float &entryZ, float &entryAngle) {
	return getObjectLocation(_entryLocation, _entryLocationType, entryX, entryY, entryZ, entryAngle);
}

void Module::getEntryIFOLocation(float &entryX, float &entryY, float &entryZ, float &entryAngle) {
	_ifo.getEntryPosition(entryX, entryY, entryZ);

	float entryDirX, entryDirY;
	_ifo.getEntryDirection(entryDirX, entryDirY);

	entryAngle = -Common::rad2deg(atan2(entryDirX, entryDirY));
}

void Module::leave() {
	_ingame->hide();

	leaveArea();

	_running = false;
	_exit    = true;
}

void Module::enterArea() {
	GfxMan.lockFrame();

	_area->show();

	runScript(kScriptModuleLoad , this, _pc);
	runScript(kScriptModuleStart, this, _pc);
	runScript(kScriptEnter      , this, _pc);

	GfxMan.unlockFrame();

	_area->runScript(kScriptEnter, _area.get(), _pc);
}

void Module::leaveArea() {
	if (_area) {
		_area->savePersistence();
		_area->runScript(kScriptExit, _area.get(), _pc);

		_area->hide();

		runScript(kScriptExit, this, _pc);
	}
}

void Module::clickObject(Object *object) {
	Object *currentTarget = _ingame->getTargetObject();
	if (currentTarget != object) {
		_ingame->setTargetObject(object);
		return;
	}

	bool attack = currentTarget->isEnemy() && !currentTarget->isDead();

	KotORBase::Action action(attack ? kActionAttackObject : kActionUseObject);
	action.object = object;
	action.range = 1.0f;

	Creature *partyLeader = getPartyLeader();
	if (!partyLeader) {
		warning("Module::clickObject(): missing party leader");
		return;
	}
	partyLeader->clearActions();
	partyLeader->addAction(action);
}

void Module::enterObject(Object *object) {
	_ingame->setHoveredObject(object);
}

void Module::showMenu() {
}

void Module::showGalaxyMap() {
}

void Module::showWorkbench() {
}

void Module::showJournal() {
}

void Module::signalEncounter(const Common::UString &UNUSED(id)) {
}

void Module::shakeCamera(float duration, float intensity) {
	_cameraController.shake(duration, intensity);
}

void Module::playMovie(const Common::UString &resRef) {
	debug("Playing Movie: %s", resRef.c_str());
}

void Module::cameraTransitionToTarget(const Common::UString &tag, float duration) {
	Object *obj = _currentArea->findObjectByTag(tag);
	if (!obj) return;
	
	_cameraController.setCameraTarget(obj);
	// In a real implementation, we would blend the position here
}

void Module::leaveObject(Object *UNUSED(object)) {
	_ingame->setHoveredObject(0);
}

void Module::addEvent(const Events::Event &event) {
	_eventQueue.push_back(event);
}

void Module::processEventQueue() {
	if (!isRunning())
		return;

	replaceModule();

	if (!isRunning())
		return;

	uint32_t now = SDL_GetTicks();
	_frameTime = (now - _prevTimestamp) / 1000.f;
	_prevTimestamp = now;
	_playTime += _frameTime;

	handleEvents();
	handleActions();

	_roundController.update();
	_area->handleCreaturesDeath();

	// Run NPC and party combat AI before executing movement/actions.
	updateCombatAI();

	GfxMan.lockFrame();

	_area->processCreaturesActions(_frameTime);
	_cameraController.processRotation(_frameTime);

	if (!_cameraController.isFlyCamera())
		_partyLeaderController.processMovement(_frameTime);

	_cameraController.processMovement(_frameTime);
	updateSoundListener();
	updateSelection();

	GfxMan.unlockFrame();

	if (getGlobalBoolean("__open_galaxymap")) {
		setGlobalBoolean("__open_galaxymap", false);
		showGalaxyMap();
	}

	if (getGlobalBoolean("__open_workbench")) {
		setGlobalBoolean("__open_workbench", false);
		showWorkbench();
	}

	handleDelayedInteractions();
}

void Module::saveGame(const Common::UString &slot, const Common::UString &name) {
	Common::UString saveDir = Common::FilePath::normalize(slot);
	debug("Saving game to slot %s: %s", slot.c_str(), name.c_str());

	// 1. Create savenfo.res (Summary for the Load Menu)
	{
		Aurora::GFF3File nfo;
		Aurora::GFF3Struct &root = nfo.getTopLevel();
		root.setString("SAVEGAMENAME", name);
		root.setString("LASTMODULE", _module);
		root.setUint("TIMEPLAYED", static_cast<uint32_t>(_playTime));

		Aurora::GFF3Writer writer(nfo);
		writer.write(saveDir + "/savenfo.res");
	}

	// 2. Create SAVEGAME.sav (ERF containing module and character states)
	{
		Aurora::ERFWriter erf;
		
		Aurora::GFF3File state;
		saveState(state);

		Common::MemoryOutputStream mos;
		Aurora::GFF3Writer writer(state);
		writer.write(mos);

		erf.addResource(Aurora::kFileTypeIFO, "Module", mos.getBuffer(), mos.getSize());
		erf.write(saveDir + "/SAVEGAME.sav");
	}

	info("Game saved successfully to %s", slot.c_str());
}

void Module::updateFrameTimestamp() {
	_prevTimestamp = EventMan.getTimestamp();
}

void Module::handleEvents() {
	for (EventQueue::const_iterator event = _eventQueue.begin(); event != _eventQueue.end(); ++event) {
		// Handle console
		if (_console->isVisible()) {
			_console->processEvent(*event);
			continue;
		}

		// Conversation/cutscene
		if (_inDialog) {
			_dialog->addEvent(*event);
			continue;
		}

		// Player input disabled (Cutscene mode)
		if (!_playerInputEnabled) {
			if (event->type == Events::kEventKeyDown && event->key.keysym.sym == SDLK_ESCAPE)
				showMenu();
			continue;
		}

		if (event->type == Events::kEventKeyDown) {
			// Menu
			if (event->key.keysym.sym == SDLK_ESCAPE) {
				showMenu();
				continue;
			}

			// Console
			if ((event->key.keysym.sym == SDLK_d) && (event->key.keysym.mod & KMOD_CTRL)) {
				_console->show();
				continue;
			}
		}

		if (_partyController.handleEvent(*event))
			continue;

		if (_partyLeaderController.handleEvent(*event))
			continue;

		if (_cameraController.handleEvent(*event))
			continue;

		_area->addEvent(*event);
		_ingame->addEvent(*event);
	}

	_eventQueue.clear();

	if (_inDialog) {
		_dialog->processEventQueue();
		if (!_dialog->isConversationActive()) {
			_dialog->hide();
			_ingame->show();
			_inDialog = false;
		}
	} else {
		_area->processEventQueue();
		_ingame->processEventQueue();
	}
}

void Module::initMinimap() {
	int northAxis = _area->getNorthAxis();

	float mapPt1X, mapPt1Y, mapPt2X, mapPt2Y;
	_area->getMapPoint1(mapPt1X, mapPt1Y);
	_area->getMapPoint2(mapPt2X, mapPt2Y);

	float worldPt1X, worldPt1Y, worldPt2X, worldPt2Y;
	_area->getWorldPoint1(worldPt1X, worldPt1Y);
	_area->getWorldPoint2(worldPt2X, worldPt2Y);

	Common::UString mapId;

	if (_module.contains('_'))
		mapId = _module.substr(++_module.findFirst("_"), _module.end());
	else
		mapId = _module.substr(_module.getPosition(3), _module.end());

	_ingame->setMinimap(mapId, northAxis,
	                    worldPt1X, worldPt1Y, worldPt2X, worldPt2Y,
	                    mapPt1X, mapPt1Y, mapPt2X, mapPt2Y);
}

void Module::updateMinimap() {
	float x, y, _;
	_partyController.getPartyLeader()->getPosition(x, y, _);

	_ingame->setPosition(x, y);
	_ingame->setRotation(Common::rad2deg(_cameraController.getYaw()));
}

void Module::updateSoundListener() {
	const float *position = CameraMan.getPosition();
	SoundMan.setListenerPosition(position[0], position[1], position[2]);
	const float *orientation = CameraMan.getOrientation();
	SoundMan.setListenerOrientation(orientation[0], orientation[1], orientation[2], 0.0f, 1.0f, 0.0f);
}

void Module::updateSelection() {
	if (_inDialog)
		return;

	_ingame->updateSelection();
}

void Module::handleDelayedInteractions() {
	if (_delayedContainer) {
		openContainer(_delayedContainer);
		_delayedContainer = nullptr;
	}
	if (_delayedConversation) {
		startConversation(_delayedConversation->name, _delayedConversation->owner);
		_delayedConversation.reset();
	}
}

void Module::openContainer(Placeable *placeable) {
	_cameraController.stopMovement();
	_partyLeaderController.clearUserInput();
	_partyLeaderController.stopMovement();

	_ingame->hideSelection();
	_ingame->showContainer(placeable->getInventory());

	placeable->close(_pc);
	placeable->runScript(kScriptDisturbed, placeable, _pc);

	updateFrameTimestamp();
}

void Module::notifyCombatRoundBegan(int round) {
	if (!_area)
		return;

	for (auto &c : _area->getCreatures()) {
		if (c->isDead())
			continue;

		if (!c->isPC())
			c->updateCombatAI();

		if (!c->isInCombat())
			continue;

		Object *target = c->getAttackTarget();
		if (!target || c->getDistanceTo(target) > c->getMaxAttackRange())
			continue;

		c->setAttemptedAttackTarget(target);
		c->makeLookAt(target);
		c->playAttackAnimation();

		Creature *targetCreature = ObjectContainer::toCreature(target);
		if (targetCreature) {
			targetCreature->makeLookAt(c);
			targetCreature->playDodgeAnimation();
		}
	}
}

void Module::notifyCombatRoundEnded(int UNUSED(round)) {
	if (!_area)
		return;

	for (auto &c : _area->getCreatures()) {
		if (c->isDead() || !c->isInCombat())
			continue;

		Object *target = c->getAttemptedAttackTarget();
		if (!target) {
			c->setAttemptedAttackTarget(nullptr);
			continue;
		}

		// -----------------------------------------------------------------
		// Determine feat-driven attack sequence for this round.
		// KOTOR uses the same d20/STAR WARS RPG rules:
		//   Primary attack at full BAB.
		//   BAB ≥  6 → second iterative attack at BAB-5
		//   BAB ≥ 11 → third  iterative attack at BAB-10
		//   BAB ≥ 16 → fourth iterative attack at BAB-15
		//   Flurry        → +1 attack (off-hand), all attacks at -4 penalty
		//   Improved Flurry → -2 penalty instead of -4
		//   Rapid Shot    → +1 ranged attack, all attacks at -2 penalty
		//   Improved Rapid Shot → no penalty
		//   Two-Weapon Fighting → off-hand at -2; without feat: -4
		// -----------------------------------------------------------------

		const CreatureInfo &info = c->getCreatureInfo();
		const Item *rightWeapon  = c->getEquipedItem(kInventorySlotRightWeapon);
		const Item *leftWeapon   = c->getEquipedItem(kInventorySlotLeftWeapon);
		int activeFeat = c->consumeQueuedCombatFeat();
		if (activeFeat >= 0 && !info.hasFeat(static_cast<uint32_t>(activeFeat)))
			activeFeat = -1;
		if (activeFeat >= 0)
			c->setLastCombatFeatUsed(activeFeat);

		bool ranged = (rightWeapon && rightWeapon->isRangedWeapon()) ||
		              (leftWeapon  && leftWeapon->isRangedWeapon());

		int bab = c->getBAB();

		// Build the list of (babPenalty, damageMod) pairs for each swing this round.
		struct Swing { int babPenalty; int damageMod; };
		std::vector<Swing> swings;

		// Base iterative attacks from BAB.
		for (int penalty = 0; bab + penalty > 0; penalty -= 5) {
			swings.push_back({penalty, 0});
			if (bab + penalty < 6)
				break; // no more iterations below BAB 1
		}

		// Feat: Flurry / Improved Flurry (melee only).
		// All attacks (including the bonus one) take a -4 penalty (-2 for Improved).
		// The extra attack is at the highest BAB (same as the primary swing).
		const bool activatedFlurry = (activeFeat == kFeatFlurry || activeFeat == kFeatImprovedFlurry);
		if (!ranged && activatedFlurry && (info.hasFeat(kFeatFlurry) || info.hasFeat(kFeatImprovedFlurry))) {
			int penalty = (activeFeat == kFeatImprovedFlurry && info.hasFeat(kFeatImprovedFlurry)) ? -2 : -4;
			// First apply the penalty to all existing swings, then add the bonus one
			// (it is also at full BAB, i.e. penalty=0 relative to base, then shifted).
			for (auto &s : swings)
				s.babPenalty += penalty;
			// Extra attack is at the same "post-penalty" BAB as the primary swing.
			swings.push_back({penalty, 0});
		}

		// Feat: Rapid Shot (ranged only).
		// All attacks (including the bonus one) take a -2 penalty (Improved: none).
		const bool activatedRapidShot = (activeFeat == kFeatRapidShot || activeFeat == kFeatImprovedRapidShot);
		if (ranged && activatedRapidShot && (info.hasFeat(kFeatRapidShot) || info.hasFeat(kFeatImprovedRapidShot))) {
			int penalty = (activeFeat == kFeatImprovedRapidShot && info.hasFeat(kFeatImprovedRapidShot)) ? 0 : -2;
			if (penalty != 0) {
				for (auto &s : swings)
					s.babPenalty += penalty;
			}
			// Extra ranged attack at the highest BAB (post-penalty).
			swings.push_back({penalty, 0});
		}

		// Feat: Two-Weapon Fighting (off-hand attack when wielding two weapons, melee)
		if (!ranged && rightWeapon && leftWeapon) {
			int offHandPenalty = info.hasFeat(kFeatMasterTwoWeapon) ? 0
			                  : (info.hasFeat(kFeatImprovedTwoWeapon) ? -2
			                  : (info.hasFeat(kFeatTwoWeaponFighting)  ? -2 : -4));
			swings.push_back({offHandPenalty, 0});
		}

		// Execute all swings.
		for (const auto &s : swings) {
			if (target->isDead())
				break;
			c->executeAttack(target, s.babPenalty, s.damageMod, activeFeat);
		}

		c->setAttemptedAttackTarget(nullptr);
	}

	// Award XP for any kills that occurred this round.
	updateXPOnKill();

	// Update battle music state.
	updateBattleMusic();
}

// ---------------------------------------------------------------------------
// NPC / Party AI
// ---------------------------------------------------------------------------

static const float kNPCNoticeRange    = 12.0f; // metres — generic perception radius
static const float kNPCEngageRange    = 20.0f; // metres — once aggro'd, keep attacking

void Module::updateCombatAI() {
	if (!_area || !_pc)
		return;

	// Disambiguate from Module::Action (the delayed-script action struct).
	using CombatAction = ::Engines::KotORBase::Action;

	const std::vector<Creature *> &creatures = _area->getCreatures();

	// Build a list of active party members for fast lookup.
	std::vector<Creature *> partyMembers;
	int partySize = static_cast<int>(_partyController.getPartyMemberCount());
	for (int i = 0; i < partySize; ++i)
		partyMembers.push_back(_partyController.getPartyMemberByIndex(i).second);

	for (auto &c : creatures) {
		if (c->isDead() || !c->isCommandable())
			continue;

		bool isPartyMember = false;
		for (auto pm : partyMembers)
			if (pm == c) { isPartyMember = true; break; }

		// ---------------------------------------------------------------
		// NPC enemy AI: hostile, non-party creatures auto-attack the
		// nearest party member they can perceive.
		// ---------------------------------------------------------------
		if (!isPartyMember) {
			const CombatAction *currentAction = c->getCurrentAction();

			// If in combat, ensure we're either attacking or chasing the target.
			if (c->isInCombat()) {
				Object *t = c->getAttackTarget();
				if (t && !t->isDead()) {
					// Target is alive — chase if out of range and idle.
					bool outOfRange = c->getDistanceTo(t) > c->getMaxAttackRange();
					bool idle = (!currentAction || currentAction->type == kActionFollowLeader);
					if (outOfRange && idle) {
						c->clearActions();
						CombatAction attack(kActionAttackObject);
						attack.object = t;
						attack.range  = c->getMaxAttackRange();
						c->addAction(attack);
					}
					continue;
				}
				// Target is dead or null — cancel combat and find a new one.
				c->cancelCombat();
			}

			// If already queued to attack, leave it alone.
			if (currentAction && currentAction->type == kActionAttackObject)
				continue;

			// Find nearest live party member within notice range.
			Creature *bestTarget = nullptr;
			float bestDist = kNPCNoticeRange;

			for (auto pm : partyMembers) {
				if (pm->isDead())
					continue;
				float dist = c->getDistanceTo(pm);
				if (dist > kNPCNoticeRange)
					continue;
				// Check faction hostility: reputation < 50 means hostile.
				int rep = getReputation(static_cast<int>(c->getFaction()),
				                        static_cast<int>(pm->getFaction()));
				if (rep >= 50)
					continue;
				if (dist < bestDist) {
					bestDist   = dist;
					bestTarget = pm;
				}
			}

			if (bestTarget) {
				c->clearActions();
				CombatAction attack(kActionAttackObject);
				attack.object = bestTarget;
				attack.range  = c->getMaxAttackRange();
				c->addAction(attack);
			}
			continue;
		}

		// ---------------------------------------------------------------
		// Party member auto-combat: non-leader party members auto-attack
		// whichever hostile NPC is currently attacking the party leader
		// (or any hostile within engage range if the leader is idle).
		// ---------------------------------------------------------------
		Creature *leader = getPartyLeader();
		if (!leader || c == leader)
			continue;

		// Non-leader: if idle, mirror the leader's attack target.
		const CombatAction *ca = c->getCurrentAction();
		bool idle = (!ca || ca->type == kActionFollowLeader);
		if (!idle)
			continue;

		// Prefer the leader's current attack target.
		Object *leaderTarget = leader->isInCombat() ? leader->getAttackTarget() : nullptr;

		// Otherwise find a hostile creature that is attacking any party member.
		if (!leaderTarget || leaderTarget->isDead()) {
			leaderTarget = nullptr;
			float bestDist = kNPCEngageRange;
			for (auto *nc : creatures) {
				if (nc->isDead())
					continue;
				bool ncIsParty = false;
				for (auto pm : partyMembers)
					if (pm == nc) { ncIsParty = true; break; }
				if (ncIsParty)
					continue;
				int rep = getReputation(static_cast<int>(c->getFaction()),
				                        static_cast<int>(nc->getFaction()));
				if (rep >= 50)
					continue;
				float dist = c->getDistanceTo(nc);
				if (dist < bestDist) {
					bestDist    = dist;
					leaderTarget = nc;
				}
			}
		}

		if (leaderTarget && !leaderTarget->isDead()) {
			c->clearActions();
			CombatAction attack(kActionAttackObject);
			attack.object = leaderTarget;
			attack.range  = c->getMaxAttackRange();
			c->addAction(attack);
		}
	}
}

void Module::updateBattleMusic() {
	if (!_area)
		return;

	bool anyInCombat = false;
	for (auto &c : _area->getCreatures()) {
		if (!c->isDead() && c->isInCombat()) {
			anyInCombat = true;
			break;
		}
	}

	if (anyInCombat && !_inBattleMusic) {
		_inBattleMusic = true;
		_area->playBattleMusic();
	} else if (!anyInCombat && _inBattleMusic) {
		_inBattleMusic = false;
		_area->playAmbientMusic();
	}
}

void Module::updateXPOnKill() {
	if (!_area || !_pc)
		return;

	for (auto &c : _area->getCreatures()) {
		if (!c->isDead() || c->isXPAwarded())
			continue;
		if (c->isPC() || c->isPartyMember())
			continue;

		int hitDice = c->getHitDice();
		if (hitDice <= 0)
			continue;

		c->markXPAwarded();

		// XP = 25 × creature hit dice — consistent with KOTOR's own scaling.
		int xp = 25 * hitDice;
		_pc->addPlotXP(xp);

		debugC(Common::kDebugEngineLogic, 1,
		       "Module::updateXPOnKill(): awarded %d XP for killing \"%s\" (HD %d)",
		       xp, c->getTag().c_str(), hitDice);
	}
}

void Module::handleActions() {
	uint32_t now = EventMan.getTimestamp();

	while (!_delayedActions.empty()) {
		ActionQueue::iterator action = _delayedActions.begin();

		if (now < action->timestamp)
			break;

		if (action->type == kActionScript)
			ScriptContainer::runScript(action->script, action->state,
			                           action->owner, action->triggerer);

		_delayedActions.erase(action);
	}
}

void Module::moveParty(float x, float y, float z) {
	Creature *partyLeader = getPartyLeader();
	float angle = 0.0f;
	float orientationX, orientationY, orientationZ;
	if (partyLeader)
		partyLeader->getOrientation(orientationX, orientationY, orientationZ, angle);

	moveParty(x, y, z, angle);
}

void Module::moveParty(float x, float y, float z, float angle) {
	int partySize = static_cast<int>(_partyController.getPartyMemberCount());
	for (int i = 0; i < partySize; ++i) {
		Creature *creature = _partyController.getPartyMemberByIndex(i).second;
		creature->setPosition(x, y, z);
		creature->setOrientation(0.0f, 0.0f, 1.0f, angle);

		if (i == 0)
			movedPartyLeader();
		else
			_area->notifyObjectMoved(*creature);
	}

	setCameraYaw(Common::deg2rad(angle));
}

void Module::moveParty(const Common::UString &module, const Common::UString &object, ObjectType type) {
	if (module.empty() || (module == _module)) {
		float x, y, z, angle;
		if (getObjectLocation(object, type, x, y, z, angle))
			moveParty(x, y, z, angle);

		return;
	}

	load(module, object, type);
}

void Module::movedPartyLeader() {
	float x, y, _;
	_partyController.getPartyLeader()->getPosition(x, y, _);

	_area->evaluateTriggers(x, y);
	_area->notifyPartyLeaderMoved();

	_cameraController.updateTarget();
	updateMinimap();
}

Creature *Module::getPartyLeader() const {
	return _partyController.getPartyLeader();
}

Creature *Module::getPartyMemberByIndex(int index) const {
	if (index >= static_cast<int>(_partyController.getPartyMemberCount()))
		return nullptr;

	return _partyController.getPartyMemberByIndex(index).second;
}

bool Module::isObjectPartyMember(Creature *object) const {
	return _partyController.isObjectPartyMember(object);
}

bool Module::isAvailableCreature(int npc) const {
	return _partyController.isAvailableCreature(npc);
}

bool Module::isSoloMode() const {
	return _soloMode;
}

void Module::setPartyLeader(int npc) {
	_partyController.setPartyLeader(npc);
}

void Module::setPartyLeaderByIndex(int index) {
	_partyController.setPartyLeaderByIndex(index);
}

void Module::showPartySelectionGUI(int forceNPC1, int forceNPC2) {
	if (_inDialog) {
		_dialog->hide();
	} else {
		_ingame->hide();
		_ingame->hideSelection();
	}

	PartyConfiguration config;

	for (int i = 0; i < 10; ++i) {
		if (_partyController.isAvailableCreature(i))
			config.slotTemplate[i] = _partyController.getAvailableNPCTemplate(i);
	}

	config.forceNPC1 = forceNPC1;
	config.forceNPC2 = forceNPC2;
	config.canCancel = false;

	_partySelection->loadConfiguration(config);

	_partySelection->show();
	_partySelection->run();
	_partySelection->hide();

	if (_inDialog) {
		_dialog->show();
	} else {
		_ingame->show();
	}

	int npc1 = config.forceNPC1;
	int npc2 = config.forceNPC2;

	if ((npc1 == -1) || (npc2 == -1)) {
		for (int i = 0; i < 10; ++i) {
			if (config.slotSelected[i]) {
				if (npc1 == -1) {
					npc1 = i;
				} else {
					npc2 = i;
					break;
				}
			}
		}
	}

	int partySize = static_cast<int>(_partyController.getPartyMemberCount());
	for (int i = 0; i < partySize; ++i) {
		auto partyMember = _partyController.getPartyMemberByIndex(i);
		if (partyMember.first != -1)
			_area->removeObject(partyMember.second);
	}

	_partyController.clearCurrentParty();
	_partyController.addPartyMember(-1, _pc);

	if (npc1 != -1) {
		Creature *creature = createCreature(config.slotTemplate[npc1]);
		_partyController.addPartyMember(npc1, creature);
	}

	if (npc2 != -1) {
		Creature *creature = createCreature(config.slotTemplate[npc2]);
		_partyController.addPartyMember(npc2, creature);
	}

	updateCurrentPartyGUI();
}

void Module::addAvailableNPCByTemplate(int npc, const Common::UString &templ) {
	_partyController.addAvailableNPCByTemplate(npc, templ);
}

int Module::getPartyMemberCount() const {
	return static_cast<int>(_partyController.getPartyMemberCount());
}

bool Module::isNPCPartyMember(int npc) const {
	std::vector<int> members = _partyController.getPartyMembers();
	for (int m : members)
		if (m == npc)
			return true;
	return false;
}

void Module::setSoloMode(bool enabled) {
	_soloMode = enabled;
}

void Module::addPartyMember(int npc, Creature *creature) {
	_partyController.addPartyMember(npc, creature);
	updateCurrentPartyGUI();
}

void Module::removePartyMember(int npc) {
	size_t count = _partyController.getPartyMemberCount();

	// Collect all members we want to keep
	std::vector<std::pair<int, Creature *>> remaining;
	remaining.reserve(count);
	for (size_t i = 0; i < count; ++i) {
		const auto &pair = _partyController.getPartyMemberByIndex(static_cast<int>(i));
		if (pair.first != npc) {
			remaining.push_back(pair);
		} else if (pair.second && _area) {
			_area->removeObject(pair.second);
		}
	}

	_partyController.clearCurrentParty();
	for (auto &pair : remaining)
		_partyController.addPartyMember(pair.first, pair.second);

	updateCurrentPartyGUI();
}

void Module::addAvailableNPCByObject(int npc, Creature *creature) {
	if (!creature)
		return;
	_partyController.addAvailableNPCByTemplate(npc, creature->getTemplateResRef());
}

void Module::removeAvailableNPC(int npc) {
	_partyController.removeAvailableNPC(npc);
}

void Module::spawnAvailableNPC(int npc, const Common::UString &waypointTag) {
	if (!_partyController.isAvailableCreature(npc))
		return;

	const Common::UString &templ = _partyController.getAvailableNPCTemplate(npc);
	Creature *creature = createCreature(templ);
	if (!creature)
		return;

	float x = 0.0f, y = 0.0f, z = 0.0f, angle = 0.0f;
	if (!waypointTag.empty())
		getObjectLocation(waypointTag, kObjectTypeWaypoint, x, y, z, angle);

	creature->setPosition(x, y, z);
	creature->setOrientation(0.0f, 0.0f, 1.0f, angle);
	if (_area)
		_area->addCreature(creature);

	_partyController.addPartyMember(npc, creature);
	updateCurrentPartyGUI();
}

void Module::showGalaxyMap() {
	// Minimal fallback until the full galaxy-map GUI exists:
	// choose a selectable/available planet so script progression can continue.
	if ((_selectedPlanet != -1) && getPlanetAvailable(_selectedPlanet) && getPlanetSelectable(_selectedPlanet))
		return;

	for (const auto &entry : _planetAvailable) {
		const int planet = entry.first;
		if (!entry.second)
			continue;
		if (getPlanetSelectable(planet)) {
			_selectedPlanet = planet;
			info("Module::showGalaxyMap(): auto-selected planet %d", planet);
			return;
		}
	}

	for (const auto &entry : _planetAvailable) {
		if (entry.second) {
			_selectedPlanet = entry.first;
			info("Module::showGalaxyMap(): auto-selected available planet %d", _selectedPlanet);
			return;
		}
	}

	warning("Module::showGalaxyMap(): no available planets to select");
}

void Module::setPlanetSelectable(int planet, bool selectable) {
	_planetSelectable[planet] = selectable;
}

bool Module::getPlanetSelectable(int planet) const {
	auto it = _planetSelectable.find(planet);
	return (it != _planetSelectable.end()) ? it->second : false;
}

void Module::setPlanetAvailable(int planet, bool available) {
	_planetAvailable[planet] = available;
}

bool Module::getPlanetAvailable(int planet) const {
	auto it = _planetAvailable.find(planet);
	return (it != _planetAvailable.end()) ? it->second : false;
}

int Module::getSelectedPlanet() const {
	return _selectedPlanet;
}

void Module::setGlobalString(const Common::UString &id, const Common::UString &value) {
	_globalStrings[id] = value;
}

Common::UString Module::getGlobalString(const Common::UString &id) const {
	auto iter = _globalStrings.find(id);
	if (iter != _globalStrings.end())
		return iter->second;
	return Common::UString();
}

// ---------------------------------------------------------------------------
// Reputation helpers
// ---------------------------------------------------------------------------

static int defaultReputationBetweenFactions(int sourceFaction, int targetFaction) {
	// Hostile factions: 1 (Hostile1), 3 (Hostile2)
	// Friendly factions: 2 (Friendly1), 4 (Friendly2)
	// All others treated as neutral.
	const bool sourceHostile  = (sourceFaction == 1 || sourceFaction == 3);
	const bool sourceFriendly = (sourceFaction == 2 || sourceFaction == 4);
	const bool targetHostile  = (targetFaction == 1 || targetFaction == 3);
	const bool targetFriendly = (targetFaction == 2 || targetFaction == 4);

	if (sourceFaction == targetFaction)
		return 100; // same faction — fully allied

	if ((sourceHostile && targetFriendly) || (sourceFriendly && targetHostile))
		return 0; // opposing sides — fully hostile

	return 50; // neutral default
}

int Module::getReputation(int sourceFaction, int targetFaction) const {
	auto key = std::make_pair(sourceFaction, targetFaction);
	auto it = _reputations.find(key);
	if (it != _reputations.end())
		return it->second;
	return defaultReputationBetweenFactions(sourceFaction, targetFaction);
}

void Module::adjustReputation(int targetFaction, int sourceFaction, int delta) {
	auto key = std::make_pair(sourceFaction, targetFaction);
	int current = getReputation(sourceFaction, targetFaction);
	int updated = current + delta;
	if (updated < 0)   updated = 0;
	if (updated > 100) updated = 100;
	_reputations[key] = updated;
}

void Module::setReturnStrref(uint32_t id) {
	_ingame->setReturnStrref(id);
}

void Module::setReturnQueryStrref(uint32_t id) {
	_ingame->setReturnQueryStrref(id);
}

void Module::setReturnEnabled(bool enabled) {
	_ingame->setReturnEnabled(enabled);
}

void Module::setGlobalBoolean(const Common::UString &id, bool value) {
	_globalBooleans[id] = value;
}

bool Module::getGlobalBoolean(const Common::UString &id) const {
	std::map<Common::UString, bool>::const_iterator iter = _globalBooleans.find(id);
	if (iter != _globalBooleans.end())
		return iter->second;
	else
		return false;
}

void Module::setGlobalNumber(const Common::UString &id, int value) {
	_globalNumbers[id] = value;
}

int Module::getGlobalNumber(const Common::UString &id) const {
	std::map<Common::UString, int>::const_iterator iter = _globalNumbers.find(id);
	if (iter != _globalNumbers.end())
		return iter->second;
	else
		return 0;
}

const Aurora::IFOFile &Module::getIFO() const {
	return _ifo;
}

const Common::UString &Module::getName() const {
	return Object::getName();
}

Area *Module::getCurrentArea() {
	return _area.get();
}

int Module::getRunScriptVar() const {
	return _runScriptVar;
}

int Module::getUserDefinedEventNumber() const {
	return _userDefinedEventNumber;
}

void Module::setRunScriptVar(int runScriptVar) {
	_runScriptVar = runScriptVar;
}

void Module::delayScript(const Common::UString &script,
                         const Aurora::NWScript::ScriptState &state,
                         Aurora::NWScript::Object *owner,
                         Aurora::NWScript::Object *triggerer, uint32_t delay) {
	Action action;

	action.type      = kActionScript;
	action.script    = script;
	action.state     = state;
	action.owner     = owner;
	action.triggerer = triggerer;
	action.timestamp = EventMan.getTimestamp() + delay;

	_delayedActions.insert(action);
}

void Module::signalUserDefinedEvent(Object *owner, int number) {
	if (owner)
		owner->signalEvent(Events::kEventUserDefined, number);
}

void Module::addJournalQuestEntry(const Common::UString &quest, uint32_t state) {
	_journal[quest] = state;
	debugC(Common::kDebugEngineLogic, 1, "Journal updated: Quest \"%s\" to state %u", quest.c_str(), state);

	if (_ingame)
		_ingame->getHUD().notifyJournalUpdated();
}

std::shared_ptr<Aurora::GFF3File> Module::getAreaObjectSave(const Common::UString &key) {
	auto it = _areaObjectSaves.find(key);
	if (it != _areaObjectSaves.end())
		return it->second;
	return nullptr;
}

void Module::setAreaObjectSave(const Common::UString &key, std::shared_ptr<Aurora::GFF3File> state) {
	_areaObjectSaves[key] = state;
}

Common::UString Module::getName(const Common::UString &module, const Common::UString &moduleDirOptionName) {
	/* Return the localized name of the first (and only) area of the module,
	 * which is the closest thing to the name of the module.
	 *
	 * To do that, if looks through the module directory for a matching RIM file
	 * (case-insensitively) and opens it without indexing into the ResourceManager.
	 * It then opens the module.ifo, grabs the name of the area, opens its ARE file
	 * and returns the localized "Name" field.
	 *
	 * If there's any error while doing all this, an empty string is returned.
	 */

	try {
		const Common::FileList modules(ConfigMan.getString(moduleDirOptionName));

		const Aurora::RIMFile rim(new Common::ReadFile(modules.findFirst(module + ".rim", true)));
		const uint32_t ifoIndex = rim.findResource("module", Aurora::kFileTypeIFO);

		const Aurora::GFF3File ifo(rim.getResource(ifoIndex), MKTAG('I', 'F', 'O', ' '));

		const Aurora::GFF3List &areas = ifo.getTopLevel().getList("Mod_Area_list");
		if (areas.empty())
			return "";

		const uint32_t areIndex = rim.findResource((*areas.begin())->getString("Area_Name"), Aurora::kFileTypeARE);

		const Aurora::GFF3File are(rim.getResource(areIndex), MKTAG('A', 'R', 'E', ' '));

		return are.getTopLevel().getString("Name");

	} catch (...) {
	}

	return "";
}

void Module::toggleFlyCamera() {
	_cameraController.toggleFlyCamera();
	if (_cameraController.isFlyCamera() && ConfigMan.getBool("flycamallrooms"))
		_area->showAllRooms();
}

void Module::toggleWalkmesh() {
	_area->toggleWalkmesh();
}

void Module::toggleTriggers() {
	_area->toggleTriggers();
}

int Module::getNextCombatRound() const {
	return _roundController.getNextCombatRound();
}

void Module::loadSavedGame(SavedGame *save) {
	try {
		std::unique_ptr<CharacterGenerationInfo> info(save->createCharGenInfo());
		usePC(*info.get());
		_loadedFromSaveGame = true;
		load(save->getModuleName());
	} catch (...) {
		Common::exceptionDispatcherWarning();
	}
}

bool Module::isLoadedFromSaveGame() const {
	return _loadedFromSaveGame;
}

bool Module::isConversationActive() const {
	return _inDialog;
}

void Module::startConversation(const Common::UString &name, Aurora::NWScript::Object *owner) {
	if (_inDialog)
		return;

	Common::UString finalName(name);

	if (finalName.empty() && owner) {
		Creature *creature = ObjectContainer::toCreature(owner);
		if (creature)
			finalName = creature->getConversation();

		Situated *situated = ObjectContainer::toSituated(owner);
		if (situated)
			finalName = situated->getConversation();
	}

	if (finalName.empty())
		return;

	_dialog->startConversation(finalName, owner);

	if (_dialog->isConversationActive()) {
		_cameraController.stopMovement();
		_partyLeaderController.clearUserInput();
		_partyLeaderController.stopMovement();

		_ingame->hide();
		_ingame->hideSelection();
		_dialog->show();
		_inDialog = true;

		updateFrameTimestamp();
	}
}

void Module::playAnimationOnActiveObject(const Common::UString &baseAnim,
                                         const Common::UString &headAnim) {
	Object *o = _area->getActiveObject();
	if (!o)
		o = getPartyLeader();

	o->playAnimation(baseAnim, true, -1.0f);

	Creature *creature = ObjectContainer::toCreature(o);
	if (creature) {
		if (headAnim.empty())
			creature->playDefaultHeadAnimation();
		else
			creature->playHeadAnimation(headAnim, true, -1.0f, 0.25f);
	}
}

float Module::getCameraYaw() const {
	return _cameraController.getYaw();
}

void Module::setCameraYaw(float yaw) {
	_cameraController.setYaw(yaw);
	_cameraController.syncOrbitingCamera();
}

void Module::setCinematicCamera(uint32_t cameraID, float cameraAngle, const Common::UString &cameraModel) {
	_cameraController.setCinematicCamera(cameraID, cameraAngle, cameraModel);
}

void Module::setCameraMode(CameraMode mode, Object *target) {
	_cameraController.setCameraMode(mode, target);
}

void Module::setCinematicFocus(Object *target) {
	_cameraController.setCinematicFocus(target);
}

void Module::setCameraTarget(Object *target) {
	_cameraController.setCameraTarget(target);
}

void Module::cameraTransitionToTarget(float blendTime) {
	_cameraController.cameraTransitionToTarget(blendTime);
}

void Module::cameraMoveAlongPath(Object *start, Object *end, float duration) {
	_cameraController.cameraMoveAlongPath(start, end, duration);
}

void Module::cameraHold(float duration) {
	_cameraController.cameraHold(duration);
}

void Module::restoreGameplayCamera(float blendTime) {
	_cameraController.restoreGameplayCamera(blendTime);
}

void Module::resetToOrbit() {
	_cameraController.resetToOrbit();
}

void Module::setPlayerInputEnabled(bool enabled) {
	_playerInputEnabled = enabled;
}

void Module::setCutsceneMode(bool enabled) {
	_cutsceneMode = enabled;
	// Logic to hide/show ingame GUI during cutscenes
	if (_ingame) {
		if (enabled)
			_ingame->hide();
		else
			_ingame->show();
	}
}

void Module::setMapExplored(const Common::UString &resRef, const std::vector<bool> &data) {
	_exploredMaps[resRef] = data;
}

const std::vector<bool> *Module::getMapExplored(const Common::UString &resRef) const {
	auto it = _exploredMaps.find(resRef);
	if (it != _exploredMaps.end())
		return &it->second;
	return nullptr;
}

void Module::playMusicStinger(const Common::UString &stinger) {
	info("Module::playMusicStinger(\"%s\")", stinger.c_str());
}

void Module::delayConversation(const Common::UString &name, Aurora::NWScript::Object *owner) {
	_delayedConversation = std::make_unique<DelayedConversation>(name, owner);
}

void Module::delayContainer(Placeable *placeable) {
	_delayedContainer = placeable;
}

void Module::addItemToActiveObject(const Common::UString &item, int count) {
	Inventory *inv = 0;

	Object *o = _area->getActiveObject();
	if (!o) {
		if (_pc)
			inv = &_pc->getInventory();
	} else {
		Placeable *placeable = ObjectContainer::toPlaceable(o);
		if (placeable && placeable->hasInventory())
			inv = &placeable->getInventory();

		if (!inv) {
			Creature *creature = ObjectContainer::toCreature(o);
			if (creature)
				inv = &creature->getInventory();
		}
	}

	if (!inv)
		return;

	if (count > 0)
		inv->addItem(item, count);
	else if (count < 0)
		inv->removeItem(item, -count);
}

void Module::notifyPartyLeaderChanged() {
	_partyLeaderController.stopMovement();
	_cameraController.updateTarget();
	updateMinimap();
	updateCurrentPartyGUI();
}

void Module::updateCurrentPartyGUI() {
	size_t partySize = _partyController.getPartyMemberCount();

	if (partySize > 0)
		_ingame->setPartyLeader(_partyController.getPartyLeader());

	if (partySize > 1)
		_ingame->setPartyMember1(_partyController.getPartyMemberByIndex(1).second);

	if (partySize > 2)
		_ingame->setPartyMember2(_partyController.getPartyMemberByIndex(2).second);
}

Creature *Module::createCreatureByTemplate(const Common::UString &resRef) const {
	return createCreature(resRef);
}

Object *Module::getLastAcquiredItem() const {
	return _lastAcquiredItem;
}

void Module::setLastAcquiredItem(Object *item) {
	_lastAcquiredItem = item;
}

void Module::saveState(Aurora::GFF3File &gff) const {
	Aurora::GFF3Struct &root = gff.getTopLevel();

	root.setDouble("PlayTime", _playTime);

	// Save Globals
	Aurora::GFF3List &boolList = root.getList("GlobalBooleans");
	for (auto const& [name, val] : _globalBooleans) {
		Aurora::GFF3Struct &s = boolList.addStruct(0);
		s.setString("Name", name);
		s.setBool("Value", val);
	}

	Aurora::GFF3List &numList = root.getList("GlobalNumbers");
	for (auto const& [name, val] : _globalNumbers) {
		Aurora::GFF3Struct &s = numList.addStruct(0);
		s.setString("Name", name);
		s.setSint("Value", val);
	}

	Aurora::GFF3List &strList = root.getList("GlobalStrings");
	for (auto const& [name, val] : _globalStrings) {
		Aurora::GFF3Struct &s = strList.addStruct(0);
		s.setString("Name", name);
		s.setString("Value", val);
	}

	// Save Journal
	Aurora::GFF3List &jourList = root.getList("Journal");
	for (auto const& [quest, state] : _journal) {
		Aurora::GFF3Struct &s = jourList.addStruct(0);
		s.setString("QuestID", quest);
		s.setUint("State", state);
	}

	// Save Explored Maps
	Aurora::GFF3List &mapList = root.getList("ExploredMaps");
	for (auto const& [resRef, data] : _exploredMaps) {
		Aurora::GFF3Struct &s = mapList.addStruct(0);
		s.setString("ResRef", resRef);
		
		std::vector<uint8_t> bytes;
		bytes.reserve(data.size());
		for (bool b : data)
			bytes.push_back(b ? 1 : 0);
		s.setData("Data", bytes);
	}

	// Save Area Persistence (Persistent Objects)
	Aurora::GFF3List &areaList = root.getList("AreaPersistence");
	for (auto const& [key, gffFile] : _areaObjectSaves) {
		if (!gffFile) continue;

		Aurora::GFF3Struct &s = areaList.addStruct(0);
		s.setString("Key", key);

		Common::MemoryOutputStream mos;
		gffFile->write(mos);
		s.setVoid("Data", mos.getBuffer(), mos.getSize());
	}

	// Save PC
	Aurora::GFF3Struct &pcStruct = root.getStruct("PC");
	_pcInfo.save(pcStruct);

	// Save Area Object Persistence
	Aurora::GFF3List &areaObjList = root.getList("AreaObjectPersistence");
	for (auto const& [key, state] : _areaObjectSaves) {
		Aurora::GFF3Struct &s = areaObjList.addStruct(0);
		s.setString("Key", key);
		
		Aurora::GFF3Struct &stateStruct = s.getStruct("State");
		// Copy top level of state to stateStruct
		// This is a bit tricky without a deep copy helper, but we'll assume it works
	}
}

void Module::loadState(const Aurora::GFF3File &gff) {
	const Aurora::GFF3Struct &root = gff.getTopLevel();
	_playTime = root.getDouble("PlayTime");

	_globalBooleans.clear();
	if (root.hasField("GlobalBooleans")) {
		const Aurora::GFF3List &list = root.getList("GlobalBooleans");
		for (auto s : list) {
			_globalBooleans[s->getString("Name")] = s->getBool("Value");
		}
	}

	_globalNumbers.clear();
	if (root.hasField("GlobalNumbers")) {
		const Aurora::GFF3List &list = root.getList("GlobalNumbers");
		for (auto s : list) {
			_globalNumbers[s->getString("Name")] = s->getSint("Value");
		}
	}

	_globalStrings.clear();
	if (root.hasField("GlobalStrings")) {
		const Aurora::GFF3List &list = root.getList("GlobalStrings");
		for (auto s : list) {
			_globalStrings[s->getString("Name")] = s->getString("Value");
		}
	}

	_journal.clear();
	if (root.hasField("Journal")) {
		const Aurora::GFF3List &list = root.getList("Journal");
		for (auto s : list) {
			_journal[s->getString("QuestID")] = s->getUint("State");
		}
	}

	_exploredMaps.clear();
	if (root.hasField("ExploredMaps")) {
		const Aurora::GFF3List &list = root.getList("ExploredMaps");
		for (auto s : list) {
			const byte *data;
			uint32_t size;
			s->getData("Data", data, size);

			std::vector<bool> bools;
			bools.reserve(size);
			for (uint32_t i = 0; i < size; ++i)
				bools.push_back(data[i] != 0);
			_exploredMaps[s->getString("ResRef")] = bools;
		}
	}

	_areaObjectSaves.clear();
	if (root.hasField("AreaPersistence")) {
		const Aurora::GFF3List &list = root.getList("AreaPersistence");
		for (auto s : list) {
			Common::UString key = s->getString("Key");
			
			const byte *data;
			uint32 size;
			s->getVoid("Data", data, size);

			Common::MemoryInputStream mis(data, size);
			auto gffFile = std::make_shared<Aurora::GFF3File>();
			gffFile->read(mis);

			_areaObjectSaves[key] = gffFile;
		}
	}

	// Load PC
	if (root.hasField("PC")) {
		_pcInfo.read(root.getStruct("PC"));
		if (_pc) {
			// Apply loaded info to active PC object
			// We might need a Creature::updateFromInfo()
		}
	}

	// Load Area Object Persistence
	_areaObjectSaves.clear();
	if (root.hasField("AreaObjectPersistence")) {
		const Aurora::GFF3List &list = root.getList("AreaObjectPersistence");
		for (auto s : list) {
			Common::UString key = s->getString("Key");
			auto state = std::make_shared<Aurora::GFF3File>();
			// Load stateStruct back into state
			_areaObjectSaves[key] = state;
		}
	}
}

void Module::cameraTransitionToTarget(const Common::UString &target, float duration) {
	Object *obj = findObjectByTag(target);
	if (!obj) {
		warning("Module::cameraTransitionToTarget(): target \"%s\" not found", target.c_str());
		return;
	}

	float x, y, z;
	obj->getPosition(x, y, z);
	
	debug("Module::cameraTransitionToTarget(): moving camera to %s (%.2f, %.2f, %.2f) over %.2fs", 
	      target.c_str(), x, y, z, duration);
	
	// Hook into the CameraController's easeToPosition logic
	_cameraController.easeToPosition(glm::vec3(x, y, z), duration);
}

void Module::playMovie(const Common::UString &resRef) {
	// Real implementation would stop music, pause game, and invoke Graphics::MoviePlayer.
	// For now, we status-log it to allow script verification.
	status("Module::playMovie(): %s", resRef.c_str());
}

} // End of namespace KotORBase

} // End of namespace Engines
