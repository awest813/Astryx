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
 *  Traps (mines on triggers, trapped doors and placeables) in KotOR games,
 *  independent of the game world.
 *
 *  The trap fields are the ones of the UTT/UTD/UTP blueprints and their GIT
 *  instances: TrapFlag, TrapType (a row of traps.2da), TrapDetectable and
 *  TrapDetectDC, TrapDisarmable and DisarmDC, TrapOneShot and KeyName.
 *
 *  The skill checks follow the d20 rules the game is built on:
 *  - Detection: Awareness + roll against TrapDetectDC, for a creature close
 *    to the trap. Out of combat the check is passive (take 10: standing
 *    around does not find a better-hidden mine); in combat it is a d20 roll
 *    each round.
 *  - Disarming: Demolitions + roll against DisarmDC (take 10 out of combat,
 *    as failure has consequences; a d20 in combat). Failing by 5 or more
 *    sets the trap off. Recovering the mine (to get its item back) is
 *    harder: the DC is DisarmDC + kRecoverDCPenalty.
 *  - Locks are picked with Security + roll against the lock DC; failure
 *    costs nothing but time, so out of combat the check takes 20.
 *  - Mines the party sets (mine items, baseitems.2da row 58 as in KotOR.js)
 *    are laid just ahead of the creature setting them, as a small square
 *    trap trigger of its faction: the enemies set them off. Their DCs are
 *    10 + the setter's Demolitions + the traps.2da DC modifiers.
 */

#ifndef ENGINES_KOTORBASE_TRAP_H
#define ENGINES_KOTORBASE_TRAP_H

#include <vector>

#include "external/glm/vec2.hpp"
#include "external/glm/vec3.hpp"

#include "src/common/types.h"
#include "src/common/ustring.h"

namespace Aurora {
	class GFF3Struct;
	class GFF3WriterStruct;
	class TwoDAFile;
}

namespace Engines {

namespace KotORBase {

namespace TrapRules {

/** How close (m) a creature has to be to a trap to have a chance to spot it. */
extern const float kDetectRange;
/** How close (m) a creature walks up to a mine to disarm or recover it. */
extern const float kDisarmRange;
/** Recovering a mine is harder than disarming it by this much. */
extern const int kRecoverDCPenalty;
/** Failing a disarm check by this much or more sets the trap off. */
extern const int kSetOffMargin;

/** Outcome of an attempt to disarm or recover a trap. */
enum DisarmResult {
	kDisarmSucceeded, ///< The trap is gone (and, when recovering, its item is taken).
	kDisarmFailed,    ///< Nothing happened; the trap is still there.
	kDisarmSetOff     ///< The attempt failed badly: the trap goes off.
};

/** The "die roll" of a check: a d20 in combat, @p take outside of it.
 *  @p d20 is the result of a d20 roll, only used in combat. */
int checkRoll(bool inCombat, int take, int d20);

/** Does a creature with this Awareness spot a trap of this DC? */
bool detects(int awareness, int roll, int detectDC);

/** The DC of recovering a trap with this disarm DC. */
int getRecoverDC(int disarmDC);

/** Disarm (or recover) a trap with this Demolitions skill. */
DisarmResult disarm(int demolitions, int roll, int disarmDC, bool recover);

/** Does a creature with this Security skill (plus any tool bonus) open a lock of this DC? */
bool unlocks(int security, int roll, int lockDC);

/** baseitems.2da row of the mines (KotOR.js' mine filter). */
extern const int kBaseItemMine;
/** itempropdef.2da row of the "Trap" item property: its subtype is the traps.2da row. */
extern const int kItemPropertyTrap;
/** Mines are laid this far (m) ahead of the creature setting them. */
extern const float kMinePlaceDistance;
/** Half the side (m) of a laid mine's square trigger. */
extern const float kMineHalfSize;
/** How long (s) setting a mine takes. */
extern const float kSetMineTime;

/** Is an item of this base item, with or without a Trap property, a mine? */
bool isMineItem(int baseItem, bool hasTrapProperty);

/** The traps.2da row of a mine item: the row whose "resref" (the item recovering
 *  the mine gives) is the item's template, else @p trapProperty (the subtype of
 *  its Trap property), else -1. */
int findTrapType(const Aurora::TwoDAFile &traps, const Common::UString &itemResRef, int trapProperty);

/** Where a creature standing at @p position facing @p orientation (degrees, as
 *  Object::getOrientation()) lays a mine. */
glm::vec2 getMinePlacement(const glm::vec2 &position, float orientation, float distance);

/** The corners of a laid mine's trigger, relative to its position. */
std::vector<glm::vec3> getMineGeometry(float halfSize);

/** The detect or disarm DC of a mine set with this Demolitions skill. */
int getSetMineDC(int demolitions, int dcModifier);

} // End of namespace TrapRules

/** The trap state of a trigger, door or placeable. */
class Trap {
public:
	/** Read the trap fields of a blueprint or instance (missing fields keep their value). */
	void load(const Aurora::GFF3Struct &gff);

	/** Save the trap state (including who has detected it). */
	void saveState(Aurora::GFF3WriterStruct &gff) const;
	/** Restore a state saved by saveState(). */
	void loadState(const Aurora::GFF3Struct &gff);

	// Properties

	/** Is there a trap at all? */
	bool isTrapped() const;
	/** Is the trap armed, i.e. will it go off? */
	bool isActive() const;
	bool isDetectable() const;
	bool isDisarmable() const;
	bool isRecoverable() const;
	bool isOneShot() const;
	/** Has a party member marked the trap ("flagged" a mine)? */
	bool isFlagged() const;

	uint8_t getType() const;
	uint8_t getDetectDC() const;
	uint8_t getDisarmDC() const;
	const Common::UString &getKeyTag() const;

	void setTrapped(bool trapped);
	void setType(uint8_t type);
	void setDetectable(bool detectable);
	void setDisarmable(bool disarmable);
	void setOneShot(bool oneShot);
	void setDetectDC(uint8_t dc);
	void setDisarmDC(uint8_t dc);
	void setFlagged(bool flagged);

	/** The creature that set the trap (ID; 0 for traps placed by the module). */
	uint32_t getCreatorID() const;
	void setCreatorID(uint32_t id);

	// Detection

	/** Has anyone detected the trap? */
	bool isDetected() const;
	/** Has this object detected the trap? */
	bool isDetectedBy(uint32_t id) const;
	/** Mark the trap as (un)detected by an object. */
	void setDetectedBy(uint32_t id, bool detected = true);
	/** The objects that detected the trap, in order. */
	const std::vector<uint32_t> &getDetectors() const;

	// Lifetime

	/** The trap went off. Returns true if it is still armed afterwards. */
	bool fire();
	/** The trap is gone (disarmed, recovered or disabled by a script). */
	void disable();

	// traps.2da

	/** Model shown once the trap is detected, the sound it explodes with,
	 *  the item recovering it gives, and the name StrRef. */
	void setTypeInfo(const Common::UString &model, const Common::UString &explosionSound,
	                 const Common::UString &itemResRef, uint32_t nameStrRef);

	const Common::UString &getModel() const;
	const Common::UString &getExplosionSound() const;
	const Common::UString &getItemResRef() const;
	uint32_t getNameStrRef() const;

private:
	bool _flag { false };
	bool _detectable { true };
	bool _disarmable { true };
	bool _recoverable { true };
	bool _oneShot { true };
	bool _active { false };
	bool _flagged { false };

	uint8_t _type { 0 };
	uint8_t _detectDC { 15 };
	uint8_t _disarmDC { 15 };

	Common::UString _keyTag;

	uint32_t _creatorID { 0 };
	std::vector<uint32_t> _detectors;

	Common::UString _model;
	Common::UString _explosionSound;
	Common::UString _itemResRef;
	uint32_t _nameStrRef { 0xFFFFFFFF };
};

} // End of namespace KotORBase

} // End of namespace Engines

#endif // ENGINES_KOTORBASE_TRAP_H
