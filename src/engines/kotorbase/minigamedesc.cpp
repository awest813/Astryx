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
 */

#include "src/aurora/gff3file.h"

#include "src/engines/kotorbase/minigamedesc.h"

namespace Engines {

namespace KotORBase {

namespace {

const char *const kScriptLabels[kMinigameScriptMAX] = {
	"OnAccelerate", "OnAnimEvent", "OnBrake", "OnCreate", "OnDamage", "OnDeath", "OnFire",
	"OnHeartbeat", "OnHitBullet", "OnHitFollower", "OnHitObstacle", "OnHitWorld", "OnTrackLoop"
};

/** The first of these labels the struct has, or "" if none. */
Common::UString findLabel(const Aurora::GFF3Struct &gff, std::initializer_list<const char *> labels) {
	for (const char *label : labels)
		if (gff.hasField(label))
			return label;

	return "";
}

/** Read a number of any GFF numeric type as a float. */
float readFloat(const Aurora::GFF3Struct &gff, std::initializer_list<const char *> labels, float def = 0.0f) {
	const Common::UString label = findLabel(gff, labels);
	if (label.empty())
		return def;

	switch (gff.getFieldType(label)) {
		case Aurora::GFF3Struct::kFieldTypeFloat:
		case Aurora::GFF3Struct::kFieldTypeDouble:
			return static_cast<float>(gff.getDouble(label, def));

		case Aurora::GFF3Struct::kFieldTypeByte:
		case Aurora::GFF3Struct::kFieldTypeUint16:
		case Aurora::GFF3Struct::kFieldTypeUint32:
		case Aurora::GFF3Struct::kFieldTypeUint64:
			return static_cast<float>(gff.getUint(label));

		case Aurora::GFF3Struct::kFieldTypeChar:
		case Aurora::GFF3Struct::kFieldTypeSint16:
		case Aurora::GFF3Struct::kFieldTypeSint32:
		case Aurora::GFF3Struct::kFieldTypeSint64:
			return static_cast<float>(gff.getSint(label));

		default:
			return def;
	}
}

float readFloat(const Aurora::GFF3Struct &gff, const char *label, float def = 0.0f) {
	return readFloat(gff, { label }, def);
}

/** Read a number of any GFF numeric type as an integer. */
int readInt(const Aurora::GFF3Struct &gff, std::initializer_list<const char *> labels, int def = 0) {
	const Common::UString label = findLabel(gff, labels);
	if (label.empty())
		return def;

	switch (gff.getFieldType(label)) {
		case Aurora::GFF3Struct::kFieldTypeFloat:
		case Aurora::GFF3Struct::kFieldTypeDouble:
			return static_cast<int>(gff.getDouble(label, def));

		case Aurora::GFF3Struct::kFieldTypeByte:
		case Aurora::GFF3Struct::kFieldTypeUint16:
		case Aurora::GFF3Struct::kFieldTypeUint32:
		case Aurora::GFF3Struct::kFieldTypeUint64:
			return static_cast<int>(gff.getUint(label));

		case Aurora::GFF3Struct::kFieldTypeChar:
		case Aurora::GFF3Struct::kFieldTypeSint16:
		case Aurora::GFF3Struct::kFieldTypeSint32:
		case Aurora::GFF3Struct::kFieldTypeSint64:
			return static_cast<int>(gff.getSint(label));

		default:
			return def;
	}
}

int readInt(const Aurora::GFF3Struct &gff, const char *label, int def = 0) {
	return readInt(gff, { label }, def);
}

Common::UString readString(const Aurora::GFF3Struct &gff, const char *label) {
	switch (gff.getFieldType(label)) {
		case Aurora::GFF3Struct::kFieldTypeExoString:
		case Aurora::GFF3Struct::kFieldTypeResRef:
			return gff.getString(label);

		default:
			return "";
	}
}

/** A vector field, or a non-vector number as (n, n, n). */
glm::vec3 readVector(const Aurora::GFF3Struct &gff, std::initializer_list<const char *> labels) {
	const Common::UString label = findLabel(gff, labels);
	if (label.empty())
		return glm::vec3(0.0f);

	if (gff.getFieldType(label) == Aurora::GFF3Struct::kFieldTypeVector) {
		float x = 0.0f, y = 0.0f, z = 0.0f;
		gff.getVector(label, x, y, z);
		return glm::vec3(x, y, z);
	}

	return glm::vec3(readFloat(gff, labels));
}

/** The structs of a field that is either a struct or a list of structs. */
std::vector<const Aurora::GFF3Struct *> readStructs(const Aurora::GFF3Struct &gff, const char *label) {
	std::vector<const Aurora::GFF3Struct *> structs;

	switch (gff.getFieldType(label)) {
		case Aurora::GFF3Struct::kFieldTypeStruct:
			structs.push_back(&gff.getStruct(label));
			break;

		case Aurora::GFF3Struct::kFieldTypeList:
			for (const Aurora::GFF3Struct *s : gff.getList(label))
				if (s)
					structs.push_back(s);
			break;

		default:
			break;
	}

	return structs;
}

const Aurora::GFF3Struct *readFirstStruct(const Aurora::GFF3Struct &gff, const char *label) {
	const std::vector<const Aurora::GFF3Struct *> structs = readStructs(gff, label);
	return structs.empty() ? nullptr : structs.front();
}

void readScripts(const Aurora::GFF3Struct &gff, MinigameScripts &scripts) {
	const Aurora::GFF3Struct *node = readFirstStruct(gff, "Scripts");
	if (!node)
		return;

	for (size_t i = 0; i < kMinigameScriptMAX; ++i)
		scripts[i] = readString(*node, kScriptLabels[i]);
}

MinigameBulletDesc parseBullet(const Aurora::GFF3Struct &gff) {
	MinigameBulletDesc bullet;

	bullet.model          = readString(gff, "Bullet_Model");
	bullet.collisionSound = readString(gff, "Collision_Sound");
	bullet.damage         = readInt  (gff, "Damage");
	bullet.lifespan       = readFloat(gff, "Lifespan");
	bullet.rateOfFire     = readFloat(gff, "Rate_Of_Fire");
	bullet.speed          = readFloat(gff, "Speed");
	bullet.targetType     = static_cast<uint32_t>(readInt(gff, "Target_Type"));

	return bullet;
}

MinigameGunBankDesc parseGunBank(const Aurora::GFF3Struct &gff) {
	MinigameGunBankDesc bank;

	bank.bankID           = static_cast<uint32_t>(readInt(gff, "BankID"));
	bank.fireSound        = readString(gff, "Fire_Sound");
	bank.gunModel         = readString(gff, "Gun_Model");
	bank.horizontalSpread = readFloat(gff, "Horiz_Spread");
	bank.verticalSpread   = readFloat(gff, "Vert_Spread");
	bank.sensingRadius    = readFloat(gff, "Sensing_Radius");
	bank.inaccuracy       = readFloat(gff, "Inaccuracy");

	if (const Aurora::GFF3Struct *bullet = readFirstStruct(gff, "Bullet"))
		bank.bullet = parseBullet(*bullet);

	return bank;
}

} // End of anonymous namespace

const char *getMinigameScriptLabel(MinigameScript script) {
	if (script < 0 || script >= kMinigameScriptMAX)
		return "";

	return kScriptLabels[script];
}

MinigameFollowerDesc MinigameDesc::parseFollower(const Aurora::GFF3Struct &gff) {
	MinigameFollowerDesc follower;

	follower.accelSecs     = readFloat(gff, "Accel_Secs");
	follower.bumpDamage    = readInt  (gff, "Bump_Damage");
	follower.camera        = readString(gff, "Camera");
	follower.cameraRotate  = readInt  (gff, "CameraRotate") != 0;
	follower.hitPoints     = readInt  (gff, "Hit_Points");
	follower.maxHitPoints  = readInt  (gff, "Max_HPs");
	follower.invincePeriod = readFloat(gff, "Invince_Period");
	follower.maxSpeed      = readFloat(gff, "Maximum_Speed");
	follower.minSpeed      = readFloat(gff, "Minimum_Speed");
	follower.numLoops      = readInt  (gff, "Num_Loops");
	follower.sphereRadius  = readFloat(gff, "Sphere_Radius");
	follower.track         = readString(gff, "Track");
	follower.trigger       = readInt  (gff, "Trigger") != 0;

	// Some retail files carry these labels misspelled.
	follower.tunnelNeg = glm::vec3(readFloat(gff, { "TunnelXNeg" }),
	                               readFloat(gff, { "TunnelYNeg" }),
	                               readFloat(gff, { "TunnelZNeg", "TunneZNeg" }));
	follower.tunnelPos = glm::vec3(readFloat(gff, { "TunnelXPos", "TunneXPos" }),
	                               readFloat(gff, { "TunnelYPos" }),
	                               readFloat(gff, { "TunnelZPos" }));
	follower.tunnelInfinite = readVector(gff, { "TunnelInfinite", "Tunnellnfinite" });

	follower.startOffset  = glm::vec3(readFloat(gff, "Start_Offset_X"),
	                                  readFloat(gff, "Start_Offset_Y"),
	                                  readFloat(gff, "Start_Offset_Z"));
	follower.targetOffset = glm::vec3(readFloat(gff, "Target_Offset_X"),
	                                  readFloat(gff, "Target_Offset_Y"),
	                                  readFloat(gff, "Target_Offset_Z"));

	for (const Aurora::GFF3Struct *model : readStructs(gff, "Models")) {
		MinigameModelDesc desc;
		desc.model    = readString(*model, "Model");
		desc.rotating = readInt(*model, "RotatingModel") != 0;

		if (!desc.model.empty())
			follower.models.push_back(desc);
	}

	for (const Aurora::GFF3Struct *bank : readStructs(gff, "Gun_Banks"))
		follower.gunBanks.push_back(parseGunBank(*bank));

	readScripts(gff, follower.scripts);

	if (const Aurora::GFF3Struct *sounds = readFirstStruct(gff, "Sounds")) {
		follower.deathSound  = readString(*sounds, "Death");
		follower.engineSound = readString(*sounds, "Engine");
	}

	return follower;
}

MinigameDesc MinigameDesc::parse(const Aurora::GFF3Struct &gff) {
	MinigameDesc desc;

	const int type = readInt(gff, "Type");
	if ((type == kMinigameTypeSwoop) || (type == kMinigameTypeTurret))
		desc.type = static_cast<MinigameType>(type);

	desc.bumpPlane       = static_cast<uint32_t>(readInt(gff, "Bump_Plane"));
	desc.cameraViewAngle = readFloat(gff, "CameraViewAngle");
	desc.dof             = static_cast<uint32_t>(readInt(gff, "DOF"));
	desc.doBumping       = readInt  (gff, "DoBumping") != 0;
	desc.nearClip        = readFloat(gff, "Near_Clip");
	desc.farClip         = readFloat(gff, "Far_Clip");
	desc.lateralAccel    = readFloat(gff, "LateralAccel");
	desc.movementPerSec  = readFloat(gff, "MovementPerSec");
	desc.music           = static_cast<uint32_t>(readInt(gff, "Music"));
	desc.useInertia      = readInt(gff, { "UseInertia", "Uselnertia" }) != 0;

	if (const Aurora::GFF3Struct *mouse = readFirstStruct(gff, "Mouse")) {
		desc.mouseAxisX = readInt(*mouse, "AxisX");
		desc.mouseAxisY = readInt(*mouse, "AxisY");
		desc.mouseFlipX = readInt(*mouse, "FlipAxisX") != 0;
		desc.mouseFlipY = readInt(*mouse, "FlipAxisY") != 0;
	}

	if (const Aurora::GFF3Struct *player = readFirstStruct(gff, "Player"))
		desc.player = parseFollower(*player);

	for (const Aurora::GFF3Struct *enemy : readStructs(gff, "Enemies"))
		desc.enemies.push_back(parseFollower(*enemy));

	for (const Aurora::GFF3Struct *obstacle : readStructs(gff, "Obstacles")) {
		MinigameObstacleDesc obstacleDesc;
		obstacleDesc.name = readString(*obstacle, "Name");
		readScripts(*obstacle, obstacleDesc.scripts);

		desc.obstacles.push_back(obstacleDesc);
	}

	return desc;
}

} // End of namespace KotORBase

} // End of namespace Engines
