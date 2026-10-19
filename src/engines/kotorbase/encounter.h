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
 *  Encounter within an area in KotOR games.
 */

#ifndef ENGINES_KOTORBASE_ENCOUNTER_H
#define ENGINES_KOTORBASE_ENCOUNTER_H

#include <set>
#include <vector>

#include "src/engines/aurora/trigger.h"

#include "src/engines/kotorbase/object.h"
#include "src/engines/kotorbase/encounterrules.h"

namespace Engines {

namespace KotORBase {

class Area;
class Module;
class Creature;

/** A polygon that spawns creatures when the party walks into it (see
 *  EncounterRules for the rules). Only the party fires encounters. */
class Encounter : public ::Engines::Trigger, public Object {
public:
	/** Load an encounter from its GIT instance (and its UTE blueprint). */
	Encounter(const Aurora::GFF3Struct &gff);

	// Basic visuals

	bool isVisible() const;

	void show();
	void hide();
	void notifyNotSeen();

	// Rules

	const EncounterRules &getRules() const { return _rules; }

	bool isActive() const;
	void setActive(bool active);

	/** How often the encounter may re-arm (-1: always). */
	int getSpawnsMax() const;
	void setSpawnsMax(int spawns);

	/** How often the encounter fired so far. */
	int getSpawnsCurrent() const;
	void setSpawnsCurrent(int spawns);

	int getDifficultyIndex() const;
	void setDifficultyIndex(int index);

	// Spawned creatures

	/** Did this encounter spawn the creature (and does it still count it)? */
	bool ownsCreature(const Object &creature) const;
	/** Count a creature as one of this encounter's (restored with the area). */
	void adoptCreature(const Object &creature);

	/** Evaluate the party against the encounter, spawn and run its scripts. */
	void update(Area &area, Module &module);

	// Persistence

	void saveState(Aurora::GFF3WriterStruct &gff) const;
	void loadState(const Aurora::GFF3Struct &gff);

private:
	EncounterRules _rules;
	std::vector<EncounterSpawnPoint> _spawnPoints;

	std::set<uint32_t> _inside;     ///< Party members inside the polygon (object IDs).
	std::vector<uint32_t> _spawned; ///< Creatures this encounter spawned (object IDs).

	double _nextReplacement { 0.0 }; ///< Play time continuous spawns may replace creatures again.
	double _nextHeartbeat { -1.0 };  ///< Play time OnHeartbeat runs next.

	void load(const Aurora::GFF3Struct &gff);

	/** Forget dead and vanished creatures; return those still alive. */
	std::vector<Creature *> getLivingSpawns();

	void spawn(Area &area, Module &module, const Creature &triggerer, int count);
};

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_ENCOUNTER_H
