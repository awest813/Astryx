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
 *  The ARE "MiniGame" struct of KotOR swoop race and turret areas.
 *
 *  Field names follow the retail ARE files (as read by KotOR.js'
 *  ModuleMiniGame, ModuleMGPlayer, ModuleMGEnemy, ModuleMGGunBank,
 *  ModuleMGGunBullet and its Forge minigame editor), including the
 *  misspelled labels some retail files carry.
 */

#ifndef ENGINES_KOTORBASE_MINIGAMEDESC_H
#define ENGINES_KOTORBASE_MINIGAMEDESC_H

#include <array>
#include <vector>

#include "external/glm/vec3.hpp"

#include "src/common/ustring.h"

namespace Aurora {
	class GFF3Struct;
}

namespace Engines {

namespace KotORBase {

/** The kind of minigame an area runs (ARE MiniGame "Type"). */
enum MinigameType {
	kMinigameTypeNone   = 0,
	kMinigameTypeSwoop  = 1, ///< A swoop race: the player translates along a track.
	kMinigameTypeTurret = 2  ///< A turret: the player rotates in place.
};

/** The script events of minigame objects (labels in their "Scripts" struct). */
enum MinigameScript {
	kMinigameScriptOnAccelerate = 0,
	kMinigameScriptOnAnimEvent,
	kMinigameScriptOnBrake,
	kMinigameScriptOnCreate,
	kMinigameScriptOnDamage,
	kMinigameScriptOnDeath,
	kMinigameScriptOnFire,
	kMinigameScriptOnHeartbeat,
	kMinigameScriptOnHitBullet,
	kMinigameScriptOnHitFollower,
	kMinigameScriptOnHitObstacle,
	kMinigameScriptOnHitWorld,
	kMinigameScriptOnTrackLoop,
	kMinigameScriptMAX
};

/** The GFF label of a minigame script event. */
const char *getMinigameScriptLabel(MinigameScript script);

typedef std::array<Common::UString, kMinigameScriptMAX> MinigameScripts;

/** A gun bank's bullet ("Bullet" struct). */
struct MinigameBulletDesc {
	Common::UString model;          ///< Bullet_Model
	Common::UString collisionSound; ///< Collision_Sound
	int   damage { 0 };             ///< Damage
	float lifespan { 0.0f };        ///< Lifespan, in seconds
	float rateOfFire { 0.0f };      ///< Rate_Of_Fire: seconds between two shots
	float speed { 0.0f };           ///< Speed, in units per second
	uint32_t targetType { 0 };      ///< Target_Type
};

/** A gun bank of the player or an enemy ("Gun_Banks" list). */
struct MinigameGunBankDesc {
	uint32_t bankID { 0 };          ///< BankID: the gun hangs off the "gunbank<ID>" node
	Common::UString fireSound;      ///< Fire_Sound
	Common::UString gunModel;       ///< Gun_Model
	float horizontalSpread { 0.0f }; ///< Horiz_Spread
	float verticalSpread { 0.0f };   ///< Vert_Spread
	float sensingRadius { 0.0f };    ///< Sensing_Radius
	float inaccuracy { 0.0f };       ///< Inaccuracy
	MinigameBulletDesc bullet;
};

/** A model of the player or an enemy ("Models" list). */
struct MinigameModelDesc {
	Common::UString model;  ///< Model
	bool rotating { false }; ///< RotatingModel: turns with the player's input
};

/** A track follower: the player ("Player") or an enemy ("Enemies" list). */
struct MinigameFollowerDesc {
	float accelSecs { 0.0f };       ///< Accel_Secs: speed gained per second
	int   bumpDamage { 0 };         ///< Bump_Damage
	Common::UString camera;         ///< Camera: model carrying the "camerahook"
	bool  cameraRotate { false };   ///< CameraRotate
	int   hitPoints { 0 };          ///< Hit_Points
	int   maxHitPoints { 0 };       ///< Max_HPs
	float invincePeriod { 0.0f };   ///< Invince_Period, in seconds
	float maxSpeed { 0.0f };        ///< Maximum_Speed
	float minSpeed { 0.0f };        ///< Minimum_Speed
	int   numLoops { 0 };           ///< Num_Loops
	float sphereRadius { 0.0f };    ///< Sphere_Radius
	Common::UString track;          ///< Track: the LYT track this follower rides on
	bool  trigger { false };        ///< Trigger (enemies only)

	glm::vec3 tunnelNeg { 0.0f };      ///< TunnelXNeg, TunnelYNeg, TunnelZNeg
	glm::vec3 tunnelPos { 0.0f };      ///< TunnelXPos, TunnelYPos, TunnelZPos
	glm::vec3 tunnelInfinite { 0.0f }; ///< TunnelInfinite: non-zero axes are unbounded
	glm::vec3 startOffset { 0.0f };    ///< Start_Offset_X/Y/Z
	glm::vec3 targetOffset { 0.0f };   ///< Target_Offset_X/Y/Z

	std::vector<MinigameModelDesc> models;
	std::vector<MinigameGunBankDesc> gunBanks;

	MinigameScripts scripts;

	Common::UString deathSound;  ///< Sounds/Death
	Common::UString engineSound; ///< Sounds/Engine
};

/** An obstacle ("Obstacles" list), matched to a LYT obstacle by name. */
struct MinigameObstacleDesc {
	Common::UString name; ///< Name
	MinigameScripts scripts;
};

/** The ARE "MiniGame" struct. */
struct MinigameDesc {
	MinigameType type { kMinigameTypeNone }; ///< Type

	uint32_t bumpPlane { 0 };      ///< Bump_Plane
	float cameraViewAngle { 0.0f }; ///< CameraViewAngle: vertical field of view, degrees
	uint32_t dof { 0 };            ///< DOF
	bool doBumping { false };      ///< DoBumping
	float nearClip { 0.0f };       ///< Near_Clip
	float farClip { 0.0f };        ///< Far_Clip
	float lateralAccel { 0.0f };   ///< LateralAccel
	float movementPerSec { 0.0f }; ///< MovementPerSec
	uint32_t music { 0 };          ///< Music
	bool useInertia { false };     ///< UseInertia

	int  mouseAxisX { 0 };         ///< Mouse/AxisX
	int  mouseAxisY { 0 };         ///< Mouse/AxisY
	bool mouseFlipX { false };     ///< Mouse/FlipAxisX
	bool mouseFlipY { false };     ///< Mouse/FlipAxisY

	MinigameFollowerDesc player;
	std::vector<MinigameFollowerDesc> enemies;
	std::vector<MinigameObstacleDesc> obstacles;

	/** Read an ARE "MiniGame" struct. Missing fields keep their defaults. */
	static MinigameDesc parse(const Aurora::GFF3Struct &miniGame);

	/** Read one track follower (the player or an enemy). */
	static MinigameFollowerDesc parseFollower(const Aurora::GFF3Struct &follower);
};

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_MINIGAMEDESC_H
