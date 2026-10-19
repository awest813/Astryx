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
 *  Situated object within an area in KotOR games.
 */

#include "src/common/error.h"
#include "src/common/maths.h"
#include "src/common/util.h"

#include "src/aurora/gff3file.h"
#include "src/aurora/gff3writer.h"
#include "src/aurora/2dafile.h"
#include "src/aurora/2dareg.h"
#include "src/aurora/nwscript/objectman.h"

#include "src/graphics/aurora/model.h"

#include "src/engines/aurora/model.h"

#include "src/engines/kotorbase/objectcontainer.h"
#include "src/engines/kotorbase/situated.h"

namespace Engines {

namespace KotORBase {

namespace BashRules {

const float kExtraReach = 1.0f;

bool canBash(bool locked, bool plot, bool notBlastable, bool broken, int hitPoints) {
	return locked && !plot && !notBlastable && !broken && (hitPoints > 0);
}

int damageAfterHardness(int damage, int hardness) {
	return MAX(damage - MAX(hardness, 0), 0);
}

int hitPointsAfterDamage(int hitPoints, int damage, bool minOneHitPoint) {
	const int left = hitPoints - MAX(damage, 0);
	return MAX(left, minOneHitPoint ? MIN(hitPoints, 1) : 0);
}

} // End of namespace BashRules

Situated::Situated(ObjectType type) :
		Object(type),
		_appearanceID(Aurora::kFieldIDInvalid),
		_soundAppType(Aurora::kFieldIDInvalid),
		_locked(false),
		_lastOpenedBy(0),
		_lastClosedBy(0),
		_lastUsedBy(0) {

}

void Situated::show() {
	if (_model)
		_model->show();
}

void Situated::hide() {
	if (_model)
		_model->hide();
}

bool Situated::isVisible() const {
	return _model && _model->isVisible();
}

void Situated::setPosition(float x, float y, float z) {
	Object::setPosition(x, y, z);
	Object::getPosition(x, y, z);

	if (_model)
		_model->setPosition(x, y, z);
}

void Situated::setOrientation(float x, float y, float z, float angle) {
	Object::setOrientation(x, y, z, angle);
	Object::getOrientation(x, y, z, angle);

	if (_model)
		_model->setOrientation(x, y, z, angle);
}

void Situated::playAnimation(const Common::UString &anim, bool restart, float length, float speed) {
	if (_model)
		_model->playAnimation(anim, restart, length, speed);
}

void Situated::getTooltipAnchor(float &x, float &y, float &z) const {
	if (!_model) {
		Object::getTooltipAnchor(x, y, z);
		return;
	}

	_model->getTooltipAnchor(x, y, z);
}

bool Situated::isLocked() const {
	return _locked;
}

bool Situated::isKeyRequired() const {
	return _keyRequired;
}

const Common::UString &Situated::getKeyTag() const {
	return _keyTag;
}

int Situated::getLockDC() const {
	return _lockDC;
}

int Situated::getLockRequiredSkill() const {
	return _lockSkill;
}

void Situated::setLocked(bool locked) {
	_locked = locked;
}

int Situated::getHardness() const {
	return _hardness;
}

bool Situated::isNotBlastable() const {
	return _notBlastable;
}

bool Situated::isBroken() const {
	return _broken;
}

bool Situated::isDead() const {
	return _broken;
}

bool Situated::canBeBashed() const {
	return BashRules::canBash(_locked, _plotFlag, _notBlastable, _broken, _currentHitPoints);
}

void Situated::takeBashDamage(int damage, Object *attacker) {
	takeDamage(BashRules::damageAfterHardness(damage, _hardness), attacker);
}

void Situated::takeDamage(int damage, Object *attacker) {
	if (_broken)
		return;

	if (attacker)
		_lastHostileActor = attacker;

	if ((_maxHitPoints <= 0) && (_currentHitPoints <= 0))
		return;

	// Plot objects can't be harmed.
	if (_plotFlag || (damage <= 0))
		return;

	_currentHitPoints = BashRules::hitPointsAfterDamage(_currentHitPoints, damage, _minOneHitPoint);
	_damagedEventPending = true;

	if (_currentHitPoints <= 0) {
		// Dead at once, so nobody swings at it again; it opens once its OnDeath ran.
		_broken = true;
		_deathEventPending = true;
	}
}

void Situated::notifyAttacked(Object *attacker) {
	if (attacker)
		_lastHostileActor = attacker;

	_attackedEventPending = true;
}

Object *Situated::getLastHostileActor() const {
	return _lastHostileActor;
}

bool Situated::consumeAttackedEvent() {
	const bool pending = _attackedEventPending;
	_attackedEventPending = false;
	return pending;
}

bool Situated::consumeDamagedEvent() {
	const bool pending = _damagedEventPending;
	_damagedEventPending = false;
	return pending;
}

bool Situated::consumeDeathEvent() {
	const bool pending = _deathEventPending;
	_deathEventPending = false;
	return pending;
}

void Situated::breakOpen(Object *UNUSED(breaker)) {
	_broken = true;
	_locked = false;

	playSound(_soundDestroyed);
}

bool Situated::getIsTrapped() const {
	return _trap.isTrapped();
}

bool Situated::getTrapActive() const {
	return _trap.isActive();
}

bool Situated::getTrapDetectable() const {
	return _trap.isDetectable();
}

bool Situated::getTrapDisarmable() const {
	return _trap.isDisarmable();
}

bool Situated::getTrapFlagged() const {
	return _trap.isFlagged();
}

bool Situated::getTrapOneShot() const {
	return _trap.isOneShot();
}

uint8_t Situated::getTrapBaseType() const {
	return _trap.getType();
}

uint8_t Situated::getTrapDetectDC() const {
	return _trap.getDetectDC();
}

uint8_t Situated::getTrapDisarmDC() const {
	return _trap.getDisarmDC();
}

const Common::UString &Situated::getTrapKeyTag() const {
	return _trap.getKeyTag();
}

Object *Situated::getTrapDetectedBy() const {
	if (_trap.getDetectors().empty())
		return nullptr;

	return ObjectContainer::toObject(ObjectMan.findObject(_trap.getDetectors().front()));
}

Object *Situated::getTrapCreator() const {
	if (_trap.getCreatorID() == 0)
		return nullptr;

	return ObjectContainer::toObject(ObjectMan.findObject(_trap.getCreatorID()));
}

void Situated::setTrapDetectedBy(Object *detector) {
	if (detector)
		_trap.setDetectedBy(detector->getID());
}

void Situated::setTrapDisabled() {
	_trap.disable();
}

void Situated::triggerTrap(Object *triggerer) {
	if (!getTrapActive())
		return;

	_trap.fire();
	runScript(kScriptTrapTriggered, this, triggerer);
}

Trap &Situated::getTrap() {
	return _trap;
}

const Trap &Situated::getTrap() const {
	return _trap;
}

void Situated::saveState(Aurora::GFF3WriterStruct &gff) const {
	Object::saveState(gff);
	gff.addByte("Locked", _locked ? 1 : 0);
	gff.addByte("Bashed", _broken ? 1 : 0);
	_trap.saveState(gff);
}

void Situated::loadState(const Aurora::GFF3Struct &gff) {
	Object::loadState(gff);
	_locked = gff.getBool("Locked", _locked);
	_broken = gff.getBool("Bashed", _broken);
	_trap.loadState(gff);
}

Object *Situated::getLastOpenedBy() const {
	return _lastOpenedBy;
}

Object *Situated::getLastClosedBy() const {
	return _lastClosedBy;
}

void Situated::forgetObject(const Object *object) {
	if (_lastOpenedBy == object)
		_lastOpenedBy = nullptr;
	if (_lastClosedBy == object)
		_lastClosedBy = nullptr;
	if (_lastUsedBy == object)
		_lastUsedBy = nullptr;
	if (_lastHostileActor == object)
		_lastHostileActor = nullptr;
}

Object *Situated::getLastUsedBy() const {
	return _lastUsedBy;
}

const Common::UString &Situated::getConversation() const {
	return _conversation;
}

const Common::UString &Situated::getModelName() const {
	return _modelName;
}

const std::vector<int> Situated::getPossibleActions() const {
	std::vector<int> actions;
	if (_locked && !_keyRequired)
		actions.push_back(kActionOpenLock);

	return actions;
}

void Situated::setUsable(bool usable) {
	_usable = usable;
	if (_model)
		_model->setClickable(isClickable());
}

void Situated::load(const Aurora::GFF3Struct &instance, const Aurora::GFF3Struct *blueprint) {
	// General properties

	if (blueprint)
		loadProperties(*blueprint); // Blueprint
	loadProperties(instance);    // Instance


	// Specialized object properties

	if (blueprint)
		loadObject(*blueprint); // Blueprint
	loadObject(instance);    // Instance


	// Appearance

	if (_appearanceID == Aurora::kFieldIDInvalid)
		warning("Situated object \"%s\" without an appearance", _tag.c_str());

	loadAppearance();
	loadSounds();

	// Model

	if (!_modelName.empty()) {
		_model.reset(loadModelObject(_modelName));

		if (!_model)
			throw Common::Exception("Failed to load situated object model \"%s\"",
			                        _modelName.c_str());
	} else
		warning("Situated object \"%s\" (\"%s\") has no model", _name.c_str(), _tag.c_str());

	if (_model) {
		// Clickable
		_model->setTag(_tag);
		_model->setClickable(isClickable());

		// ID
		_ids.push_back(_model->getID());
	}

	// Position

	setPosition(instance.getDouble("X"),
	            instance.getDouble("Y"),
	            instance.getDouble("Z"));

	// Orientation

	float bearing = instance.getDouble("Bearing");

	setOrientation(0.0f, 0.0f, 1.0f, Common::rad2deg(bearing));
}

void Situated::loadProperties(const Aurora::GFF3Struct &gff) {
	// Tag
	_tag = gff.getString("Tag", _tag);

	// Name
	_name = gff.getString("LocName", _name);

	// Description
	_description = gff.getString("Description", _description);

	// Portrait
	loadPortrait(gff);

	// Appearance
	_appearanceID = gff.getUint("Appearance", _appearanceID);

	// Static
	_static = gff.getBool("Static", _static);

	// Usable
	_usable = gff.getBool("Useable", _usable);

	// Locked
	_locked = gff.getBool("Locked", _locked);

	// Key required
	_keyRequired = gff.getBool("KeyRequired", _keyRequired);

	// Key tag
	_keyTag = gff.getString("KeyTag", _keyTag);

	// Lock DC
	_lockDC = gff.getUint("LockDC", _lockDC);

	// Lock Skill
	_lockSkill = gff.getUint("LockSkill", _lockSkill);

	// Plot (indestructible), and what bashing it open takes
	_plotFlag     = gff.getBool("Plot", _plotFlag);
	_hardness     = gff.getUint("Hardness", _hardness);
	_notBlastable = gff.getBool("NotBlastable", _notBlastable);

	_trap.load(gff);

	// Conversation
	_conversation = gff.getString("Conversation", _conversation);

	// Faction
	_faction = Faction(gff.getUint("FactionID"));

	// Scripts
	readScripts(gff, false);
}

void Situated::loadPortrait(const Aurora::GFF3Struct &gff) {
	uint32_t portraitID = gff.getUint("PortraitId");
	if (portraitID != 0) {
		const Aurora::TwoDAFile &twoda = TwoDAReg.get2DA("portraits");

		Common::UString portrait = twoda.getRow(portraitID).getString("BaseResRef");
		if (!portrait.empty())
			_portrait = "po_" + portrait;
	}

	_portrait = gff.getString("Portrait", _portrait);
}

void Situated::loadSounds() {
	if (_soundAppType == Aurora::kFieldIDInvalid)
		return;

	const Aurora::TwoDAFile &twoda = TwoDAReg.get2DA("placeableobjsnds");

	_soundOpened    = twoda.getRow(_soundAppType).getString("Opened");
	_soundClosed    = twoda.getRow(_soundAppType).getString("Closed");
	_soundDestroyed = twoda.getRow(_soundAppType).getString("Destroyed");
	_soundUsed      = twoda.getRow(_soundAppType).getString("Used");
	_soundLocked    = twoda.getRow(_soundAppType).getString("Locked");
}

} // End of namespace KotORBase

} // End of namespace Engines
