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
 *
 *  The simulation itself is SwoopMinigame; this places the track, player,
 *  enemy, obstacle, gun and bullet models, follows the player with the
 *  camera, turns key presses into steering and fire, and runs the
 *  minigame scripts. Ported from KotOR.js (ModuleMiniGame, ModuleMGPlayer,
 *  ModuleMGEnemy, ModuleMGTrack, ModuleMGGunBank, ModuleMGGunBullet,
 *  FollowerCamera's minigame branch and the MG* key actions of
 *  IngameControls).
 */

#ifndef ENGINES_KOTORBASE_MINIGAMERUNTIME_H
#define ENGINES_KOTORBASE_MINIGAMERUNTIME_H

#include <map>
#include <memory>
#include <vector>

#include "external/glm/vec3.hpp"
#include "external/glm/mat4x4.hpp"

#include "src/common/ustring.h"

#include "src/events/types.h"

#include "src/engines/kotorbase/swoopminigame.h"

namespace Graphics {
	namespace Aurora {
		class Model;
	}
}

namespace Engines {

namespace KotORBase {

class Module;
class Area;

class MinigameRuntime : public MinigameHost {
public:
	MinigameRuntime(Module &module, Area &area);
	~MinigameRuntime();

	/** Load the minigame of the area. False if the area has no usable one. */
	bool load();

	/** Run the OnCreate scripts. */
	void start();

	/** Handle an input event. Returns true if the minigame used it. */
	bool handleEvent(const Events::Event &event);

	/** Advance the minigame by a frame. */
	void update(float dt);

	// MinigameHost
	void runScript(const Common::UString &script, MinigameObject &self) override;
	void playAnimation(MinigameObject &object, const Common::UString &animation, bool loop) override;
	void removeAnimation(MinigameObject &object, const Common::UString &animation) override;
	void onDeath(MinigameObject &object) override;
	void playSound(const Common::UString &sound, MinigameObject *source) override;
	void onCameraClipChanged() override;

private:
	struct PartModel {
		std::unique_ptr<Graphics::Aurora::Model> model;
		bool rotating { true };
	};

	/** The models of the player or an enemy. */
	struct FollowerModels {
		MinigameObject *object { nullptr };

		glm::vec3 trackPosition { 0.0f };
		std::unique_ptr<Graphics::Aurora::Model> track;
		Common::UString trackAnimation;
		float trackAnimationLength { 0.0f };
		float trackTime { 0.0f };
		bool trackMoving { false };

		std::vector<PartModel> parts;
		std::unique_ptr<Graphics::Aurora::Model> camera;
		std::vector<std::unique_ptr<Graphics::Aurora::Model>> guns; ///< One per gun bank.

		glm::mat4 container { 1.0f }; ///< World transform of the rotating parts.
		float hideTimer { -1.0f };    ///< Seconds until a dead follower vanishes.
	};

	Module *_module;
	Area *_area;
	SwoopMinigame &_minigame;

	bool _loaded { false };

	FollowerModels _player;
	std::vector<std::unique_ptr<FollowerModels>> _enemies;
	std::vector<std::unique_ptr<Graphics::Aurora::Model>> _obstacles;
	std::map<uint32_t, std::unique_ptr<Graphics::Aurora::Model>> _bullets;

	MinigameInput _input;
	bool _firing { false };
	bool _accelerateRequested { false };
	bool _brakeRequested { false };

	std::vector<MinigameObject *> _trackLoops; ///< OnTrackLoop events to run this frame.

	void loadFollower(FollowerModels &follower, const MinigameFollowerDesc &desc, MinigameObject *object,
	                  bool isPlayer);
	void loadObstacles();
	void hideAll();

	FollowerModels *getModels(MinigameObject &object);

	/** Place all models and the camera. */
	void placeAll(float dt);
	/** Place a follower's models and tell the simulation where it is. */
	void placeFollower(FollowerModels &follower, float dt, bool isPlayer);
	void placeBullets();
	void placeCamera();
	void applyPerspective();

	/** Where a gun bank's bullets start, in world coordinates. */
	glm::vec3 getMuzzle(FollowerModels &follower, size_t bank);
	void fireGuns(FollowerModels &follower, MinigameObject &shooter, const std::vector<size_t> &banks);

	bool handleKey(const Events::Event &event, bool down);
};

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_MINIGAMERUNTIME_H
