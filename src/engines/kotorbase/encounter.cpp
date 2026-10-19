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

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <memory>

#include "src/common/util.h"
#include "src/common/maths.h"
#include "src/common/error.h"
#include "src/common/random.h"

#include "src/aurora/gff3file.h"
#include "src/aurora/nwscript/objectman.h"

#include "src/engines/aurora/util.h"

#include "src/engines/kotorbase/encounter.h"
#include "src/engines/kotorbase/area.h"
#include "src/engines/kotorbase/module.h"
#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/objectcontainer.h"
#include "src/engines/kotorbase/reputation.h"

namespace Engines {

namespace KotORBase {

Encounter::Encounter(const Aurora::GFF3Struct &gff) :
		Engines::Trigger(),
		Object(kObjectTypeEncounter) {

	load(gff);
	prepare();
}

void Encounter::show() {
	Renderable::show();
}

void Encounter::hide() {
	Renderable::hide();
}

void Encounter::notifyNotSeen() {
}

bool Encounter::isVisible() const {
	return Renderable::isVisible();
}

void Encounter::load(const Aurora::GFF3Struct &gff) {
	_templateResRef = gff.getString("TemplateResRef");

	std::unique_ptr<Aurora::GFF3File> ute;
	if (!_templateResRef.empty())
		ute.reset(loadOptionalGFF3(_templateResRef, Aurora::kFileTypeUTE, MKTAG('U', 'T', 'E', ' ')));

	// A GIT instance carries all the encounter fields itself; the blueprint
	// only fills in what an instance may lack.
	if (ute) {
		const Aurora::GFF3Struct &blueprint = ute->getTopLevel();

		_rules.load(blueprint);
		Object::_tag = blueprint.getString("Tag");
		_name = blueprint.getString("LocalizedName");
		readScripts(blueprint);
	}

	_rules.load(gff);
	Object::_tag = gff.getString("Tag", Object::_tag);
	_name = gff.getString("LocalizedName", _name);

	if (gff.hasField("OnEntered") || !ute)
		readScripts(gff);

	setFaction(static_cast<Faction>(_rules.faction));

	setPosition(gff.getDouble("XPosition"), gff.getDouble("YPosition"), gff.getDouble("ZPosition"));

	_geometry    = readEncounterGeometry(gff);
	_spawnPoints = readEncounterSpawnPoints(gff);
}

bool Encounter::isActive() const {
	return _rules.active;
}

void Encounter::setActive(bool active) {
	_rules.active = active;
}

int Encounter::getSpawnsMax() const {
	return _rules.respawns;
}

void Encounter::setSpawnsMax(int spawns) {
	_rules.respawns = spawns;
}

int Encounter::getSpawnsCurrent() const {
	return _rules.numberSpawned;
}

void Encounter::setSpawnsCurrent(int spawns) {
	_rules.numberSpawned = spawns;
}

int Encounter::getDifficultyIndex() const {
	return _rules.difficultyIndex;
}

void Encounter::setDifficultyIndex(int index) {
	_rules.difficultyIndex = index;
}

bool Encounter::ownsCreature(const Object &creature) const {
	return std::find(_spawned.begin(), _spawned.end(), creature.getID()) != _spawned.end();
}

void Encounter::adoptCreature(const Object &creature) {
	if (!ownsCreature(creature))
		_spawned.push_back(creature.getID());
}

std::vector<Creature *> Encounter::getLivingSpawns() {
	std::vector<Creature *> living;

	for (auto it = _spawned.begin(); it != _spawned.end(); ) {
		Creature *creature = ObjectContainer::toCreature(ObjectMan.findObject(*it));
		if (!creature || creature->isDead()) {
			it = _spawned.erase(it);
			continue;
		}

		living.push_back(creature);
		++it;
	}

	return living;
}

/** The PC and the party, the creatures that can fire encounters. */
static std::vector<Creature *> getParty(Module &module) {
	std::vector<Creature *> party;

	if (Creature *pc = module.getPC())
		party.push_back(pc);

	for (int i = 0; i < module.getPartyMemberCount(); i++) {
		Creature *member = module.getPartyMemberByIndex(i);
		if (member && std::find(party.begin(), party.end(), member) == party.end())
			party.push_back(member);
	}

	return party;
}

void Encounter::update(Area &area, Module &module) {
	const double now = module.getPlayTime();

	std::vector<Creature *> entered, exited;
	std::set<uint32_t> inside;
	Creature *hostileInside = nullptr;

	for (Creature *creature : getParty(module)) {
		if (creature->isDead())
			continue;

		float x, y, z;
		creature->getPosition(x, y, z);

		const bool isInside  = contains(x, y);
		const bool wasInside = _inside.count(creature->getID()) > 0;

		const bool isPlayer   = creature->isPC() || (creature == module.getPartyLeader());
		const bool hostile    = ReputationMan.isHostile(static_cast<int>(_rules.faction),
		                                                static_cast<int>(creature->getFaction()));
		const bool qualifies  = _rules.canTrigger(isPlayer, hostile);

		if (isInside) {
			inside.insert(creature->getID());
			if (qualifies && !hostileInside)
				hostileInside = creature;
		}

		if (!qualifies)
			continue;

		if (isInside && !wasInside)
			entered.push_back(creature);
		else if (!isInside && wasInside)
			exited.push_back(creature);
	}

	_inside.swap(inside);

	for (Creature *creature : exited)
		runScript(kScriptExit, this, creature);

	for (Creature *creature : entered) {
		runScript(kScriptEnter, this, creature);

		// The OnEntered script may have switched the encounter off.
		if (_rules.fire()) {
			spawn(area, module, *creature, _rules.getSpawnCount(static_cast<int>(getLivingSpawns().size())));
			_nextReplacement = now + EncounterRules::kHeartbeatInterval;
		}
	}

	// A continuous encounter replaces fallen creatures while the party stays.
	int alive = static_cast<int>(getLivingSpawns().size());
	if (hostileInside && (now >= _nextReplacement) && _rules.wantsReplacements(true, alive)) {
		spawn(area, module, *hostileInside, _rules.getSpawnCount(alive));
		_nextReplacement = now + EncounterRules::kHeartbeatInterval;

		alive = static_cast<int>(getLivingSpawns().size());
	}

	if (_rules.update(now, hostileInside != nullptr, alive) & EncounterRules::kEventExhausted)
		runScript(kScriptExhausted, this, this);

	if (_nextHeartbeat < 0.0)
		_nextHeartbeat = now + EncounterRules::kHeartbeatInterval;

	if (now >= _nextHeartbeat) {
		_nextHeartbeat = now + EncounterRules::kHeartbeatInterval;
		if (hasScript(kScriptHeartbeat))
			runScript(kScriptHeartbeat, this, this);
	}
}

void Encounter::spawn(Area &area, Module &module, const Creature &triggerer, int count) {
	if (count <= 0)
		return;

	std::vector<Common::UString> aliveResRefs;
	for (Creature *creature : getLivingSpawns())
		aliveResRefs.push_back(creature->getTemplateResRef());

	const std::vector<size_t> picks = _rules.selectCreatures(count, aliveResRefs, [](size_t n) {
		return static_cast<size_t>(RNG.getNext(0, static_cast<int>(n)));
	});
	if (picks.empty())
		return;

	glm::vec3 origin;
	triggerer.getPosition(origin.x, origin.y, origin.z);

	const size_t firstPoint = _spawnPoints.empty() ? 0 :
		static_cast<size_t>(RNG.getNext(0, static_cast<int>(_spawnPoints.size())));

	const std::vector<EncounterSpawnLocation> locations =
		computeEncounterSpawnLocations(_spawnPoints, _geometry, origin, picks.size(), firstPoint);

	for (size_t i = 0; i < picks.size(); i++) {
		const Common::UString &resRef = _rules.creatures[picks[i]].resRef;

		glm::vec3 position = locations[i].position;
		float facing       = locations[i].facing;

		const float ground = area.evaluateElevation(position.x, position.y);
		if (ground != FLT_MIN) {
			position.z = ground;
		} else if (!locations[i].fromSpawnPoint) {
			// Off the walkmesh: next to the creature that fired the encounter instead.
			const float angle = static_cast<float>(i) * static_cast<float>(M_PI) / 3.0f;
			position = origin + glm::vec3(std::cos(angle) * 2.0f, std::sin(angle) * 2.0f, 0.0f);

			const float height = area.evaluateElevation(position.x, position.y);
			if (height != FLT_MIN)
				position.z = height;
			else
				position = origin;

			facing = Common::rad2deg(std::atan2(origin.y - position.y, origin.x - position.x)) - 90.0f;
		}

		Creature *creature = nullptr;
		try {
			creature = module.createCreatureByTemplate(resRef);
		} catch (Common::Exception &e) {
			e.add("Encounter \"%s\" failed to spawn \"%s\"", Object::getTag().c_str(), resRef.c_str());
			printException(e, "WARNING: ");
			continue;
		}

		if (!creature)
			continue;

		// The creature keeps the faction of its own blueprint.
		creature->setPosition(position.x, position.y, position.z);
		creature->setOrientation(0.0f, 0.0f, 1.0f, facing);

		// Like CreateObject: OnSpawn runs, and the area keeps it across visits.
		area.addCreature(creature);

		_spawned.push_back(creature->getID());
	}
}

void Encounter::saveState(Aurora::GFF3WriterStruct &gff) const {
	Object::saveState(gff);
	_rules.saveState(gff);
}

void Encounter::loadState(const Aurora::GFF3Struct &gff) {
	Object::loadState(gff);
	_rules.loadState(gff);
}

} // End of namespace KotORBase

} // End of namespace Engines
