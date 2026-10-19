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
 */

#include <algorithm>
#include <cmath>

#include "src/common/maths.h"
#include "src/common/util.h"

#include "src/aurora/gff3file.h"
#include "src/aurora/gff3writer.h"
#include "src/aurora/2dafile.h"

#include "src/engines/kotorbase/trap.h"

namespace Engines {

namespace KotORBase {

namespace TrapRules {

const float kDetectRange      = 10.0f;
const float kDisarmRange      =  2.0f;
const int   kRecoverDCPenalty =  5;
const int   kSetOffMargin     =  5;

int checkRoll(bool inCombat, int take, int d20) {
	return inCombat ? d20 : take;
}

bool detects(int awareness, int roll, int detectDC) {
	return (roll + awareness) >= detectDC;
}

int getRecoverDC(int disarmDC) {
	return disarmDC + kRecoverDCPenalty;
}

DisarmResult disarm(int demolitions, int roll, int disarmDC, bool recover) {
	const int dc    = recover ? getRecoverDC(disarmDC) : disarmDC;
	const int total = roll + demolitions;

	if (total >= dc)
		return kDisarmSucceeded;

	return ((dc - total) >= kSetOffMargin) ? kDisarmSetOff : kDisarmFailed;
}

bool unlocks(int security, int roll, int lockDC) {
	return (roll + security) >= lockDC;
}

const int   kBaseItemMine      = 58;
const int   kItemPropertyTrap  = 46;
const float kMinePlaceDistance = 1.0f;
const float kMineHalfSize      = 0.75f;
const float kSetMineTime       = 1.5f;

bool isMineItem(int baseItem, bool hasTrapProperty) {
	return (baseItem == kBaseItemMine) || hasTrapProperty;
}

int findTrapType(const Aurora::TwoDAFile &traps, const Common::UString &itemResRef, int trapProperty) {
	if (!itemResRef.empty()) {
		for (size_t i = 0; i < traps.getRowCount(); ++i)
			if (traps.getRow(i).getString("resref").equalsIgnoreCase(itemResRef))
				return static_cast<int>(i);
	}

	if ((trapProperty >= 0) && (static_cast<size_t>(trapProperty) < traps.getRowCount()))
		return trapProperty;

	return -1;
}

glm::vec2 getMinePlacement(const glm::vec2 &position, float orientation, float distance) {
	// An orientation of 0 faces +Y (Object::makeLookAt()).
	const float facing = Common::deg2rad(orientation + 90.0f);
	return position + distance * glm::vec2(std::cos(facing), std::sin(facing));
}

std::vector<glm::vec3> getMineGeometry(float halfSize) {
	return {
		glm::vec3(-halfSize, -halfSize, 0.0f),
		glm::vec3( halfSize, -halfSize, 0.0f),
		glm::vec3( halfSize,  halfSize, 0.0f),
		glm::vec3(-halfSize,  halfSize, 0.0f)
	};
}

int getSetMineDC(int demolitions, int dcModifier) {
	return CLIP(10 + MAX(demolitions, 0) + dcModifier, 1, 255);
}

} // End of namespace TrapRules

void Trap::load(const Aurora::GFF3Struct &gff) {
	_flag        = gff.getBool("TrapFlag", _flag);
	_type       = static_cast<uint8_t>(gff.getUint("TrapType", _type));
	_detectable  = gff.getBool("TrapDetectable", _detectable);
	_disarmable  = gff.getBool("TrapDisarmable", _disarmable);
	_recoverable = gff.getBool("TrapRecoverable", _recoverable);
	_oneShot     = gff.getBool("TrapOneShot", _oneShot);
	_active      = gff.getBool("TrapActive", _flag);
	_flagged     = gff.getBool("TrapFlagged", _flagged);
	_detectDC    = static_cast<uint8_t>(gff.getUint("TrapDetectDC", _detectDC));
	_disarmDC    = static_cast<uint8_t>(gff.getUint("DisarmDC", _disarmDC));
	_keyTag      = gff.getString("KeyName", _keyTag);
}

void Trap::saveState(Aurora::GFF3WriterStruct &gff) const {
	gff.addByte("TrapFlag", _flag ? 1 : 0);
	gff.addByte("TrapDetectable", _detectable ? 1 : 0);
	gff.addByte("TrapDisarmable", _disarmable ? 1 : 0);
	gff.addByte("TrapRecoverable", _recoverable ? 1 : 0);
	gff.addByte("TrapOneShot", _oneShot ? 1 : 0);
	gff.addByte("TrapActive", _active ? 1 : 0);
	gff.addByte("TrapFlagged", _flagged ? 1 : 0);
	gff.addUint32("TrapType", _type);
	gff.addUint32("TrapDetectDC", _detectDC);
	gff.addUint32("DisarmDC", _disarmDC);
	gff.addExoString("KeyName", _keyTag);

	// Object IDs don't survive a reload; remember that the trap was seen.
	gff.addByte("TrapDetected", isDetected() ? 1 : 0);
}

void Trap::loadState(const Aurora::GFF3Struct &gff) {
	load(gff);

	_detectors.clear();
	_flagged = _flagged || gff.getBool("TrapDetected", false);
}

bool Trap::isTrapped() const {
	return _flag;
}

bool Trap::isActive() const {
	return _flag && _active;
}

bool Trap::isDetectable() const {
	return _detectable;
}

bool Trap::isDisarmable() const {
	return _disarmable;
}

bool Trap::isRecoverable() const {
	return _recoverable;
}

bool Trap::isOneShot() const {
	return _oneShot;
}

bool Trap::isFlagged() const {
	return _flag && _flagged;
}

uint8_t Trap::getType() const {
	return _type;
}

uint8_t Trap::getDetectDC() const {
	return _detectDC;
}

uint8_t Trap::getDisarmDC() const {
	return _disarmDC;
}

const Common::UString &Trap::getKeyTag() const {
	return _keyTag;
}

void Trap::setTrapped(bool trapped) {
	_flag   = trapped;
	_active = trapped;
}

void Trap::setType(uint8_t type) {
	_type = type;
}

void Trap::setDetectable(bool detectable) {
	_detectable = detectable;
}

void Trap::setDisarmable(bool disarmable) {
	_disarmable = disarmable;
}

void Trap::setOneShot(bool oneShot) {
	_oneShot = oneShot;
}

void Trap::setDetectDC(uint8_t dc) {
	_detectDC = dc;
}

void Trap::setDisarmDC(uint8_t dc) {
	_disarmDC = dc;
}

void Trap::setFlagged(bool flagged) {
	_flagged = flagged;
}

uint32_t Trap::getCreatorID() const {
	return _creatorID;
}

void Trap::setCreatorID(uint32_t id) {
	_creatorID = id;
}

bool Trap::isDetected() const {
	return isFlagged() || !_detectors.empty();
}

bool Trap::isDetectedBy(uint32_t id) const {
	return std::find(_detectors.begin(), _detectors.end(), id) != _detectors.end();
}

void Trap::setDetectedBy(uint32_t id, bool detected) {
	auto it = std::find(_detectors.begin(), _detectors.end(), id);

	if (detected && (it == _detectors.end()))
		_detectors.push_back(id);
	else if (!detected && (it != _detectors.end()))
		_detectors.erase(it);
}

const std::vector<uint32_t> &Trap::getDetectors() const {
	return _detectors;
}

bool Trap::fire() {
	if (!isActive())
		return false;

	if (_oneShot)
		_active = false;

	return _active;
}

void Trap::disable() {
	_flag    = false;
	_active  = false;
	_flagged = false;
	_detectors.clear();
}

void Trap::setTypeInfo(const Common::UString &model, const Common::UString &explosionSound,
                       const Common::UString &itemResRef, uint32_t nameStrRef) {

	_model          = model;
	_explosionSound = explosionSound;
	_itemResRef     = itemResRef;
	_nameStrRef     = nameStrRef;
}

const Common::UString &Trap::getModel() const {
	return _model;
}

const Common::UString &Trap::getExplosionSound() const {
	return _explosionSound;
}

const Common::UString &Trap::getItemResRef() const {
	return _itemResRef;
}

uint32_t Trap::getNameStrRef() const {
	return _nameStrRef;
}

} // End of namespace KotORBase

} // End of namespace Engines
