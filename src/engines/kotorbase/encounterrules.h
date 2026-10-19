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
 *
 *  Encounters (UTE blueprints, "Encounter List" in the GIT) follow the Aurora
 *  encounter rules of BioWare's "Encounter Format" document:
 *
 *  - An Active encounter fires when a creature hostile to its Faction (only a
 *    player character if PlayerOnly) enters its polygon.
 *  - It then spawns RecCreatures creatures (never more than MaxCreatures of
 *    its own alive at a time) out of CreatureList; a SingleSpawn entry is
 *    never alive twice at the same time.
 *  - SpawnOption 1 (single shot) fires once. SpawnOption 0 (continuous) also
 *    replaces fallen creatures while a hostile creature stays inside.
 *  - Once fired, the encounter is exhausted. When all its creatures are gone,
 *    OnExhausted runs; with Reset, it re-arms ResetTime seconds later, at most
 *    Respawns times (-1: always).
 *
 *  Simplifications: the creature level budget (encdifficulty.2da and the
 *  creatures' CR) is not evaluated; the encounter spawns RecCreatures
 *  creatures, clamped to MaxCreatures, picked at random.
 */

#ifndef ENGINES_KOTORBASE_ENCOUNTERRULES_H
#define ENGINES_KOTORBASE_ENCOUNTERRULES_H

#include <cstddef>
#include <functional>
#include <vector>

#include "external/glm/vec3.hpp"

#include "src/common/types.h"
#include "src/common/ustring.h"

namespace Aurora {
	class GFF3Struct;
	class GFF3WriterStruct;
}

namespace Engines {

namespace KotORBase {

/** A creature an encounter can spawn (EncounterCreature struct). */
struct EncounterCreatureEntry {
	Common::UString resRef;   ///< UTC blueprint to spawn.
	float cr { 0.0f };        ///< Challenge rating (not evaluated).
	bool singleSpawn { false }; ///< Never alive twice at the same time.
};

/** A fixed spawn location (EncounterSpawnPoint struct, area coordinates). */
struct EncounterSpawnPoint {
	glm::vec3 position { 0.0f, 0.0f, 0.0f };
	float orientation { 0.0f }; ///< Bearing in radians, counterclockwise from north.
};

/** Where a single creature is spawned. */
struct EncounterSpawnLocation {
	glm::vec3 position { 0.0f, 0.0f, 0.0f };
	float facing { 0.0f };       ///< Degrees, as Object::setOrientation() takes it.
	bool fromSpawnPoint { false }; ///< From the SpawnPointList (else around the polygon).
};

/** An encounter's properties and spawn state. */
class EncounterRules {
public:
	enum SpawnOption {
		kSpawnContinuous = 0,
		kSpawnSingleShot = 1
	};

	/** Events update() reports. */
	enum Event {
		kEventNone      = 0,
		kEventExhausted = 1 << 0, ///< All creatures of the spawn are gone: run OnExhausted.
		kEventReset     = 1 << 1  ///< The encounter re-armed.
	};

	/** How often a continuous encounter replaces fallen creatures, and OnHeartbeat runs. */
	static const float kHeartbeatInterval;

	// Properties (UTE / GIT instance)

	bool active { true };
	uint32_t faction { 0 };
	int difficultyIndex { 0 };
	int maxCreatures { 1 };
	int recCreatures { 1 };
	bool playerOnly { false };
	bool reset { false };
	int resetTime { 0 };   ///< Seconds.
	int respawns { 0 };    ///< How often the encounter may re-arm; -1: always.
	int spawnOption { kSpawnSingleShot };

	std::vector<EncounterCreatureEntry> creatures;

	// State

	bool exhausted { false };       ///< Fired, not re-armed yet.
	bool cycleOpen { false };       ///< Continuous: still replacing fallen creatures.
	bool exhaustReported { false }; ///< OnExhausted ran for the current spawn.
	int numberSpawned { 0 };        ///< How often the encounter fired.
	double resetStart { -1.0 };     ///< Play time the reset countdown started at (< 0: none).

	/** Read the properties a struct has, keeping the current values of those it lacks.
	 *  Load the blueprint first, then the GIT instance on top of it. */
	void load(const Aurora::GFF3Struct &gff);

	void saveState(Aurora::GFF3WriterStruct &gff) const;
	void loadState(const Aurora::GFF3Struct &gff);

	/** May a creature fire an encounter? */
	static bool canTrigger(bool playerOnly, bool isPlayer, bool hostile);
	bool canTrigger(bool isPlayer, bool hostile) const;

	/** A creature that can trigger the encounter entered it. Returns true if
	 *  the encounter fires now; spawn getSpawnCount() creatures then. */
	bool fire();

	/** How many creatures to spawn now, with that many of ours still alive. */
	int getSpawnCount(int alive) const;

	/** Does a continuous encounter want to replace fallen creatures? */
	bool wantsReplacements(bool hostileInside, int alive) const;

	/** May the encounter re-arm (again)? */
	bool canReset() const;

	/** Bookkeeping, once per frame. Returns a combination of Event flags. */
	int update(double now, bool hostileInside, int alive);

	/** Pick count blueprints (indices into creatures). SingleSpawn entries whose
	 *  creature is alive (aliveResRefs) or already picked are left out, so
	 *  fewer may come back. random(n) returns a value in [0, n). */
	std::vector<size_t> selectCreatures(int count, const std::vector<Common::UString> &aliveResRefs,
	                                    const std::function<size_t(size_t)> &random) const;
};

/** The encounter polygon of a GIT instance, in area coordinates. The points
 *  are relative to the instance's XPosition/YPosition/ZPosition. */
std::vector<glm::vec3> readEncounterGeometry(const Aurora::GFF3Struct &instance);

/** The spawn points of a GIT instance (absolute area coordinates). */
std::vector<EncounterSpawnPoint> readEncounterSpawnPoints(const Aurora::GFF3Struct &instance);

/** Where to spawn count creatures. Spawn points are used in turn, starting at
 *  firstPoint; without any, the creatures appear at the corners of the polygon
 *  farthest from the creature that fired the encounter, facing it. Repeated
 *  locations are spread out a little. */
std::vector<EncounterSpawnLocation> computeEncounterSpawnLocations(
		const std::vector<EncounterSpawnPoint> &spawnPoints, const std::vector<glm::vec3> &polygon,
		const glm::vec3 &triggerer, size_t count, size_t firstPoint);

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_ENCOUNTERRULES_H
