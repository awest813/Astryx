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
 *  Trigger within an area in KotOR games.
 */

#ifndef ENGINES_KOTORBASE_TRIGGER_H
#define ENGINES_KOTORBASE_TRIGGER_H

#include <memory>

#include "src/engines/aurora/trigger.h"

#include "src/engines/kotorbase/object.h"
#include "src/engines/kotorbase/trap.h"
#include "src/engines/kotorbase/triggeroccupancy.h"

namespace Graphics {
	namespace Aurora {
		class Model;
	}
}

namespace Engines {

namespace KotORBase {

class Trigger : public ::Engines::Trigger, public Object {
public:
	/** The trigger kinds of the UTT "Type" field. */
	enum Kind {
		kKindGeneric    = 0,
		kKindTransition = 1,
		kKindTrap       = 2
	};

	Trigger(const Aurora::GFF3Struct &gff);
	~Trigger();

	// The script object's ID and tag, not the debug polygon's.
	using Object::getID;
	using Object::getTag;

	// Basic visuals

	bool isVisible() const;

	void show();
	void hide();
	void notifyNotSeen();

	// Area transitions

	/** Does walking into this trigger move the party somewhere else? */
	bool isTransition() const;
	const Common::UString &getLinkedTo() const;
	const Common::UString &getLinkedToModule() const;

	// Occupants

	/** The objects inside the trigger (by ID). */
	TriggerOccupancy &getOccupancy();
	const TriggerOccupancy &getOccupancy() const;

	/** The object that last entered/exited the trigger (GetEnteringObject/GetExitingObject). */
	uint32_t getLastEnteredID() const;
	uint32_t getLastExitedID() const;
	void setLastEnteredID(uint32_t id);
	void setLastExitedID(uint32_t id);

	// Traps (mines)

	/** Is this a trap trigger (a mine) that is still there? */
	bool isTrap() const;
	/** Is the mine armed? */
	bool isTrapActive() const;
	/** Has anyone (in practice: the party) spotted the mine? */
	bool isTrapDetected() const;
	/** Can the mine be disarmed (and recovered)? */
	bool isTrapDisarmable() const;

	Trap &getTrap();
	const Trap &getTrap() const;

	/** Mark the mine as spotted by this creature, and show its model. */
	void detectTrap(Object *detector);
	/** The mine goes off for @p triggerer. Returns false if it was not armed. */
	bool fireTrap(Object *triggerer);
	/** The mine is gone (disarmed or recovered): disarm it and hide its model. */
	void disableTrap();

	// Persistence

	void saveState(Aurora::GFF3WriterStruct &gff) const override;
	void loadState(const Aurora::GFF3Struct &gff) override;

protected:
	void load(const Aurora::GFF3Struct &gff);
	void loadBlueprint(const Aurora::GFF3Struct &gff);
	void loadLinkedTo(const Aurora::GFF3Struct &gff);
	void loadTrap(const Aurora::GFF3Struct &gff);

private:
	Common::UString _linkedTo;       ///< Tag of the waypoint/door this trigger leads to.
	Common::UString _linkedToModule; ///< Module of that object (empty: this module).

	uint32_t _kind { kKindGeneric };

	TriggerOccupancy _occupancy;
	uint32_t _lastEnteredID { 0 };
	uint32_t _lastExitedID { 0 };

	Trap _trap;
	std::unique_ptr<Graphics::Aurora::Model> _trapModel; ///< Shown once the mine is spotted.
	bool _shown { false }; ///< Is the trigger (and so its mine model) shown?

	void loadTrapInfo();
	void updateTrapModel();
};

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_TRIGGER_H
