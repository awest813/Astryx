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

#include "src/common/endianness.h"
#include "src/common/error.h"
#include "src/common/debug.h"

#include "src/aurora/resman.h"
#include "src/aurora/gff3file.h"
#include "src/aurora/gff3writer.h"
#include "src/aurora/2dafile.h"
#include "src/aurora/2dareg.h"
#include "src/aurora/talkman.h"

#include "src/graphics/aurora/model.h"

#include "src/engines/aurora/util.h"
#include "src/engines/aurora/model.h"

#include "src/engines/kotorbase/trigger.h"

namespace Engines {

namespace KotORBase {

Trigger::Trigger(const Aurora::GFF3Struct &gff) :
		Engines::Trigger(),
		Object(kObjectTypeTrigger) {
	load(gff);
	prepare();
}

Trigger::~Trigger() {
}

void Trigger::show() {
	Renderable::show();

	_shown = true;
	updateTrapModel();
}

void Trigger::hide() {
	Renderable::hide();

	_shown = false;
	updateTrapModel();
}

void Trigger::notifyNotSeen() {
}

bool Trigger::isVisible() const {
	return Renderable::isVisible();
}

void Trigger::load(const Aurora::GFF3Struct &gff) {
	_templateResRef = gff.getString("TemplateResRef");

	std::unique_ptr<Aurora::GFF3File> utt;
	if (!_templateResRef.empty())
		utt.reset(loadOptionalGFF3(_templateResRef, Aurora::kFileTypeUTT, MKTAG('U', 'T', 'T', ' ')));

	if (utt) {
		loadBlueprint(utt->getTopLevel());
		loadLinkedTo(utt->getTopLevel());
		loadTrap(utt->getTopLevel());
	} else {
		// Fall back to what the GIT instance itself carries.
		warning("Trigger \"%s\" has no blueprint", _templateResRef.c_str());
		loadBlueprint(gff);
	}

	loadLinkedTo(gff);
	loadTrap(gff);
	loadTrapInfo();

	float x, y, z;
	x = (float)gff.getDouble("XPosition");
	y = (float)gff.getDouble("YPosition");
	z = (float)gff.getDouble("ZPosition");
	glm::vec3 position(x, y, z);
	setPosition(x, y, z);

	x = (float)gff.getDouble("XOrientation");
	y = (float)gff.getDouble("YOrientation");
	z = (float)gff.getDouble("ZOrientation");
	glm::vec3 orientation(x, y, z);

	const Aurora::GFF3List &geometry = gff.getList("Geometry");
	for (Aurora::GFF3List::const_iterator p = geometry.begin();
			p != geometry.end();
			++p) {
		x = (float)(*p)->getDouble("PointX");
		y = (float)(*p)->getDouble("PointY");
		z = (float)(*p)->getDouble("PointZ");
		_geometry.push_back(position + glm::vec3(x, y, z));
	}
}

void Trigger::loadLinkedTo(const Aurora::GFF3Struct &gff) {
	_linkedTo       = gff.getString("LinkedTo", _linkedTo);
	_linkedToModule = gff.getString("LinkedToModule", _linkedToModule);
}

bool Trigger::isTransition() const {
	return !_linkedTo.empty();
}

const Common::UString &Trigger::getLinkedTo() const {
	return _linkedTo;
}

const Common::UString &Trigger::getLinkedToModule() const {
	return _linkedToModule;
}

void Trigger::loadBlueprint(const Aurora::GFF3Struct &gff) {
	Object::_tag = gff.getString("Tag");

	_name = gff.getString("LocalizedName");

	readScripts(gff);
}

void Trigger::loadTrap(const Aurora::GFF3Struct &gff) {
	_kind = gff.getUint("Type", _kind);

	if (gff.hasField("Faction"))
		_faction = Faction(gff.getUint("Faction"));

	_trap.load(gff);

	// Mines are triggers of the trap kind; a few don't set TrapFlag as well.
	if ((_kind == kKindTrap) && !_trap.isTrapped() && !gff.hasField("TrapFlag"))
		_trap.setTrapped(true);

	// A mine without a faction goes off for the party, like the "Trap" faction.
	if (_trap.isTrapped() && (_faction == kFactionInvalid))
		_faction = kFactionTrap;
}

void Trigger::loadTrapInfo() {
	if (!_trap.isTrapped())
		return;

	try {
		const Aurora::TwoDAFile &traps = TwoDAReg.get2DA("traps");
		if (_trap.getType() >= traps.getRowCount()) {
			warning("Trigger \"%s\": unknown trap type %u", Object::_tag.c_str(), (uint)_trap.getType());
			return;
		}

		const Aurora::TwoDARow &row = traps.getRow(_trap.getType());

		const uint32_t nameStrRef = row.empty("name") ? 0xFFFFFFFF : static_cast<uint32_t>(row.getInt("name"));
		_trap.setTypeInfo(row.getString("model"), row.getString("explosionsound"),
		                  row.getString("resref"), nameStrRef);

		// A mine is called after its kind ("Minor Frag Mine").
		if (_name.empty() && (nameStrRef != 0xFFFFFFFF))
			_name = TalkMan.getString(nameStrRef);

	} catch (...) {
		warning("Trigger \"%s\": could not read traps.2da", Object::_tag.c_str());
	}
}

TriggerOccupancy &Trigger::getOccupancy() {
	return _occupancy;
}

const TriggerOccupancy &Trigger::getOccupancy() const {
	return _occupancy;
}

uint32_t Trigger::getLastEnteredID() const {
	return _lastEnteredID;
}

uint32_t Trigger::getLastExitedID() const {
	return _lastExitedID;
}

void Trigger::setLastEnteredID(uint32_t id) {
	_lastEnteredID = id;
}

void Trigger::setLastExitedID(uint32_t id) {
	_lastExitedID = id;
}

bool Trigger::isTrap() const {
	return _trap.isTrapped();
}

bool Trigger::isTrapActive() const {
	return _trap.isActive();
}

bool Trigger::isTrapDetected() const {
	return _trap.isTrapped() && _trap.isDetected();
}

bool Trigger::isTrapDisarmable() const {
	return _trap.isActive() && _trap.isDisarmable();
}

Trap &Trigger::getTrap() {
	return _trap;
}

const Trap &Trigger::getTrap() const {
	return _trap;
}

void Trigger::detectTrap(Object *detector) {
	if (!_trap.isTrapped() || !detector)
		return;

	_trap.setDetectedBy(detector->getID());
	updateTrapModel();
}

bool Trigger::fireTrap(Object *triggerer) {
	if (!_trap.isActive())
		return false;

	// A one-shot mine is spent once it went off.
	if (!_trap.fire())
		_trap.disable();

	runScript(kScriptTrapTriggered, this, triggerer);

	updateTrapModel();
	return true;
}

void Trigger::disableTrap() {
	_trap.disable();
	updateTrapModel();
}

void Trigger::updateTrapModel() {
	const bool wantModel = _trap.isTrapped() && _trap.isDetected() && !_trap.getModel().empty();

	if (wantModel && !_trapModel) {
		try {
			_trapModel.reset(loadModelObject(_trap.getModel()));
		} catch (...) {
			_trapModel.reset();
		}

		if (!_trapModel) {
			warning("Trigger \"%s\": failed to load mine model \"%s\"", Object::_tag.c_str(), _trap.getModel().c_str());
			// Don't try again every time.
			_trap.setTypeInfo("", _trap.getExplosionSound(), _trap.getItemResRef(), _trap.getNameStrRef());
			return;
		}

		float x, y, z;
		getPosition(x, y, z);
		_trapModel->setPosition(x, y, z);
		_trapModel->setTag(Object::_tag);
		_trapModel->setClickable(true);
		_trapModel->playAnimation("detect", false);

		// Lets the area find the mine under the cursor.
		_ids.push_back(_trapModel->getID());
	}

	if (!_trapModel)
		return;

	if (wantModel && _shown)
		_trapModel->show();
	else
		_trapModel->hide();
}

void Trigger::saveState(Aurora::GFF3WriterStruct &gff) const {
	Object::saveState(gff);
	_trap.saveState(gff);
}

void Trigger::loadState(const Aurora::GFF3Struct &gff) {
	Object::loadState(gff);
	_trap.loadState(gff);

	updateTrapModel();
}

} // End of namespace KotORBase

} // End of namespace Engines
