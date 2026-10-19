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
 *  A running swoop race or turret minigame: models, camera and input.
 */

#include <algorithm>
#include <cmath>
#include <map>

#include "external/glm/geometric.hpp"
#include "external/glm/gtc/matrix_transform.hpp"
#include "external/glm/gtc/quaternion.hpp"

#include "src/common/configman.h"
#include "src/common/debug.h"
#include "src/common/error.h"
#include "src/common/maths.h"
#include "src/common/util.h"

#include "src/aurora/gff3file.h"
#include "src/aurora/lytfile.h"

#include "src/graphics/graphics.h"
#include "src/graphics/camera.h"

#include "src/graphics/aurora/model.h"
#include "src/graphics/aurora/modelnode.h"

#include "src/sound/types.h"

#include "src/engines/aurora/model.h"
#include "src/engines/aurora/util.h"

#include "src/engines/kotorbase/minigameruntime.h"
#include "src/engines/kotorbase/minigamedesc.h"
#include "src/engines/kotorbase/module.h"
#include "src/engines/kotorbase/area.h"
#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/cameracontroller.h"
#include "src/engines/kotorbase/cinematicshot.h"

#include "src/engines/kotorbase/script/container.h"

namespace Engines {

namespace KotORBase {

namespace {

/** The node of a track model its follower hangs from (KotOR.js ModuleMGPlayer.setTrack). */
const char *const kNodeModelHook  = "modelhook";
/** The camera node of a player's camera model (KotOR.js FollowerCamera). */
const char *const kNodeCameraHook = "camerahook";
/** Where a gun bank's bullets leave the gun model (KotOR.js ModuleMGGunBank). */
const char *const kNodeBulletHook = "bullethook0";

/** Enemies idle in this animation while alive (KotOR.js ModuleMGEnemy.update). */
const char *const kAnimationReady = "ready_01";
const char *const kAnimationFire  = "fire";

/** A dead enemy stays on screen this long at least, for its death animation. */
const float kDeadEnemyLinger = 1.5f;

glm::vec3 translation(const glm::mat4 &m) {
	return glm::vec3(m[3]);
}

/** Put a model at a world transform (translation and rotation). */
void placeModel(Graphics::Aurora::Model *model, const glm::mat4 &world) {
	if (!model)
		return;

	const glm::vec3 position = translation(world);
	model->setPosition(position.x, position.y, position.z);

	glm::mat3 rotation(world);
	for (int i = 0; i < 3; ++i) {
		const float length = glm::length(rotation[i]);
		if (length > 0.0f)
			rotation[i] /= length;
	}

	const glm::quat q = glm::normalize(glm::quat_cast(rotation));
	const float angle = glm::angle(q);
	if (std::fabs(angle) < 1e-5f) {
		model->setOrientation(0.0f, 0.0f, 1.0f, 0.0f);
		return;
	}

	const glm::vec3 axis = glm::axis(q);
	model->setOrientation(axis.x, axis.y, axis.z, Common::rad2deg(angle));
}

/** Rotation that turns +Y into the direction. */
glm::mat4 orientAlong(const glm::vec3 &direction) {
	const glm::vec3 forward = glm::normalize(direction);

	glm::vec3 up(0.0f, 0.0f, 1.0f);
	if (std::fabs(glm::dot(forward, up)) > 0.99f)
		up = glm::vec3(1.0f, 0.0f, 0.0f);

	const glm::vec3 right = glm::normalize(glm::cross(forward, up));
	const glm::vec3 realUp = glm::cross(right, forward);

	glm::mat4 m(1.0f);
	m[0] = glm::vec4(right, 0.0f);
	m[1] = glm::vec4(forward, 0.0f);
	m[2] = glm::vec4(realUp, 0.0f);
	return m;
}

Graphics::Aurora::Model *loadModel(const Common::UString &resRef, const char *what) {
	if (resRef.empty())
		return nullptr;

	Graphics::Aurora::Model *model = loadModelObject(resRef);
	if (!model)
		warning("Minigame: failed to load %s model \"%s\"", what, resRef.c_str());

	return model;
}

void hideModel(Graphics::Aurora::Model *model) {
	if (model)
		model->hide();
}

} // End of anonymous namespace

MinigameRuntime::MinigameRuntime(Module &module, Area &area) :
	_module(&module), _area(&area), _minigame(SwoopMinigame::get()) {

}

MinigameRuntime::~MinigameRuntime() {
	_minigame.setHost(nullptr);
	_minigame.reset();

	hideAll();

	if (_loaded) {
		const Area::CameraStyle &style = _area->getCameraStyle();
		GfxMan.setPerspective(style.viewAngle, 0.1f, 10000.0f);
	}
}

bool MinigameRuntime::load() {
	if (!_area->isMinigame())
		return false;

	MinigameDesc desc;
	try {
		desc = MinigameDesc::parse(_area->getMinigame());
	} catch (...) {
		Common::exceptionDispatcherWarning("Minigame: failed to read the area's MiniGame struct");
		return false;
	}

	if (desc.type == kMinigameTypeNone) {
		warning("Minigame: area \"%s\" has a MiniGame struct of unknown type", _area->getResRef().c_str());
		return false;
	}

	std::vector<Common::UString> obstacleNames;
	for (const Aurora::LYTFile::PlacedModel &obstacle : _area->getLayout().getObstacles())
		obstacleNames.push_back(obstacle.model);

	_minigame.start(desc, obstacleNames);
	_minigame.setHost(this);
	_minigame.setReverseVerticalInput(ConfigMan.getBool("reverseminigameyaxis", false));

	status("Loading %s minigame: %u enemies, %u obstacles",
	       (desc.type == kMinigameTypeSwoop) ? "swoop race" : "turret",
	       static_cast<uint>(desc.enemies.size()), static_cast<uint>(obstacleNames.size()));

	GfxMan.lockFrame();

	loadFollower(_player, desc.player, _minigame.getPlayer(), true);

	for (size_t i = 0; i < desc.enemies.size(); ++i) {
		_enemies.push_back(std::make_unique<FollowerModels>());
		loadFollower(*_enemies.back(), desc.enemies[i], _minigame.getEnemy(i), false);
	}

	loadObstacles();

	// The party doesn't take part: the player is the swoop or the turret.
	for (int i = 0; i < _module->getPartyMemberCount(); ++i)
		if (Creature *member = _module->getPartyMemberByIndex(i))
			member->hide();

	GfxMan.unlockFrame();

	_loaded = true;
	applyPerspective();

	// Place everything once, so the first scripts see real positions.
	placeAll(0.0f);
	return true;
}

void MinigameRuntime::start() {
	if (_loaded)
		_minigame.runCreateEvents();
}

void MinigameRuntime::loadFollower(FollowerModels &follower, const MinigameFollowerDesc &desc,
                                   MinigameObject *object, bool isPlayer) {

	follower.object = object;

	// The track: a LYT model the follower hangs from.
	bool foundTrack = false;
	for (const Aurora::LYTFile::PlacedModel &track : _area->getLayout().getTracks()) {
		if (!track.model.equalsIgnoreCase(desc.track))
			continue;

		follower.trackPosition = glm::vec3(track.x, track.y, track.z);
		follower.track.reset(loadModel(track.model, "track"));
		foundTrack = true;
		break;
	}

	if (!foundTrack)
		warning("Minigame: track \"%s\" isn't in the area layout", desc.track.c_str());

	if (follower.track) {
		follower.track->setPosition(follower.trackPosition.x, follower.trackPosition.y, follower.trackPosition.z);
		follower.track->show();

		if (!follower.track->getNode(kNodeModelHook))
			warning("Minigame: track \"%s\" has no \"%s\"", desc.track.c_str(), kNodeModelHook);

		// Enemies ride their track's animation, looping (KotOR.js ModuleMGEnemy.update).
		// The player's track is moved by the player's speed instead.
		const std::vector<Common::UString> &animations = follower.track->getAnimationNames();
		if (!isPlayer && !animations.empty()) {
			follower.trackAnimation = animations.front();
			follower.trackAnimationLength = follower.track->getAnimationLength(follower.trackAnimation);
			follower.track->playAnimation(follower.trackAnimation, true, -1.0f);
			follower.trackMoving = true;
		}
	}

	if (isPlayer && !desc.camera.empty())
		follower.camera.reset(loadModel(desc.camera, "camera"));

	for (const MinigameModelDesc &part : desc.models) {
		PartModel partModel;
		partModel.model.reset(loadModel(part.model, "minigame object"));
		partModel.rotating = part.rotating;

		if (!partModel.model)
			continue;

		// KotOR.js hides a model that is the camera model.
		if (!desc.camera.equalsIgnoreCase(part.model))
			partModel.model->show();

		if (!isPlayer && partModel.model->hasAnimation(kAnimationReady))
			partModel.model->playAnimation(kAnimationReady, true, -1.0f);

		follower.parts.push_back(std::move(partModel));
	}

	if (follower.parts.empty())
		warning("Minigame: \"%s\" has no models", object ? object->getTag().c_str() : "");

	for (const MinigameGunBankDesc &bank : desc.gunBanks) {
		std::unique_ptr<Graphics::Aurora::Model> gun(loadModel(bank.gunModel, "gun"));
		if (gun)
			gun->show();

		follower.guns.push_back(std::move(gun));
	}
}

void MinigameRuntime::loadObstacles() {
	const Aurora::LYTFile::PlacedModelArray &obstacles = _area->getLayout().getObstacles();

	for (size_t i = 0; i < obstacles.size(); ++i) {
		const glm::vec3 position(obstacles[i].x, obstacles[i].y, obstacles[i].z);

		std::unique_ptr<Graphics::Aurora::Model> model(loadModel(obstacles[i].model, "obstacle"));

		glm::vec3 min = position - glm::vec3(1.0f), max = position + glm::vec3(1.0f);
		if (model) {
			model->setPosition(position.x, position.y, position.z);
			model->show();

			float minX, minY, minZ, maxX, maxY, maxZ;
			model->getAbsoluteBound(minX, minY, minZ, maxX, maxY, maxZ);
			if ((maxX >= minX) && (maxY >= minY) && (maxZ >= minZ) &&
			    ((maxX > minX) || (maxY > minY) || (maxZ > minZ))) {
				min = glm::vec3(minX, minY, minZ);
				max = glm::vec3(maxX, maxY, maxZ);
			}
		}

		_minigame.setObstacleBounds(i, min, max);
		_obstacles.push_back(std::move(model));
	}
}

void MinigameRuntime::hideAll() {
	auto hideFollower = [](FollowerModels &follower) {
		hideModel(follower.track.get());
		hideModel(follower.camera.get());
		for (PartModel &part : follower.parts)
			hideModel(part.model.get());
		for (std::unique_ptr<Graphics::Aurora::Model> &gun : follower.guns)
			hideModel(gun.get());
	};

	hideFollower(_player);
	for (std::unique_ptr<FollowerModels> &enemy : _enemies)
		hideFollower(*enemy);

	for (std::unique_ptr<Graphics::Aurora::Model> &obstacle : _obstacles)
		hideModel(obstacle.get());

	for (std::pair<const uint32_t, std::unique_ptr<Graphics::Aurora::Model>> &bullet : _bullets)
		hideModel(bullet.second.get());
}

MinigameRuntime::FollowerModels *MinigameRuntime::getModels(MinigameObject &object) {
	if (object.getMinigameKind() == kMinigameObjectPlayer)
		return &_player;

	if (object.getMinigameKind() == kMinigameObjectEnemy) {
		const size_t index = object.getMinigameIndex();
		return (index < _enemies.size()) ? _enemies[index].get() : nullptr;
	}

	return nullptr;
}

// .--- Per frame

void MinigameRuntime::update(float dt) {
	if (!_loaded || !_minigame.isActive())
		return;

	_minigame.setInput(_input);
	_minigame.step(dt);

	if (_accelerateRequested) {
		_accelerateRequested = false;
		_minigame.onAccelerate();
	}

	if (_brakeRequested) {
		_brakeRequested = false;
		_minigame.onBrake();
	}

	if (!_minigame.isActive())
		return;

	placeAll(dt);

	_minigame.dispatchHits(_minigame.detectHits());

	std::vector<MinigameObject *> trackLoops;
	trackLoops.swap(_trackLoops);
	for (MinigameObject *follower : trackLoops)
		if (_minigame.isActive())
			_minigame.onTrackLoop(*follower);

	// Enemies fire when the player is in their sights.
	std::map<MinigameObject *, std::vector<size_t>> enemyShots;
	for (const MinigameFireRequest &request : _minigame.getEnemyFireRequests())
		enemyShots[request.shooter].push_back(request.bank);

	for (std::pair<MinigameObject *const, std::vector<size_t>> &shots : enemyShots)
		if (FollowerModels *models = getModels(*shots.first))
			fireGuns(*models, *shots.first, shots.second);

	// The player fires as long as the button is held, at the guns' rate.
	if (_firing && _minigame.getPlayerState().alive && !_minigame.getPlayerState().gunBanks.empty()) {
		std::vector<size_t> banks;
		for (size_t i = 0; i < _minigame.getPlayerState().gunBanks.size(); ++i)
			banks.push_back(i);

		fireGuns(_player, *_player.object, banks);
	}

	// KotOR.js runs the minigame heartbeats every frame.
	_minigame.runHeartbeats();
}

void MinigameRuntime::placeAll(float dt) {
	GfxMan.lockFrame();

	placeFollower(_player, dt, true);
	for (std::unique_ptr<FollowerModels> &enemy : _enemies)
		placeFollower(*enemy, dt, false);

	placeBullets();
	placeCamera();

	GfxMan.unlockFrame();
}

void MinigameRuntime::placeFollower(FollowerModels &follower, float dt, bool isPlayer) {
	if (!follower.object)
		return;

	glm::vec3 trackPosition = follower.trackPosition;
	if (isPlayer && (_minigame.getType() == kMinigameTypeSwoop))
		trackPosition += _minigame.getPlayerOffset();

	const glm::mat4 trackWorld = glm::translate(glm::mat4(1.0f), trackPosition);

	glm::mat4 hookWorld = trackWorld;
	if (follower.track) {
		follower.track->setPosition(trackPosition.x, trackPosition.y, trackPosition.z);

		if (follower.trackMoving && (follower.trackAnimationLength > 0.0f)) {
			follower.trackTime += dt;
			while (follower.trackTime >= follower.trackAnimationLength) {
				follower.trackTime -= follower.trackAnimationLength;
				_trackLoops.push_back(follower.object);
			}
		}

		if (const Graphics::Aurora::ModelNode *hook = follower.track->getNode(kNodeModelHook))
			hookWorld = trackWorld * composeNodeTransform(*hook);
	}

	follower.container = hookWorld;
	if (isPlayer && (_minigame.getType() == kMinigameTypeTurret)) {
		// Turret aim: KotOR.js rotates the player's container in YZX order.
		const glm::vec3 &rotation = _minigame.getPlayerOffset();

		follower.container = glm::rotate(follower.container, Common::deg2rad(rotation.y), glm::vec3(0, 1, 0));
		follower.container = glm::rotate(follower.container, Common::deg2rad(rotation.z), glm::vec3(0, 0, 1));
		follower.container = glm::rotate(follower.container, Common::deg2rad(rotation.x), glm::vec3(1, 0, 0));
	}

	for (PartModel &part : follower.parts)
		placeModel(part.model.get(), part.rotating ? follower.container : trackWorld);

	placeModel(follower.camera.get(), follower.container);

	const std::vector<MinigameGunBank> *banks = nullptr;
	if (MinigameFollower *state = _minigame.getFollower(follower.object))
		banks = &state->gunBanks;

	for (size_t i = 0; i < follower.guns.size(); ++i) {
		if (!follower.guns[i])
			continue;

		glm::mat4 gunWorld = follower.container;
		if (banks && (i < banks->size())) {
			const Common::UString hookName = Common::String::format("gunbank%u", (*banks)[i].desc.bankID);
			for (PartModel &part : follower.parts) {
				if (!part.model)
					continue;

				if (const Graphics::Aurora::ModelNode *hook = part.model->getNode(hookName)) {
					gunWorld = (part.rotating ? follower.container : trackWorld) * composeNodeTransform(*hook);
					break;
				}
			}
		}

		placeModel(follower.guns[i].get(), gunWorld);
	}

	// A dead enemy vanishes once its death animation had time to play.
	if (follower.hideTimer >= 0.0f) {
		follower.hideTimer -= dt;
		if (follower.hideTimer < 0.0f) {
			for (PartModel &part : follower.parts)
				hideModel(part.model.get());
			for (std::unique_ptr<Graphics::Aurora::Model> &gun : follower.guns)
				hideModel(gun.get());
		}
	}

	const glm::vec3 forward = glm::mat3(follower.container) * glm::vec3(0.0f, 1.0f, 0.0f);
	_minigame.setFollowerPose(*follower.object, translation(follower.container), forward);
}

void MinigameRuntime::placeBullets() {
	const std::list<MinigameBullet> &bullets = _minigame.getBullets();

	std::map<uint32_t, bool> alive;
	for (const MinigameBullet &bullet : bullets) {
		if (bullet.spent)
			continue;

		alive[bullet.id] = true;

		std::map<uint32_t, std::unique_ptr<Graphics::Aurora::Model>>::iterator model = _bullets.find(bullet.id);
		if (model == _bullets.end()) {
			Common::UString resRef;
			if (bullet.shooter)
				if (MinigameGunBank *bank = _minigame.getGunBank(bullet.shooter, static_cast<int>(bullet.bank)))
					resRef = bank->desc.bullet.model;

			std::unique_ptr<Graphics::Aurora::Model> bulletModel(loadModel(resRef, "bullet"));
			if (bulletModel)
				bulletModel->show();

			model = _bullets.emplace(bullet.id, std::move(bulletModel)).first;
		}

		if (model->second)
			placeModel(model->second.get(),
			           glm::translate(glm::mat4(1.0f), bullet.position) * orientAlong(bullet.direction));
	}

	for (std::map<uint32_t, std::unique_ptr<Graphics::Aurora::Model>>::iterator b = _bullets.begin();
	     b != _bullets.end(); ) {

		if (alive.find(b->first) != alive.end()) {
			++b;
			continue;
		}

		hideModel(b->second.get());
		b = _bullets.erase(b);
	}
}

void MinigameRuntime::placeCamera() {
	glm::mat4 cameraWorld(1.0f);
	bool haveHook = false;

	// The camera model's "camerahook", or one in the player's own models.
	if (_player.camera)
		if (const Graphics::Aurora::ModelNode *hook = _player.camera->getNode(kNodeCameraHook)) {
			cameraWorld = _player.container * composeNodeTransform(*hook);
			haveHook = true;
		}

	for (size_t i = 0; !haveHook && (i < _player.parts.size()); ++i) {
		if (!_player.parts[i].model)
			continue;

		if (const Graphics::Aurora::ModelNode *hook = _player.parts[i].model->getNode(kNodeCameraHook)) {
			cameraWorld = _player.container * composeNodeTransform(*hook);
			haveHook = true;
		}
	}

	glm::vec3 position, direction;
	if (haveHook) {
		position  = translation(cameraWorld);
		direction = lookDirectionFromTransform(cameraWorld);
	} else {
		// No camera model: chase the player from behind and above.
		const glm::vec3 player  = translation(_player.container);
		const glm::vec3 forward = glm::normalize(glm::mat3(_player.container) * glm::vec3(0.0f, 1.0f, 0.0f));

		position  = player - forward * 6.0f + glm::vec3(0.0f, 0.0f, 2.5f);
		direction = (player + forward * 10.0f) - position;
	}

	if (glm::length(direction) < 0.0001f)
		direction = glm::vec3(0.0f, 1.0f, 0.0f);

	float pitch, yaw;
	directionToOrientation(direction, pitch, yaw);

	CameraMan.setPosition(position.x, position.y, position.z);
	CameraMan.setOrientation(pitch, 0.0f, Common::rad2deg(yaw));
	CameraMan.update();
}

void MinigameRuntime::applyPerspective() {
	const float viewAngle = _minigame.getDesc().cameraViewAngle;
	const float nearClip  = _minigame.getCameraNearClip();
	const float farClip   = _minigame.getCameraFarClip();

	const float fov  = ((viewAngle > 1.0f) && (viewAngle < 179.0f)) ? viewAngle : 55.0f;
	const float nearPlane = (nearClip > 0.0f) ? nearClip : 0.1f;
	const float farPlane  = (farClip > nearPlane) ? farClip : 10000.0f;

	GfxMan.setPerspective(fov, nearPlane, farPlane);
}

glm::vec3 MinigameRuntime::getMuzzle(FollowerModels &follower, size_t bank) {
	if ((bank < follower.guns.size()) && follower.guns[bank]) {
		Graphics::Aurora::Model &gun = *follower.guns[bank];

		float x, y, z, ax, ay, az, angle;
		gun.getPosition(x, y, z);
		gun.getOrientation(ax, ay, az, angle);

		glm::mat4 gunWorld = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
		if ((angle != 0.0f) && ((ax != 0.0f) || (ay != 0.0f) || (az != 0.0f)))
			gunWorld = glm::rotate(gunWorld, Common::deg2rad(angle), glm::vec3(ax, ay, az));

		if (const Graphics::Aurora::ModelNode *hook = gun.getNode(kNodeBulletHook))
			return translation(gunWorld * composeNodeTransform(*hook));

		return glm::vec3(x, y, z);
	}

	return translation(follower.container);
}

void MinigameRuntime::fireGuns(FollowerModels &follower, MinigameObject &shooter, const std::vector<size_t> &banks) {
	MinigameFollower *state = _minigame.getFollower(&shooter);
	if (!state || !state->alive)
		return;

	bool fired = false;
	for (size_t bank : banks) {
		GfxMan.lockFrame();
		const glm::vec3 muzzle = getMuzzle(follower, bank);
		GfxMan.unlockFrame();

		if (!_minigame.fire(shooter, bank, muzzle, state->forward))
			continue;

		fired = true;

		if ((bank < follower.guns.size()) && follower.guns[bank] &&
		    follower.guns[bank]->hasAnimation(kAnimationFire))
			follower.guns[bank]->playAnimation(kAnimationFire, true);
	}

	if (fired)
		_minigame.onFire(shooter);
}

// '---

// .--- Input

bool MinigameRuntime::handleEvent(const Events::Event &event) {
	switch (event.type) {
		case Events::kEventKeyDown:
			return handleKey(event, true);

		case Events::kEventKeyUp:
			return handleKey(event, false);

		case Events::kEventMouseDown:
			if (event.button.button == SDL_BUTTON_LEFT) {
				_firing = true;
				return true;
			}
			return false;

		case Events::kEventMouseUp:
			if (event.button.button == SDL_BUTTON_LEFT) {
				_firing = false;
				return true;
			}
			return false;

		default:
			return false;
	}
}

bool MinigameRuntime::handleKey(const Events::Event &event, bool down) {
	switch (event.key.keysym.scancode) {
		case SDL_SCANCODE_W:
		case SDL_SCANCODE_UP:
			// A swoop accelerates on the press (the player's OnAccelerate).
			if (down && !_input.up && (_minigame.getType() == kMinigameTypeSwoop))
				_accelerateRequested = true;
			_input.up = down;
			return true;

		case SDL_SCANCODE_S:
		case SDL_SCANCODE_DOWN:
			if (down && !_input.down && (_minigame.getType() == kMinigameTypeSwoop))
				_brakeRequested = true;
			_input.down = down;
			return true;

		case SDL_SCANCODE_A:
		case SDL_SCANCODE_LEFT:
			_input.left = down;
			return true;

		case SDL_SCANCODE_D:
		case SDL_SCANCODE_RIGHT:
			_input.right = down;
			return true;

		case SDL_SCANCODE_SPACE:
		case SDL_SCANCODE_RETURN:
			// Without guns (a swoop), fire just runs the player's OnFire.
			if (down && !_firing && _minigame.getPlayerState().gunBanks.empty() && _player.object)
				_minigame.onFire(*_player.object);
			_firing = down;
			return true;

		default:
			return false;
	}
}

// '---

// .--- MinigameHost

void MinigameRuntime::runScript(const Common::UString &script, MinigameObject &self) {
	debugC(Common::kDebugEngineLogic, 2, "Minigame: running \"%s\" on \"%s\"",
	       script.c_str(), self.getTag().c_str());

	ScriptContainer::runScript(script, &self, _module->getPC());
}

void MinigameRuntime::playAnimation(MinigameObject &object, const Common::UString &animation, bool loop) {
	FollowerModels *models = getModels(object);
	if (!models)
		return;

	GfxMan.lockFrame();
	for (PartModel &part : models->parts)
		if (part.model && part.model->hasAnimation(animation))
			part.model->playAnimation(animation, true, loop ? -1.0f : 0.0f);
	GfxMan.unlockFrame();
}

void MinigameRuntime::removeAnimation(MinigameObject &object, const Common::UString &animation) {
	FollowerModels *models = getModels(object);
	if (!models)
		return;

	GfxMan.lockFrame();
	for (PartModel &part : models->parts) {
		if (!part.model || !part.model->hasAnimation(animation))
			continue;

		if (part.model->hasAnimation(kAnimationReady) && (object.getMinigameKind() == kMinigameObjectEnemy))
			part.model->playAnimation(kAnimationReady, true, -1.0f);
		else
			part.model->playDefaultAnimation();
	}
	GfxMan.unlockFrame();
}

void MinigameRuntime::onDeath(MinigameObject &object) {
	if (object.getMinigameKind() == kMinigameObjectObstacle) {
		const size_t index = object.getMinigameIndex();
		if ((index < _obstacles.size()) && _obstacles[index])
			_obstacles[index]->hide();
		return;
	}

	FollowerModels *models = getModels(object);
	if (!models)
		return;

	// The track stops (KotOR.js ModuleMGEnemy.update).
	models->trackMoving = false;

	if (object.getMinigameKind() == kMinigameObjectEnemy) {
		float linger = kDeadEnemyLinger;
		for (PartModel &part : models->parts)
			if (part.model)
				linger = std::max(linger, part.model->getAnimationLength("die"));

		models->hideTimer = linger;
	}
}

void MinigameRuntime::playSound(const Common::UString &sound, MinigameObject *UNUSED(source)) {
	if (!sound.empty())
		::Engines::playSound(sound, Sound::kSoundTypeSFX);
}

void MinigameRuntime::onCameraClipChanged() {
	if (_loaded)
		applyPerspective();
}

// '---

} // End of namespace KotORBase

} // End of namespace Engines
