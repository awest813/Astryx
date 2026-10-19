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
 *  Encounter spawn rules in KotOR games, independent of the game world.
 */

#include <algorithm>
#include <cmath>

#include "src/common/util.h"
#include "src/common/maths.h"

#include "src/aurora/gff3file.h"
#include "src/aurora/gff3writer.h"

#include "src/engines/kotorbase/encounterrules.h"

namespace Engines {

namespace KotORBase {

const float EncounterRules::kHeartbeatInterval = 6.0f;

/** How far apart creatures sharing a spawn location stand. */
static const float kSpawnSpread = 1.0f;
/** How far spawns around the polygon are pulled in from its corners. */
static const float kCornerInset = 1.5f;

void EncounterRules::load(const Aurora::GFF3Struct &gff) {
	active          = gff.getBool("Active", active);
	difficultyIndex = static_cast<int>(gff.getSint("DifficultyIndex", difficultyIndex));
	maxCreatures    = static_cast<int>(gff.getSint("MaxCreatures", maxCreatures));
	recCreatures    = static_cast<int>(gff.getSint("RecCreatures", recCreatures));
	playerOnly      = gff.getBool("PlayerOnly", playerOnly);
	reset           = gff.getBool("Reset", reset);
	resetTime       = static_cast<int>(gff.getSint("ResetTime", resetTime));
	respawns        = static_cast<int>(gff.getSint("Respawns", respawns));
	spawnOption     = static_cast<int>(gff.getSint("SpawnOption", spawnOption));

	if (gff.hasField("Faction")) {
		// 0xFFFFFFFF (-1) stands for no faction; treat it like faction 0.
		const uint64_t value = gff.getUint("Faction");
		faction = (value == 0xFFFFFFFFULL) ? 0 : static_cast<uint32_t>(value);
	}

	if (gff.hasField("CreatureList")) {
		creatures.clear();

		for (const Aurora::GFF3Struct *entry : gff.getList("CreatureList")) {
			if (!entry)
				continue;

			EncounterCreatureEntry creature;
			creature.resRef      = entry->getString("ResRef");
			creature.cr          = static_cast<float>(entry->getDouble("CR"));
			creature.singleSpawn = entry->getBool("SingleSpawn");

			if (!creature.resRef.empty())
				creatures.push_back(creature);
		}
	}
}

void EncounterRules::saveState(Aurora::GFF3WriterStruct &gff) const {
	gff.addByte("Active", active ? 1 : 0);
	gff.addByte("Exhausted", exhausted ? 1 : 0);
	gff.addByte("ExhaustReported", exhaustReported ? 1 : 0);
	gff.addSint32("NumberSpawned", numberSpawned);
	gff.addSint32("Respawns", respawns);
	gff.addSint32("DifficultyIndex", difficultyIndex);
	gff.addDouble("ResetStart", resetStart);
}

void EncounterRules::loadState(const Aurora::GFF3Struct &gff) {
	active          = gff.getBool("Active", active);
	exhausted       = gff.getBool("Exhausted", exhausted);
	exhaustReported = gff.getBool("ExhaustReported", exhaustReported);
	numberSpawned   = static_cast<int>(gff.getSint("NumberSpawned", numberSpawned));
	respawns        = static_cast<int>(gff.getSint("Respawns", respawns));
	difficultyIndex = static_cast<int>(gff.getSint("DifficultyIndex", difficultyIndex));
	resetStart      = gff.getDouble("ResetStart", resetStart);

	// Nobody is inside a freshly loaded encounter.
	cycleOpen = false;
}

bool EncounterRules::canTrigger(bool playerOnly, bool isPlayer, bool hostile) {
	return hostile && (isPlayer || !playerOnly);
}

bool EncounterRules::canTrigger(bool isPlayer, bool hostile) const {
	return canTrigger(playerOnly, isPlayer, hostile);
}

bool EncounterRules::fire() {
	if (!active || exhausted)
		return false;

	exhausted       = true;
	exhaustReported = false;
	resetStart      = -1.0;
	cycleOpen       = spawnOption == kSpawnContinuous;

	numberSpawned++;

	return true;
}

int EncounterRules::getSpawnCount(int alive) const {
	const int recommended = MAX(recCreatures, 1);
	const int maximum     = (maxCreatures > 0) ? maxCreatures : recommended;

	return MAX(MIN(recommended, maximum) - MAX(alive, 0), 0);
}

bool EncounterRules::wantsReplacements(bool hostileInside, int alive) const {
	return active && cycleOpen && hostileInside && (getSpawnCount(alive) > 0);
}

bool EncounterRules::canReset() const {
	return reset && ((respawns < 0) || (numberSpawned <= respawns));
}

int EncounterRules::update(double now, bool hostileInside, int alive) {
	int events = kEventNone;

	// A continuous encounter stops replacing its creatures once everyone hostile left.
	if (cycleOpen && !hostileInside)
		cycleOpen = false;

	if (exhausted && !cycleOpen && (alive <= 0) && !exhaustReported) {
		exhaustReported = true;
		events |= kEventExhausted;

		if (canReset())
			resetStart = now;
	}

	if (exhausted && exhaustReported && (resetStart >= 0.0) && ((now - resetStart) >= resetTime)) {
		exhausted       = false;
		exhaustReported = false;
		resetStart      = -1.0;
		events |= kEventReset;
	}

	return events;
}

std::vector<size_t> EncounterRules::selectCreatures(int count, const std::vector<Common::UString> &aliveResRefs,
                                                    const std::function<size_t(size_t)> &random) const {

	std::vector<size_t> picked;
	std::vector<Common::UString> taken = aliveResRefs;

	for (int i = 0; i < count; i++) {
		std::vector<size_t> candidates;
		for (size_t j = 0; j < creatures.size(); j++) {
			const EncounterCreatureEntry &entry = creatures[j];
			if (entry.singleSpawn && std::any_of(taken.begin(), taken.end(),
					[&entry](const Common::UString &resRef) { return resRef.equalsIgnoreCase(entry.resRef); }))
				continue;

			candidates.push_back(j);
		}

		if (candidates.empty())
			break;

		const size_t choice = candidates[MIN<size_t>(random(candidates.size()), candidates.size() - 1)];

		picked.push_back(choice);
		taken.push_back(creatures[choice].resRef);
	}

	return picked;
}

std::vector<glm::vec3> readEncounterGeometry(const Aurora::GFF3Struct &instance) {
	const glm::vec3 origin(instance.getDouble("XPosition"),
	                       instance.getDouble("YPosition"),
	                       instance.getDouble("ZPosition"));

	std::vector<glm::vec3> geometry;
	if (!instance.hasField("Geometry"))
		return geometry;

	for (const Aurora::GFF3Struct *point : instance.getList("Geometry")) {
		if (!point)
			continue;

		geometry.push_back(origin + glm::vec3(point->getDouble("X"),
		                                      point->getDouble("Y"),
		                                      point->getDouble("Z")));
	}

	return geometry;
}

std::vector<EncounterSpawnPoint> readEncounterSpawnPoints(const Aurora::GFF3Struct &instance) {
	std::vector<EncounterSpawnPoint> points;
	if (!instance.hasField("SpawnPointList"))
		return points;

	for (const Aurora::GFF3Struct *entry : instance.getList("SpawnPointList")) {
		if (!entry)
			continue;

		EncounterSpawnPoint point;
		point.position    = glm::vec3(entry->getDouble("X"), entry->getDouble("Y"), entry->getDouble("Z"));
		point.orientation = static_cast<float>(entry->getDouble("Orientation"));

		points.push_back(point);
	}

	return points;
}

/** Spread out the nth creature sharing a location (n = 0 stays put). */
static glm::vec3 spreadOut(const glm::vec3 &position, size_t n) {
	if (n == 0)
		return position;

	const float angle = static_cast<float>(n - 1) * (static_cast<float>(M_PI) / 3.0f);
	return position + glm::vec3(std::cos(angle) * kSpawnSpread, std::sin(angle) * kSpawnSpread, 0.0f);
}

static float facingTowards(const glm::vec3 &from, const glm::vec3 &to) {
	// Same convention as Object::makeLookAt().
	return Common::rad2deg(std::atan2(to.y - from.y, to.x - from.x)) - 90.0f;
}

static float distance2D(const glm::vec3 &a, const glm::vec3 &b) {
	return std::sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
}

std::vector<EncounterSpawnLocation> computeEncounterSpawnLocations(
		const std::vector<EncounterSpawnPoint> &spawnPoints, const std::vector<glm::vec3> &polygon,
		const glm::vec3 &triggerer, size_t count, size_t firstPoint) {

	std::vector<EncounterSpawnLocation> locations;
	locations.reserve(count);

	if (!spawnPoints.empty()) {
		std::vector<size_t> uses(spawnPoints.size(), 0);

		for (size_t i = 0; i < count; i++) {
			const size_t index = (firstPoint + i) % spawnPoints.size();
			const EncounterSpawnPoint &point = spawnPoints[index];

			EncounterSpawnLocation location;
			location.position       = spreadOut(point.position, uses[index]++);
			location.facing         = Common::rad2deg(point.orientation);
			location.fromSpawnPoint = true;

			locations.push_back(location);
		}

		return locations;
	}

	// No spawn points: the original spawns out of sight of whoever fired the
	// encounter. Use the corners of the polygon farthest away, pulled inwards.
	std::vector<glm::vec3> candidates;
	if (!polygon.empty()) {
		glm::vec3 centroid(0.0f, 0.0f, 0.0f);
		for (const glm::vec3 &corner : polygon)
			centroid += corner;
		centroid /= static_cast<float>(polygon.size());

		for (const glm::vec3 &corner : polygon) {
			const float length = distance2D(corner, centroid);
			const float inset  = MIN(kCornerInset, length * 0.5f);

			glm::vec3 position = corner;
			if (length > 0.0f) {
				position.x += (centroid.x - corner.x) / length * inset;
				position.y += (centroid.y - corner.y) / length * inset;
			}

			candidates.push_back(position);
		}

		std::stable_sort(candidates.begin(), candidates.end(), [&triggerer](const glm::vec3 &a, const glm::vec3 &b) {
			return distance2D(a, triggerer) > distance2D(b, triggerer);
		});
	}

	// Without a polygon, stand around (not on) the creature that fired it.
	std::vector<size_t> uses(candidates.size(), 0);
	if (candidates.empty()) {
		candidates.push_back(triggerer);
		uses.push_back(1);
	}

	for (size_t i = 0; i < count; i++) {
		const size_t index = i % candidates.size();

		EncounterSpawnLocation location;
		location.position = spreadOut(candidates[index], uses[index]++);
		location.facing   = facingTowards(location.position, triggerer);

		locations.push_back(location);
	}

	return locations;
}

} // End of namespace KotORBase

} // End of namespace Engines
